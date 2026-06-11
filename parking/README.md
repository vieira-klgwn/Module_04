# Parking Management System (C++ / OOP + DSA)

A single-file, menu-driven C++ console application that manages a parking lot.
It registers parking slots and vehicles, **captures entry and exit times
automatically from the system clock**, calculates parking fees on exit
(supporting stays that span several days), keeps a permanent (read-only)
transaction history, and reports live statistics. All identifiers are
**strictly validated with regular expressions**.



---

## Program Description

The system lets a parking operator:

- create parking slots (each supports one vehicle type),
- park a vehicle (the system finds a suitable free slot automatically and
  **stamps the current date & time on entry — no manual time entry**),
- release a vehicle (the **current date & time is stamped automatically on
  exit**) and charge a fee based on how long it stayed, even across days,
- view live information (slots, active vehicles, prices, history, statistics),
- update hourly prices at runtime **without** changing old bills.

All user input is validated, so the program never crashes on bad input.
Slot IDs, zones and plate numbers must match strict formats (see
**Input Formats & Validations** below).

---

## Compilation & Run

The program uses `<regex>` and `<ctime>`, so compile with C++11 (or newer):

```bash
g++ -std=c++11 main.cpp -o parking
./parking
```



---

## OOP Principles Used

| Principle | Where it is applied |
|-----------|---------------------|
| **Encapsulation** | Every class keeps its data `private` and exposes only safe getters/setters (e.g. `ParkingSlot::setOccupied`, `Vehicle::getPlateNumber`). |
| **Constructors** | Each class builds a valid object in one step (`ParkingSlot(id, type, zone)`, `Vehicle(...)`, `ParkingTransaction(...)`). |
| **Abstraction** | `ParkingManagementSystem` hides all the vectors and logic behind simple named actions like `registerVehicleEntry()`. `main()` never touches the data directly. |
| **Immutability** | `ParkingTransaction` has **only getters** — once created, a bill can never change. This is why price updates never affect old records. |
| **Modularity** | Small, single-purpose classes: `DateTime`, `InputHelper`, `ParkingSlot`, `Vehicle`, `ParkingTransaction`, `PriceList`, `ParkingManagementSystem`. |

### Classes

- `DateTime` — wraps a full calendar timestamp (`time_t`); provides
  `DateTime::now()` (reads the system clock), `secondsUntil()` (duration that
  can span days), `toString()` → `YYYY-MM-DD HH:MM:SS`, and `fromString()`
  (used when reloading saved transactions).
- `InputHelper` — static helpers for **safe** input (uses `try/catch` for
  numbers and `regex_match` for formatted fields).
- `ParkingSlot` — one parking space (ID, supported type, zone, status).
- `Vehicle` — a currently parked vehicle (plate, type, auto entry time, slot).
- `ParkingTransaction` — a completed, read-only bill record.
- `PriceRate` / `PriceList` — current hourly price per vehicle type (stored in a `vector`).
- `ParkingManagementSystem` — the controller that owns all data and runs the menu.

---

## Data Structures Used (DSA)

All storage uses a **linear data structure (`vector`)**

| Structure | Used for |
|-----------|----------|
| `vector<ParkingSlot>` | all parking slots |
| `vector<Vehicle>` | vehicles currently parked |
| `vector<ParkingTransaction>` | completed transactions (history) |
| `vector<PriceRate>` | current hourly price per vehicle type |

Every lookup (slot by ID, vehicle by plate, price by type) is a straightforward
linear search loop:

```cpp
for (int i = 0; i < (int)collection.size(); i++) { ... }
```

---

## File Persistence (Transactions Saved for Future Reference)

Completed transactions are stored in a plain text file so they survive after
the program closes.

- **File:** `transactions.txt` (created in the same folder as the program).
- **When it saves:** automatically after every vehicle exit.
- **When it loads:** automatically when the program starts, so past
  transactions and total revenue are restored.
- **Format:** a neat table with a **column header row**, a dashed separator,
  then one transaction per line (columns separated by `|`). The **Entry** and
  **Exit** columns now store the full automatic timestamp
  `YYYY-MM-DD HH:MM:SS`:

