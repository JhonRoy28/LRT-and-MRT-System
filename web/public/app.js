const rideList = document.getElementById("ride-list");
const addRideBtn = document.getElementById("add-ride");
const addTransferBtn = document.getElementById("add-transfer");
const transferNote = document.getElementById("transfer-note");
const form = document.getElementById("trip-form");
const receipt = document.getElementById("receipt");
const tripFeed = document.getElementById("trip-feed");
const statusEl = document.getElementById("server-status");
const tripCountEl = document.getElementById("trip-count");
const useCardEl = document.getElementById("use-card");
const balanceEl = document.getElementById("start-balance");
const errorEl = document.getElementById("form-error");
const summaryEl = document.getElementById("summary");
const transferBanner = document.getElementById("transfer-banner");
const transferBannerText = document.getElementById("transfer-banner-text");
const downloadReceiptBtn = document.getElementById("download-receipt");
const exportCsvBtn = document.getElementById("export-csv");
const fareForm = document.getElementById("fare-form");
const fareMessage = document.getElementById("fare-message");

let config = null;
let transferCandidate = null;
let lastTripId = null;

useCardEl.addEventListener("change", () => {
  balanceEl.disabled = !useCardEl.checked;
});

function createSelect(options, value) {
  const select = document.createElement("select");
  options.forEach((opt) => {
    const option = document.createElement("option");
    option.value = opt;
    option.textContent = opt;
    if (opt === value) {
      option.selected = true;
    }
    select.appendChild(option);
  });
  return select;
}

function lineOptions() {
  return Object.values(config.lines);
}

function stationOptions(line) {
  const key = line === config.lines.LRT1 ? "LRT1" : line === config.lines.LRT2 ? "LRT2" : "MRT3";
  return config.stations[key];
}

function discountOptions() {
  return [
    { label: "None", value: config.discounts.NONE },
    { label: "Student", value: config.discounts.STUDENT },
    { label: "PWD", value: config.discounts.PWD },
    { label: "Senior", value: config.discounts.SENIOR }
  ];
}

function rideCard(index, preset) {
  const card = document.createElement("div");
  card.className = "ride-card";

  const lineSelect = createSelect(lineOptions(), preset?.line || config.lines.LRT1);
  const fromSelect = createSelect(stationOptions(lineSelect.value), preset?.from || stationOptions(lineSelect.value)[0]);
  const toSelect = createSelect(stationOptions(lineSelect.value), preset?.to || stationOptions(lineSelect.value)[1]);

  lineSelect.addEventListener("change", () => {
    const stations = stationOptions(lineSelect.value);
    fromSelect.innerHTML = "";
    toSelect.innerHTML = "";
    stations.forEach((station) => {
      const opt1 = document.createElement("option");
      opt1.value = station;
      opt1.textContent = station;
      fromSelect.appendChild(opt1);

      const opt2 = document.createElement("option");
      opt2.value = station;
      opt2.textContent = station;
      toSelect.appendChild(opt2);
    });
    toSelect.selectedIndex = Math.min(1, stations.length - 1);
    updateTransfer();
  });

  fromSelect.addEventListener("change", updateTransfer);
  toSelect.addEventListener("change", updateTransfer);

  const discountSelect = document.createElement("select");
  discountOptions().forEach((item) => {
    const option = document.createElement("option");
    option.value = item.value;
    option.textContent = item.label;
    discountSelect.appendChild(option);
  });
  discountSelect.value = preset?.discountType || config.discounts.NONE;

  const removeBtn = document.createElement("button");
  removeBtn.type = "button";
  removeBtn.className = "ghost";
  removeBtn.textContent = "Remove";
  removeBtn.addEventListener("click", () => {
    card.remove();
    updateTransfer();
  });

  card.appendChild(labelWrap("Line", lineSelect));
  card.appendChild(labelWrap("From", fromSelect));
  card.appendChild(labelWrap("To", toSelect));
  card.appendChild(labelWrap("Discount", discountSelect));
  card.appendChild(removeBtn);

  return card;
}

