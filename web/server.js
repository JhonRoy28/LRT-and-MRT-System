const express = require("express");
const fs = require("fs");
const path = require("path");

const app = express();
const PORT = 3000;

const DATA_DIR = path.join(__dirname, "data");
const DATA_FILE = path.join(DATA_DIR, "trips.json");
const FARES_FILE = path.join(DATA_DIR, "fares.json");

const LINES = {
  LRT1: "LRT-1",
  LRT2: "LRT-2",
  MRT3: "MRT-3"
};

const STATIONS = {
  LRT1: [
    "Baclaran", "EDSA", "Libertad", "Gil Puyat", "Vito Cruz", "Quirino",
    "Pedro Gil", "UN Ave", "Central", "Carriedo", "Doroteo Jose", "Bambang",
    "Tayuman", "Blumentritt", "Abad Santos", "R. Papa", "5th Ave",
    "Monumento", "Balintawak", "Roosevelt (FPJ)"
  ],
  LRT2: [
    "Recto", "Legarda", "Pureza", "V. Mapa", "J. Ruiz", "Gilmore",
    "Betty Go-Belmonte", "Cubao", "Anonas", "Katipunan", "Santolan",
    "Marikina", "Antipolo"
  ],
  MRT3: [
    "North Ave", "Quezon Ave", "Kamuning", "Cubao", "Santolan-Annapolis",
    "Ortigas", "Shaw", "Boni", "Guadalupe", "Buendia", "Ayala",
    "Magallanes", "Taft"
  ]
};

const DEFAULT_FARES = {
  LRT1: {
    shortMax: 5,
    mediumMax: 10,
    shortFare: 15,
    mediumFare: 20,
    longFare: 30
  },
  LRT2: {
    shortMax: 5,
    mediumMax: 10,
    shortFare: 15,
    mediumFare: 20,
    longFare: 30
  },
  MRT3: {
    range1: 3,
    range2: 6,
    range3: 9,
    range4: 12,
    range5: 13,
    fare1: 13,
    fare2: 16,
    fare3: 20,
    fare4: 24,
    fare5: 28
  }
};

const DISCOUNTS = {
  NONE: 1,
  STUDENT: 2,
  PWD: 3,
  SENIOR: 4
};

const TRANSFERS = [
  { fromLine: LINES.LRT1, fromStation: "Doroteo Jose", toLine: LINES.LRT2, toStation: "Recto" },
  { fromLine: LINES.LRT2, fromStation: "Recto", toLine: LINES.LRT1, toStation: "Doroteo Jose" },
  { fromLine: LINES.MRT3, fromStation: "Taft", toLine: LINES.LRT1, toStation: "EDSA" },
  { fromLine: LINES.LRT1, fromStation: "EDSA", toLine: LINES.MRT3, toStation: "Taft" },
  { fromLine: LINES.LRT2, fromStation: "Cubao", toLine: LINES.MRT3, toStation: "Cubao" },
  { fromLine: LINES.MRT3, fromStation: "Cubao", toLine: LINES.LRT2, toStation: "Cubao" }
];

app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

function ensureDataDir() {
  if (!fs.existsSync(DATA_DIR)) {
    fs.mkdirSync(DATA_DIR, { recursive: true });
  }
}

function ensureDataFile() {
  ensureDataDir();
  if (!fs.existsSync(DATA_FILE)) {
    fs.writeFileSync(DATA_FILE, JSON.stringify([]), "utf8");
  }
}

function ensureFareFile() {
  ensureDataDir();
  if (!fs.existsSync(FARES_FILE)) {
    fs.writeFileSync(FARES_FILE, JSON.stringify(DEFAULT_FARES, null, 2), "utf8");
  }
}

function readTrips() {
  ensureDataFile();
  const raw = fs.readFileSync(DATA_FILE, "utf8");
  return JSON.parse(raw);
}

function writeTrips(trips) {
  ensureDataFile();
  fs.writeFileSync(DATA_FILE, JSON.stringify(trips, null, 2), "utf8");
}

function readFares() {
  ensureFareFile();
  const raw = fs.readFileSync(FARES_FILE, "utf8");
  return JSON.parse(raw);
}

function writeFares(fares) {
  ensureFareFile();
  fs.writeFileSync(FARES_FILE, JSON.stringify(fares, null, 2), "utf8");
}

let fares = readFares();

function stationIndex(line, station) {
  const list = STATIONS[lineKey(line)];
  return list.indexOf(station);
}

function lineKey(line) {
  if (line === LINES.LRT1) return "LRT1";
  if (line === LINES.LRT2) return "LRT2";
  return "MRT3";
}

function stationCount(fromIndex, toIndex) {
  return Math.abs(fromIndex - toIndex);
}

