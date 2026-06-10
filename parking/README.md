# Parking Management System (C++ / OOP + DSA)

A single-file, menu-driven C++ console application that manages a parking lot.
It registers parking slots and vehicles, calculates parking fees on exit,
keeps a permanent (read-only) transaction history, and reports live statistics.



---

## Program Description

The system lets a parking operator:

- create parking slots (each supports one vehicle type),
- park a vehicle (the system finds a suitable free slot automatically),
- release a vehicle and charge a fee based on how long it stayed,
- view live information (slots, active vehicles, prices, history, statistics),
- update hourly prices at runtime **without** changing old bills.

All user input is validated, so the program never crashes on bad input.

---

## Compilation & Run

```bash
g++ main.cpp -o parking
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
| **Modularity** | Small, single-purpose classes: `Time`, `InputHelper`, `ParkingSlot`, `Vehicle`, `ParkingTransaction`, `PriceList`, `ParkingManagementSystem`. |

### Classes

- `Time` — stores hour/minute, compares times, formats `HH:MM`.
- `InputHelper` — static helpers for **safe** input (uses `try/catch` for numbers).
- `ParkingSlot` — one parking space (ID, supported type, zone, status).
- `Vehicle` — a currently parked vehicle (plate, type, entry time, slot).
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
  then one transaction per line (columns separated by `|`):

```
Plate      | Type        | Slot   | Entry  | Exit   | Hours | Price/hr | TotalFee
------------------------------------------------------------------------------
RAD100     | car         | S1     | 08:00  | 10:15  | 3     | 1000     | 3000
MOTO50     | motorcycle  | M1     | 09:00  | 09:14  | 1     | 500      | 500
TRK1       | truck       | T1     | 07:00  | 10:01  | 4     | 2000     | 8000
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



## Fee Calculation (Ceiling Hours)

Every started hour counts as a **full** hour:

```
HoursCharged = ceil(durationMinutes / 60)
Fee          = HoursCharged × CurrentPrice
```

| Duration | Hours charged |
|----------|---------------|
| 15 min | 1 |
| 1 h 20 min | 2 |
| 2 h 1 min | 3 |

The fee and the price used are **frozen** inside the transaction at exit time.

---

## Menu Options Explained

| # | Option | What it does |
|---|--------|--------------|
| 1 | Add Parking Slot | Create a slot (unique ID, supported type, zone). Rejects empty/duplicate IDs and invalid types. |
| 2 | Display Parking Slots | List every slot with its status. |
| 3 | Search Parking Slot | Find one slot by its ID. |
| 4 | Register Vehicle Entry | Park a vehicle; auto-assigns the first free slot of its type. Rejects empty/duplicate plates and full lots. |
| 5 | Register Vehicle Exit | Release a vehicle, compute the fee, free the slot, and save the transaction. |
| 6 | View Active Vehicles | List vehicles currently parked. |
| 7 | View Parking Prices | Show current hourly prices. |
| 8 | Update Parking Prices | Change a price (must be > 0). Old bills stay unchanged. |
| 9 | View Transaction History | Show all completed (read-only) bills. |
| 10 | Display System Statistics | Totals: slots, available, occupied, active vehicles, transactions, revenue. |
| 11 | Exit | Quit the program. |

---

## Validations Handled

- **Slots:** empty ID, duplicate ID, invalid vehicle type, empty zone.
- **Entry:** empty plate, duplicate active plate, invalid type, no slot of that type, no free slot.
- **Exit:** vehicle not found, exit time not after entry time, missing price.
- **Prices:** zero, negative, and non-numeric values rejected.
- **Menu:** non-numeric and out-of-range choices rejected (loops until valid).
- The program **never crashes** and **never enters infinite loops** on bad input.

---

## Sample Run (abbreviated)

### Adding slots
```
== Add Parking Slot ==
Enter Slot ID: A1
Allowed types: motorcycle, car, truck
Enter Vehicle Type for this slot: car
Enter Zone: North
Parking slot added successfully.
```

### Vehicle entry
```
== Register Vehicle Entry ==
Enter Plate Number: RAD123
Enter Vehicle Type: car
Enter Entry Time (HH:MM, 24-hour): 08:00
Vehicle parked successfully in slot A1.
```

### Vehicle exit
```
== Register Vehicle Exit ==
Enter Plate Number: RAD123
Enter Exit Time (HH:MM, 24-hour): 10:15
Vehicle exited successfully.
Duration: 135 minute(s)
Hours charged (ceiling): 3
Price used: 1000 FRW/hour
Total fee: 3000 FRW
```

### Price update does not change old bills
```
== Update Parking Prices ==
Enter vehicle type to update: car
Enter new price per hour (> 0): 2000
Price updated. Old transactions stay unchanged.

== Transaction History ==
Plate: RAD123 | Type: car | Slot: A1 | Entry: 08:00 | Exit: 10:15 | Hours: 3 | Price/hr: 1000 | Fee: 3000 FRW
```
The old bill still shows **1000** even though the car price is now 2000.

### Statistics
```
== System Statistics ==
Total slots          : 2
Available slots      : 2
Occupied slots       : 0
Active vehicles      : 0
Completed transactions: 1
Total revenue        : 3000 FRW
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