function labelWrap(label, control) {
  const wrapper = document.createElement("label");
  wrapper.textContent = label;
  wrapper.appendChild(control);
  return wrapper;
}

function readRideData() {
  return Array.from(rideList.children).map((card) => {
    const selects = card.querySelectorAll("select");
    return {
      line: selects[0].value,
      from: selects[1].value,
      to: selects[2].value,
      discountType: Number(selects[3].value)
    };
  });
}

function validateRides(rides) {
  for (let i = 0; i < rides.length; i++) {
    if (rides[i].from === rides[i].to) {
      return "From and to stations must be different.";
    }
    if (i > 0) {
      const prev = rides[i - 1];
      if (prev.line !== rides[i].line) {
        const valid = config.transfers.some((transfer) =>
          transfer.fromLine === prev.line &&
          transfer.fromStation === prev.to &&
          transfer.toLine === rides[i].line
        );
        if (!valid) {
          return "Invalid transfer: you can only transfer at allowed stations.";
        }
      }
    }
  }
  return "";
}

function updateTransfer() {
  const rides = readRideData();
  const validationError = validateRides(rides);
  if (validationError) {
    errorEl.textContent = validationError;
  } else {
    errorEl.textContent = "";
  }

  if (rides.length === 0) {
    transferCandidate = null;
    addTransferBtn.disabled = true;
    transferNote.textContent = "No transfer available yet.";
    transferBanner.style.display = "none";
    return;
  }

  const lastRide = rides[rides.length - 1];
  const possible = config.transfers.filter(
    (transfer) => transfer.fromLine === lastRide.line && transfer.fromStation === lastRide.to
  );

  if (possible.length === 0) {
    transferCandidate = null;
    addTransferBtn.disabled = true;
    transferNote.textContent = "No transfer available yet.";
    transferBanner.style.display = "none";
    return;
  }

  transferCandidate = possible[0];
  addTransferBtn.disabled = false;
  transferNote.textContent = `Transfer available to ${transferCandidate.toLine} at ${transferCandidate.toStation}.`;
  transferBanner.style.display = "block";
  transferBannerText.textContent = `Next ride can start at ${transferCandidate.toStation} (${transferCandidate.toLine}).`;
}

addRideBtn.addEventListener("click", () => {
  if (!config) return;
  rideList.appendChild(rideCard(rideList.children.length));
  updateTransfer();
});

addTransferBtn.addEventListener("click", () => {
  if (!transferCandidate) return;
  const preset = {
    line: transferCandidate.toLine,
    from: transferCandidate.toStation,
    to: stationOptions(transferCandidate.toLine)[1] || transferCandidate.toStation,
    discountType: config.discounts.NONE
  };
  rideList.appendChild(rideCard(rideList.children.length, preset));
  updateTransfer();
});