function fareForRide(line, stations) {
  if (line === LINES.LRT1) {
    const rule = fares.LRT1;
    if (stations <= rule.shortMax) return rule.shortFare;
    if (stations <= rule.mediumMax) return rule.mediumFare;
    return rule.longFare;
  }
  if (line === LINES.LRT2) {
    const rule = fares.LRT2;
    if (stations <= rule.shortMax) return rule.shortFare;
    if (stations <= rule.mediumMax) return rule.mediumFare;
    return rule.longFare;
  }
  const rule = fares.MRT3;
  if (stations <= rule.range1) return rule.fare1;
  if (stations <= rule.range2) return rule.fare2;
  if (stations <= rule.range3) return rule.fare3;
  if (stations <= rule.range4) return rule.fare4;
  return rule.fare5;
}

function applyDiscount(fare, discountType) {
  if (discountType === DISCOUNTS.STUDENT || discountType === DISCOUNTS.PWD || discountType === DISCOUNTS.SENIOR) {
    return fare * 0.5;
  }
  return fare;
}

function isValidTransfer(prevLine, prevEnd, nextLine) {
  if (prevLine === nextLine) {
    return true;
  }
  return TRANSFERS.some((transfer) =>
    transfer.fromLine === prevLine &&
    transfer.fromStation === prevEnd &&
    transfer.toLine === nextLine
  );
}

function computeTrip(payload) {
  const rides = payload.rides || [];
  if (rides.length === 0) {
    throw new Error("At least one ride is required.");
  }

  let baseTotal = 0;
  let paidTotal = 0;
  const computedRides = [];

  for (let i = 0; i < rides.length; i++) {
    const ride = rides[i];
    const line = ride.line;
    const from = ride.from;
    const to = ride.to;
    const discount = ride.discountType;

    const fromIndex = stationIndex(line, from);
    const toIndex = stationIndex(line, to);
    if (fromIndex < 0 || toIndex < 0) {
      throw new Error("Invalid station selection.");
    }
    if (fromIndex === toIndex) {
      throw new Error("From and to stations must be different.");
    }
    if (i > 0) {
      const prev = computedRides[i - 1];
      if (!isValidTransfer(prev.line, prev.to, line)) {
        throw new Error("Invalid transfer: you can only transfer at allowed stations.");
      }
    }

    const stations = stationCount(fromIndex, toIndex);
    const baseFare = fareForRide(line, stations);
    const finalFare = applyDiscount(baseFare, discount);

    baseTotal += baseFare;
    paidTotal += finalFare;

    computedRides.push({
      line,
      from,
      to,
      stations,
      discountType: discount,
      baseFare,
      finalFare
    });
  }

  return {
    baseTotal,
    paidTotal,
    discountTotal: baseTotal - paidTotal,
    rides: computedRides
  };
}

function computeCard(payload, paidTotal) {
  if (!payload.useCard) {
    return { useCard: false };
  }
  const startBalance = Number(payload.startBalance || 0);
  let endBalance = startBalance;
  let topupTotal = 0;

  if (endBalance < paidTotal) {
    const needed = paidTotal - endBalance;
    topupTotal += needed;
    endBalance += needed;
  }
  endBalance -= paidTotal;

  return {
    useCard: true,
    startBalance,
    endBalance,
    topupTotal
  };
}

app.get("/api/config", (req, res) => {
  res.json({
    lines: LINES,
    stations: STATIONS,
    discounts: DISCOUNTS,
    transfers: TRANSFERS,
    fares
  });
});

app.get("/api/fares", (req, res) => {
  res.json(fares);
});

app.put("/api/fares", (req, res) => {
  const payload = req.body;
  if (!payload || !payload.LRT1 || !payload.LRT2 || !payload.MRT3) {
    res.status(400).json({ error: "Invalid fare payload." });
    return;
  }
  fares = payload;
  writeFares(fares);
  res.json(fares);
});

app.get("/api/trips", (req, res) => {
  res.json(readTrips());
});

app.get("/api/trips/:id", (req, res) => {
  const trips = readTrips();
  const trip = trips.find((item) => item.id === req.params.id);
  if (!trip) {
    res.status(404).json({ error: "Trip not found." });
    return;
  }
  res.json(trip);
});

