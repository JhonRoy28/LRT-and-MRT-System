# Train Fare Ticketing Dashboard

A web-based ticketing system for Metro Manila rail lines (LRT-1, LRT-2, MRT-3). The app calculates fares, enforces transfer rules, applies discounts, and stores trips in a JSON file. It also provides a ticketing-style dashboard with receipts, summaries, and export tools.

## Features

- Ride builder with line and station selection
- Automatic fare calculation per line
- Discount handling (Student, PWD, Senior = 50% off)
- Transfer validation (only allowed transfer stations)
- Stored-value card simulation with top-up
- Trip receipt view + downloadable receipt text
- Daily summary dashboard (totals, by line, by discount)
- Fare rules editor (persisted to JSON)
- CSV export of all trips

## Lines and Transfer Rules

Supported lines:
- LRT-1 (Red line)
- LRT-2 (Purple line)
- MRT-3 (EDSA line)

Allowed transfers:
- LRT-1 Doroteo Jose -> LRT-2 Recto
- LRT-2 Recto -> LRT-1 Doroteo Jose
- LRT-1 EDSA -> MRT-3 Taft
- MRT-3 Taft -> LRT-1 EDSA
- LRT-2 Cubao -> MRT-3 Cubao
- MRT-3 Cubao -> LRT-2 Cubao

## Tech Stack

- Backend: Node.js + Express
- Frontend: HTML, CSS, vanilla JavaScript
- Storage: JSON files

## Project Structure

- server.js: API server, fare logic, transfer validation, exports
- public/: UI (HTML/CSS/JS)
- data/trips.json: stored trip data
- data/fares.json: persisted fare rules

## Setup

1. Install Node.js (LTS).
2. Open a terminal in the web folder:
   ```powershell
   cd "C:\Users\jhonr\Project 1\web"
   ```
3. Install dependencies:
   ```powershell
   npm install
   ```
4. Start the server:
   ```powershell
   npm start
   ```
5. Open the app:
   - http://localhost:3000

## Usage

1. Enter passenger name and (optional) stored-value card balance.
2. Add rides and select line, from/to stations, and discounts.
3. Use "Add transfer ride" when transfer is available.
4. Save the trip to generate a receipt and store the trip in JSON.
5. View daily totals and export CSV as needed.

## Screenshots

Add your screenshots here once captured:

- Dashboard overview: `screenshots/dashboard.png`
- Ride builder + transfer banner: `screenshots/ride-builder.png`
- Receipt + summary: `screenshots/receipt-summary.png`
- Fare rules editor: `screenshots/fare-editor.png`

Recommended: create a `screenshots/` folder inside the `web/` directory and drop images there.

## API Endpoints

- GET /api/config
  - Returns lines, stations, discounts, transfers, and fares.
- GET /api/trips
  - Returns all saved trips.
- POST /api/trips
  - Creates a trip and returns computed receipt data.
- GET /api/trips/:id
  - Returns a single trip by ID.
- GET /api/trips/:id/receipt
  - Download a plain-text receipt for the trip.
- GET /api/trips.csv
  - Download all trips as CSV.
- GET /api/summary
  - Returns summary totals (overall, by line, by discount).
- GET /api/fares
  - Returns current fare rules.
- PUT /api/fares
  - Updates fare rules (JSON payload).

## Data Files

- data/trips.json
  - Stores all trips.
- data/fares.json
  - Stores editable fare rules.

## Known Limitations

- JSON file storage is single-user and not optimized for high concurrency.
- No authentication or user accounts (single shared dashboard).
- No PDF receipts (text download only).

## Future Improvements

- Authentication and multi-user accounts
- Printable/PDF receipt rendering
- Admin roles for fare rule changes
- Database storage (SQLite/PostgreSQL)

## LinkedIn Summary (Optional)

"Built a web-based ticketing dashboard for LRT-1, LRT-2, and MRT-3. The app calculates fares, validates transfers, applies discounts, simulates stored-value cards, and exports trip data to CSV. Stack: Node.js + Express + vanilla JS with JSON persistence."