downloadReceiptBtn.addEventListener("click", async () => {
  if (!lastTripId) return;
  const res = await fetch(`/api/trips/${lastTripId}/receipt`);
  const text = await res.text();
  const blob = new Blob([text], { type: "text/plain" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `${lastTripId}.txt`;
  link.click();
  URL.revokeObjectURL(url);
});

exportCsvBtn.addEventListener("click", () => {
  window.location.href = "/api/trips.csv";
});

async function loadConfig() {
  const res = await fetch("/api/config");
  config = await res.json();
  statusEl.textContent = "Online";
  rideList.appendChild(rideCard(0));
  updateTransfer();
  hydrateFareForm(config.fares);
}

async function loadTrips() {
  const res = await fetch("/api/trips");
  const trips = await res.json();
  tripCountEl.textContent = trips.length;
  tripFeed.innerHTML = "";

  if (trips.length === 0) {
    tripFeed.innerHTML = '<p class="empty">No trips recorded yet.</p>';
    return;
  }

  trips.slice(0, 6).forEach((trip) => {
    const card = document.createElement("div");
    card.className = "trip-card";
    card.innerHTML = `
      <strong>${trip.passengerName}</strong>
      <p>${new Date(trip.createdAt).toLocaleString()}</p>
      <p>Rides: ${trip.rides.length} | Paid: Php ${trip.paidTotal.toFixed(2)}</p>
    `;
    tripFeed.appendChild(card);
  });
}

async function loadSummary() {
  const res = await fetch("/api/summary");
  const summary = await res.json();
  summaryEl.innerHTML = `
    <div class="summary-card">
      <strong>Totals</strong>
      <p>Trips: ${summary.trips}</p>
      <p>Base: Php ${summary.baseTotal.toFixed(2)}</p>
      <p>Paid: Php ${summary.paidTotal.toFixed(2)}</p>
      <p>Discount: Php ${summary.discountTotal.toFixed(2)}</p>
    </div>
    <div class="summary-card">
      <strong>By Line</strong>
      <p>LRT-1: Php ${summary.byLine["LRT-1"].paid.toFixed(2)}</p>
      <p>LRT-2: Php ${summary.byLine["LRT-2"].paid.toFixed(2)}</p>
      <p>MRT-3: Php ${summary.byLine["MRT-3"].paid.toFixed(2)}</p>
    </div>
    <div class="summary-card">
      <strong>By Discount</strong>
      <p>None: Php ${summary.byDiscount.NONE.paid.toFixed(2)}</p>
      <p>Student: Php ${summary.byDiscount.STUDENT.paid.toFixed(2)}</p>
      <p>PWD: Php ${summary.byDiscount.PWD.paid.toFixed(2)}</p>
      <p>Senior: Php ${summary.byDiscount.SENIOR.paid.toFixed(2)}</p>
    </div>
  `;
}

function hydrateFareForm(fares) {
  document.getElementById("lrt1-short-max").value = fares.LRT1.shortMax;
  document.getElementById("lrt1-medium-max").value = fares.LRT1.mediumMax;
  document.getElementById("lrt1-short-fare").value = fares.LRT1.shortFare;
  document.getElementById("lrt1-medium-fare").value = fares.LRT1.mediumFare;
  document.getElementById("lrt1-long-fare").value = fares.LRT1.longFare;

  document.getElementById("lrt2-short-max").value = fares.LRT2.shortMax;
  document.getElementById("lrt2-medium-max").value = fares.LRT2.mediumMax;
  document.getElementById("lrt2-short-fare").value = fares.LRT2.shortFare;
  document.getElementById("lrt2-medium-fare").value = fares.LRT2.mediumFare;
  document.getElementById("lrt2-long-fare").value = fares.LRT2.longFare;

  document.getElementById("mrt3-range1").value = fares.MRT3.range1;
  document.getElementById("mrt3-range2").value = fares.MRT3.range2;
  document.getElementById("mrt3-range3").value = fares.MRT3.range3;
  document.getElementById("mrt3-range4").value = fares.MRT3.range4;
  document.getElementById("mrt3-range5").value = fares.MRT3.range5;
  document.getElementById("mrt3-fare1").value = fares.MRT3.fare1;
  document.getElementById("mrt3-fare2").value = fares.MRT3.fare2;
  document.getElementById("mrt3-fare3").value = fares.MRT3.fare3;
  document.getElementById("mrt3-fare4").value = fares.MRT3.fare4;
  document.getElementById("mrt3-fare5").value = fares.MRT3.fare5;
}

fareForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  fareMessage.textContent = "";

  const payload = {
    LRT1: {
      shortMax: Number(document.getElementById("lrt1-short-max").value),
      mediumMax: Number(document.getElementById("lrt1-medium-max").value),
      shortFare: Number(document.getElementById("lrt1-short-fare").value),
      mediumFare: Number(document.getElementById("lrt1-medium-fare").value),
      longFare: Number(document.getElementById("lrt1-long-fare").value)
    },
    LRT2: {
      shortMax: Number(document.getElementById("lrt2-short-max").value),
      mediumMax: Number(document.getElementById("lrt2-medium-max").value),
      shortFare: Number(document.getElementById("lrt2-short-fare").value),
      mediumFare: Number(document.getElementById("lrt2-medium-fare").value),
      longFare: Number(document.getElementById("lrt2-long-fare").value)
    },
    MRT3: {
      range1: Number(document.getElementById("mrt3-range1").value),
      range2: Number(document.getElementById("mrt3-range2").value),
      range3: Number(document.getElementById("mrt3-range3").value),
      range4: Number(document.getElementById("mrt3-range4").value),
      range5: Number(document.getElementById("mrt3-range5").value),
      fare1: Number(document.getElementById("mrt3-fare1").value),
      fare2: Number(document.getElementById("mrt3-fare2").value),
      fare3: Number(document.getElementById("mrt3-fare3").value),
      fare4: Number(document.getElementById("mrt3-fare4").value),
      fare5: Number(document.getElementById("mrt3-fare5").value)
    }
  };

  const res = await fetch("/api/fares", {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  });

  if (!res.ok) {
    fareMessage.textContent = "Unable to save fare rules.";
    return;
  }

  config.fares = await res.json();
  fareMessage.textContent = "Fare rules saved.";
});