app.get("/api/trips/:id/receipt", (req, res) => {
  const trips = readTrips();
  const trip = trips.find((item) => item.id === req.params.id);
  if (!trip) {
    res.status(404).json({ error: "Trip not found." });
    return;
  }

  const lines = [];
  lines.push("Train Fare Receipt");
  lines.push(`Trip ID: ${trip.id}`);
  lines.push(`Passenger: ${trip.passengerName}`);
  lines.push(`Date: ${trip.createdAt}`);
  lines.push("");
  lines.push("Ride | Line | From -> To | Gaps | Discount | Base | Final");
  lines.push("-----+------+----------------+------+----------+------+-------");

  trip.rides.forEach((ride, index) => {
    lines.push(`${index + 1} | ${ride.line} | ${ride.from} -> ${ride.to} | ${ride.stations} | ${ride.discountType} | Php ${ride.baseFare.toFixed(2)} | Php ${ride.finalFare.toFixed(2)}`);
  });

  lines.push("");
  lines.push(`Base total: Php ${trip.baseTotal.toFixed(2)}`);
  lines.push(`Paid total: Php ${trip.paidTotal.toFixed(2)}`);
  lines.push(`Discount: Php ${trip.discountTotal.toFixed(2)}`);

  if (trip.useCard) {
    lines.push("");
    lines.push(`Card start: Php ${trip.startBalance.toFixed(2)}`);
    lines.push(`Card end: Php ${trip.endBalance.toFixed(2)}`);
    lines.push(`Top-up: Php ${trip.topupTotal.toFixed(2)}`);
  }

  res.setHeader("Content-Type", "text/plain");
  res.setHeader("Content-Disposition", `attachment; filename=${trip.id}.txt`);
  res.send(lines.join("\n"));
});

app.post("/api/trips", (req, res) => {
  try {
    const payload = req.body || {};
    const tripTotals = computeTrip(payload);
    const cardInfo = computeCard(payload, tripTotals.paidTotal);

    const trip = {
      id: `trip_${Date.now()}`,
      createdAt: new Date().toISOString(),
      passengerName: payload.passengerName || "Passenger",
      ...tripTotals,
      ...cardInfo
    };

    const trips = readTrips();
    trips.unshift(trip);
    writeTrips(trips);

    res.status(201).json(trip);
  } catch (err) {
    res.status(400).json({ error: err.message });
  }
});

app.get("/api/trips.csv", (req, res) => {
  const trips = readTrips();
  const rows = [
    "trip_id,created_at,passenger,ride_index,line,from,to,gaps,discount_type,base_fare,final_fare,trip_base,trip_paid,trip_discount"
  ];

  trips.forEach((trip) => {
    trip.rides.forEach((ride, index) => {
      rows.push(
        `${trip.id},${trip.createdAt},${trip.passengerName},${index + 1},${ride.line},${ride.from},${ride.to},${ride.stations},${ride.discountType},${ride.baseFare.toFixed(2)},${ride.finalFare.toFixed(2)},${trip.baseTotal.toFixed(2)},${trip.paidTotal.toFixed(2)},${trip.discountTotal.toFixed(2)}`
      );
    });
  });

  res.setHeader("Content-Type", "text/csv");
  res.setHeader("Content-Disposition", "attachment; filename=trips.csv");
  res.send(rows.join("\n"));
});

app.get("/api/summary", (req, res) => {
  const trips = readTrips();
  const summary = {
    trips: trips.length,
    baseTotal: 0,
    paidTotal: 0,
    discountTotal: 0,
    byLine: {
      "LRT-1": { base: 0, paid: 0, discount: 0 },
      "LRT-2": { base: 0, paid: 0, discount: 0 },
      "MRT-3": { base: 0, paid: 0, discount: 0 }
    },
    byDiscount: {
      NONE: { base: 0, paid: 0, discount: 0 },
      STUDENT: { base: 0, paid: 0, discount: 0 },
      PWD: { base: 0, paid: 0, discount: 0 },
      SENIOR: { base: 0, paid: 0, discount: 0 }
    }
  };

  trips.forEach((trip) => {
    summary.baseTotal += trip.baseTotal;
    summary.paidTotal += trip.paidTotal;
    summary.discountTotal += trip.discountTotal;

    trip.rides.forEach((ride) => {
      if (summary.byLine[ride.line]) {
        summary.byLine[ride.line].base += ride.baseFare;
        summary.byLine[ride.line].paid += ride.finalFare;
        summary.byLine[ride.line].discount += ride.baseFare - ride.finalFare;
      }

      if (ride.discountType === DISCOUNTS.NONE) {
        summary.byDiscount.NONE.base += ride.baseFare;
        summary.byDiscount.NONE.paid += ride.finalFare;
        summary.byDiscount.NONE.discount += ride.baseFare - ride.finalFare;
      } else if (ride.discountType === DISCOUNTS.STUDENT) {
        summary.byDiscount.STUDENT.base += ride.baseFare;
        summary.byDiscount.STUDENT.paid += ride.finalFare;
        summary.byDiscount.STUDENT.discount += ride.baseFare - ride.finalFare;
      } else if (ride.discountType === DISCOUNTS.PWD) {
        summary.byDiscount.PWD.base += ride.baseFare;
        summary.byDiscount.PWD.paid += ride.finalFare;
        summary.byDiscount.PWD.discount += ride.baseFare - ride.finalFare;
      } else if (ride.discountType === DISCOUNTS.SENIOR) {
        summary.byDiscount.SENIOR.base += ride.baseFare;
        summary.byDiscount.SENIOR.paid += ride.finalFare;
        summary.byDiscount.SENIOR.discount += ride.baseFare - ride.finalFare;
      }
    });
  });

  res.json(summary);
});

app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