```
Plate      | Type        | Slot   | Entry               | Exit                | Hours | Price/hr | TotalFee
--------------------------------------------------------------------------------------------------------
RAB123A    | car         | SLOT-1 | 2026-06-11 08:00:00 | 2026-06-11 10:15:00 | 3     | 1000     | 3000
RA123      | motorcycle  | SLOT-2 | 2026-06-11 09:00:00 | 2026-06-11 09:14:00 | 1     | 500      | 500
RTK789X    | truck       | SLOT-3 | 2026-06-10 07:00:00 | 2026-06-11 10:01:00 | 28    | 2000     | 56000
```

When loading, the header row and the dashed separator are skipped automatically
(they fail the numeric checks inside a `try/catch`), so a bad or partial file
never crashes the program. This uses the same `saveToFile` / `loadFromFile` +
`splitLine` technique taught in `fileHandling/fileHandlingTutorial.cpp`.

> Tip: delete `transactions.txt` if you ever want to reset the history to empty.

---

## Default Prices

| Vehicle type | Price |
|--------------|-------|
| Motorcycle | 500 FRW/hour |
| Car | 1000 FRW/hour |
| Truck | 2000 FRW/hour |



## Automatic Date & Time (No Manual Entry)

Entry and exit times are **captured automatically** from the system clock using
the C++ `<ctime>` library (`DateTime::now()` wraps `time(nullptr)`), similar to
a `dateTime.now()` call in other languages.

- On **entry**, the current date & time is stored with the vehicle.
- On **exit**, the current date & time is stamped, and the duration is the
  number of seconds between the two timestamps.

Because full timestamps (date **and** time) are stored, a vehicle can stay
parked **across midnight or for several days** and still be billed correctly —
something that was impossible with the old "type the time manually" approach
(you could not express "in yesterday 06:30, out today 04:30").

## Fee Calculation (Ceiling Hours, Multi-Day Aware)

Every **started** hour counts as a full hour, and a completed session is always
charged **at least one hour**:

```
durationSeconds = exitTimestamp - entryTimestamp     (can span many days)
HoursCharged    = max(1, ceil(durationSeconds / 3600))
Fee             = HoursCharged × CurrentPrice
```

| Duration | Hours charged |
|----------|---------------|
| a few seconds | 1 |
| 15 min | 1 |
| 1 h 20 min | 2 |
| 2 h 1 min | 3 |
| 1 day 0 h 0 min | 24 |
| 1 day 22 h | 46 |

The fee and the price used are **frozen** inside the transaction at exit time.

---

## Menu Options Explained

| # | Option | What it does |
|---|--------|--------------|
| 1 | Add Parking Slot | Create a slot (`SLOT-<n>` ID, supported type, ≤10-char zone). Rejects bad-format/empty/duplicate IDs, bad zones, and invalid types. |
| 2 | Display Parking Slots | List every slot with its status. |
| 3 | Search Parking Slot | Find one slot by its ID. |
| 4 | Register Vehicle Entry | Park a vehicle; validates the plate against the type's format, auto-assigns the first free slot, and **records the entry date/time automatically**. Rejects bad-format/empty/duplicate plates and full lots. |
| 5 | Register Vehicle Exit | Release a vehicle; **stamps the exit date/time automatically**, computes the (multi-day) fee, frees the slot, and saves the transaction. |
| 6 | View Active Vehicles | List vehicles currently parked. |
| 7 | View Parking Prices | Show current hourly prices. |
| 8 | Update Parking Prices | Change a price (must be > 0). Old bills stay unchanged. |
| 9 | View Transaction History | Show all completed (read-only) bills. |
| 10 | Display System Statistics | Totals: slots, available, occupied, active vehicles, transactions, revenue. |
| 11 | Exit | Quit the program. |

---

## Input Formats & Validations (Regex-Enforced)

Every formatted field is checked with a regular expression and the prompt tells
the user exactly what to type. Letters must be **UPPERCASE**, and **no spaces or
symbols** are allowed inside any field.