function renderReceipt(trip) {
  lastTripId = trip.id;
  downloadReceiptBtn.disabled = false;
  receipt.innerHTML = `
    <h3>${trip.passengerName}</h3>
    <div class="row"><span>Trip ID</span><span>${trip.id}</span></div>
    <div class="row"><span>Date</span><span>${new Date(trip.createdAt).toLocaleString()}</span></div>
    <div class="divider"></div>
    ${trip.rides.map((ride, index) => `
      <div class="row"><span>Ride ${index + 1} (${ride.line})</span><span>Php ${ride.finalFare.toFixed(2)}</span></div>
      <div class="row"><span>${ride.from} to ${ride.to}</span><span>${ride.stations} gaps</span></div>
    `).join("")}
    <div class="divider"></div>
    <div class="row"><span>Base total</span><span>Php ${trip.baseTotal.toFixed(2)}</span></div>
    <div class="row"><span>Paid total</span><span>Php ${trip.paidTotal.toFixed(2)}</span></div>
    <div class="row"><span>Discount</span><span>Php ${trip.discountTotal.toFixed(2)}</span></div>
    ${trip.useCard ? `
      <div class="divider"></div>
      <div class="row"><span>Card start</span><span>Php ${trip.startBalance.toFixed(2)}</span></div>
      <div class="row"><span>Card end</span><span>Php ${trip.endBalance.toFixed(2)}</span></div>
      <div class="row"><span>Top-up</span><span>Php ${trip.topupTotal.toFixed(2)}</span></div>
    ` : ""}
  `;
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  errorEl.textContent = "";

  const rides = readRideData();
  const validationError = validateRides(rides);
  if (validationError) {
    errorEl.textContent = validationError;
    return;
  }
  if (rides.length === 0) {
    errorEl.textContent = "Add at least one ride.";
    return;
  }

  const payload = {
    passengerName: document.getElementById("passenger-name").value || "Passenger",
    useCard: useCardEl.checked,
    startBalance: Number(balanceEl.value || 0),
    rides
  };

  const res = await fetch("/api/trips", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  });

  const data = await res.json();
  if (!res.ok) {
    errorEl.textContent = data.error || "Unable to save trip.";
    return;
  }

  renderReceipt(data);
  await loadTrips();
  await loadSummary();
});

loadConfig().then(() => Promise.all([loadTrips(), loadSummary()])).catch(() => {
  statusEl.textContent = "Offline";
  errorEl.textContent = "Unable to connect to server.";
});