| Field | Rule | Regex | Valid | Invalid |
|-------|------|-------|-------|---------|
| **Slot ID** | `SLOT-` + a positive number | `^SLOT-[1-9][0-9]*$` | `SLOT-1`, `SLOT-12` | `slot1`, `SLOT_1`, `S1`, `SLOT-0` |
| **Zone** | 1–10 letters/digits only | `^[A-Za-z0-9]{1,10}$` | `North`, `Zone1` | `North!`, `(empty)`, `MoreThan10Chars` |
| **Plate (car / truck)** | 7 chars: `R` + 2 letters + 3 digits + 1 letter | `^R[A-Z]{2}[0-9]{3}[A-Z]$` | `RAB123A` | `rab123a`, `RAB123`, `RAB1234` |
| **Plate (motorcycle)** | 5 chars: `R` + 1 letter + 3 digits | `^R[A-Z][0-9]{3}$` | `RA123` | `RA12`, `RAB123`, `ra123` |

> The plate format is chosen by vehicle type: standard **cars** and **trucks**
> share the 7-character pattern (e.g. `RAB123A`), while **motorcycles** use the
> shorter 5-character pattern (e.g. `RA123`).

## Validations Handled

- **Slot ID:** must match `SLOT-<number>`; empty and duplicate IDs rejected.
- **Zone:** must be 1–10 alphanumeric characters; spaces/symbols/empty rejected.
- **Plate number:** must match the regex for the chosen vehicle type
  (UPPERCASE only, no spaces or symbols); empty and duplicate active plates rejected.
- **Entry:** invalid type, no slot of that type, no free slot.
- **Exit:** vehicle not found, missing price (exit time is automatic, so it can
  never be before entry under a healthy clock).
- **Prices:** zero, negative, and non-numeric values rejected.
- **Menu:** non-numeric and out-of-range choices rejected (loops until valid).
- The program **never crashes** and **never enters infinite loops** on bad input.

---

## Sample Run (abbreviated)

### Adding slots
```
== Add Parking Slot ==
Slot ID format: SLOT-<number> in UPPERCASE, e.g. SLOT-1, SLOT-2 (no spaces or symbols).
Enter Slot ID: SLOT-1
Allowed types: motorcycle, car, truck
Enter Vehicle Type for this slot: car
Zone format: 1 to 10 letters/digits only (no spaces or symbols).
Enter Zone: North
Parking slot added successfully.
```

### Vehicle entry (time captured automatically)
```
== Register Vehicle Entry ==
Allowed types: motorcycle, car, truck
Enter Vehicle Type: car
Plate format for car: R + 2 letters + 3 digits + 1 letter, e.g. RAB123A (7 UPPERCASE chars).
Enter Plate Number: RAB123A
Vehicle parked successfully in slot SLOT-1.
Entry time recorded automatically: 2026-06-11 08:00:00
```

### Vehicle exit (time captured automatically, can span days)
```
== Register Vehicle Exit ==
Enter Plate Number: RAB123A
Vehicle exited successfully.
Entry time: 2026-06-11 08:00:00
Exit time : 2026-06-11 10:15:00
Duration  : 0 day(s), 2 hour(s), 15 minute(s)
Hours charged (ceiling): 3
Price used: 1000 FRW/hour
Total fee : 3000 FRW
```

### Price update does not change old bills
```
== Update Parking Prices ==
Allowed types: motorcycle, car, truck
Enter vehicle type to update: car
Enter new price per hour (> 0): 2000
Price updated. Old transactions stay unchanged.

== Transaction History ==
Plate      | Type        | Slot   | Entry               | Exit                | Hours | Price/hr | TotalFee
--------------------------------------------------------------------------------------------------------
RAB123A    | car         | SLOT-1 | 2026-06-11 08:00:00 | 2026-06-11 10:15:00 | 3     | 1000     | 3000 FRW
```
The old bill still shows **1000** even though the car price is now 2000.

### Statistics
```
== System Statistics ==
Total slots           : 2
Available slots       : 2
Occupied slots        : 0
Active vehicles       : 0
Completed transactions: 1
Total revenue         : 3000 FRW
```

---

## File Structure

```
parking/
├── main.cpp              # complete single-file program
└── README.md             # this file
└── System Diagram.png    # for this Parking Management System 
└── transactions.txt      # File keeping the transactions 

```
