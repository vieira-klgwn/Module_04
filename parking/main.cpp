/*
==============================================================================
  PARKING MANAGEMENT SYSTEM  (OOP + Linear DSA + Input Validation)
  Single-file C++ console application.

  OOP PRINCIPLES USED
  -------------------
  - Encapsulation : every class keeps its data PRIVATE and exposes
                    safe public getters / setters / member functions.
  - Constructors  : each class builds a valid object in one step.
  - Abstraction   : ParkingManagementSystem hides all the data structures
                    and logic behind simple, named menu actions.
  - Modularity    : small focused classes, each with one job.

  DATA STRUCTURES USED (all LINEAR -> vector)
  -------------------------------------------
  - vector<ParkingSlot>          : all parking slots.
  - vector<Vehicle>              : vehicles currently parked.
  - vector<ParkingTransaction>   : completed (read-only) transactions.
  - vector<PriceRate>            : current hourly price per vehicle type.
  Searching is done with simple linear-search for loops.

  COMPILE : g++ main.cpp -o parking
  RUN     : ./parking
==============================================================================
*/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <cstdlib>   // exit() for safe EOF handling
#include <fstream>   // ofstream / ifstream for saving transactions to a file
#include <ctime>     // time(), localtime(), mktime() for automatic date/time
#include <cstring>   // memset() when building a tm struct
#include <regex>     // regular-expression validation of IDs / plates / zones

using namespace std;

// ===========================================================================
// CLASS: DateTime
// Stores a FULL calendar timestamp (date + time) as a time_t.
// The entry/exit times are now captured AUTOMATICALLY from the system clock
// (DateTime::now()), so a vehicle can stay parked across midnight / many days
// and the duration is still computed correctly.
// Encapsulation: the raw timestamp is private; only safe methods are exposed.
// ===========================================================================
class DateTime {
private:
    time_t timestamp;   // seconds since the epoch (local system clock)

public:
    DateTime() : timestamp(0) {}
    explicit DateTime(time_t t) : timestamp(t) {}

    // Capture the current real-world date and time from the system clock.
    static DateTime now() {
        return DateTime(time(nullptr));
    }

    time_t getTimestamp() const { return timestamp; }

    // Whole seconds between this moment and a later one (other - this).
    // May span hours, days or more, which is exactly what we need.
    long long secondsUntil(const DateTime& other) const {
        return (long long)difftime(other.timestamp, timestamp);
    }

    // Human-readable "YYYY-MM-DD HH:MM:SS".
    string toString() const {
        char buffer[20];
        struct tm* lt = localtime(&timestamp);
        if (lt == nullptr) return "0000-00-00 00:00:00";
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", lt);
        return string(buffer);
    }

    // Rebuild a DateTime from "YYYY-MM-DD HH:MM:SS" (used when loading the
    // saved transactions file). Throws if the text is not a valid timestamp,
    // which lets the loader skip header / separator / damaged rows safely.
    static DateTime fromString(const string& text) {
        string tmp = text;
        for (int i = 0; i < (int)tmp.size(); i++) {
            if (tmp[i] == '-' || tmp[i] == ':') tmp[i] = ' ';
        }
        stringstream ss(tmp);
        int y, mo, d, h, mi, s;
        if (!(ss >> y >> mo >> d >> h >> mi >> s)) {
            throw runtime_error("bad datetime");
        }
        struct tm tmv;
        memset(&tmv, 0, sizeof(tmv));
        tmv.tm_year  = y - 1900;
        tmv.tm_mon   = mo - 1;
        tmv.tm_mday  = d;
        tmv.tm_hour  = h;
        tmv.tm_min   = mi;
        tmv.tm_sec   = s;
        tmv.tm_isdst = -1;            // let the system work out daylight saving
        time_t t = mktime(&tmv);
        if (t == (time_t)-1) throw runtime_error("bad datetime");
        return DateTime(t);
    }
};

// ===========================================================================
// CLASS: InputHelper
// A small toolbox of STATIC functions for SAFE user input.
// Every method loops until the user gives valid data, so the
// program NEVER crashes on bad input.
// ===========================================================================
class InputHelper {
public:
    // Remove spaces from the start and end of a string
    static string trim(const string& text) {
        int start = 0;
        int end = (int)text.size() - 1;
        while (start <= end && isspace((unsigned char)text[start])) start++;
        while (end >= start && isspace((unsigned char)text[end])) end--;
        if (start > end) return "";
        return text.substr(start, end - start + 1);
    }

    // Make a string lowercase (so "Car", "CAR", "car" all work the same)
    static string toLower(const string& text) {
        string result = text;
        for (int i = 0; i < (int)result.size(); i++) {
            result[i] = tolower((unsigned char)result[i]);
        }
        return result;
    }

    // Read one trimmed line of text.
    // If the input stream ends (EOF) we exit cleanly instead of looping
    // forever, which keeps the "never enter infinite loops" guarantee.
    static string readLine(const string& prompt) {
        string value;
        cout << prompt;
        if (!getline(cin, value)) {
            cout << "\nInput ended. Exiting program safely.\n";
            exit(0);
        }
        return trim(value);
    }

    // Read a line that must NOT be empty (keeps asking until valid)
    static string readNonEmpty(const string& prompt) {
        while (true) {
            string value = readLine(prompt);
            if (!value.empty()) return value;
            cout << "Error: This field cannot be empty. Please try again.\n";
        }
    }

    // Read an integer inside [minValue, maxValue].
    // Uses try-catch to handle non-numeric input safely.
    static int readIntInRange(const string& prompt, int minValue, int maxValue) {
        while (true) {
            string line = readLine(prompt);
            try {
                size_t pos;
                int value = stoi(line, &pos);   // may throw if not a number
                if (pos != line.size()) {
                    throw invalid_argument("extra characters");
                }
                if (value < minValue || value > maxValue) {
                    cout << "Error: Please enter a number between "
                         << minValue << " and " << maxValue << ".\n";
                    continue;
                }
                return value;
            } catch (const exception&) {
                cout << "Error: Invalid number. Please try again.\n";
            }
        }
    }

    // Read a price that must be strictly greater than zero.
    static double readPositiveDouble(const string& prompt) {
        while (true) {
            string line = readLine(prompt);
            try {
                size_t pos;
                double value = stod(line, &pos);   // may throw
                if (pos != line.size()) {
                    throw invalid_argument("extra characters");
                }
                if (value <= 0) {
                    cout << "Error: Price must be greater than zero.\n";
                    continue;
                }
                return value;
            } catch (const exception&) {
                cout << "Error: Invalid price. Please enter a positive number.\n";
            }
        }
    }

    // Does a string fully match a regular expression?
    // Wrapped in try/catch so a bad pattern can never crash the program.
    static bool matchesRegex(const string& value, const string& pattern) {
        try {
            return regex_match(value, regex(pattern));
        } catch (const exception&) {
            return false;
        }
    }

    // Read a NON-EMPTY value that must fully match a regular expression.
    // Keeps re-asking and shows the guidance message until the input is valid.
    static string readPattern(const string& prompt,
                              const string& pattern,
                              const string& errorMessage) {
        while (true) {
            string value = readNonEmpty(prompt);
            if (matchesRegex(value, pattern)) return value;
            cout << "Error: " << errorMessage << "\n";
        }
    }
};

// ===========================================================================
// CLASS: ParkingSlot
// One physical parking space. Encapsulated: data is private.
// ===========================================================================
class ParkingSlot {
private:
    string slotId;        // unique ID
    string vehicleType;   // supported type: motorcycle / car / truck
    string zone;          // zone label, e.g. "A", "North"
    bool occupied;        // true = a vehicle is parked here

public:
    ParkingSlot() : occupied(false) {}

    ParkingSlot(string id, string type, string z)
        : slotId(id), vehicleType(type), zone(z), occupied(false) {}

    string getSlotId() const { return slotId; }
    string getVehicleType() const { return vehicleType; }
    string getZone() const { return zone; }
    bool isOccupied() const { return occupied; }

    void setOccupied(bool value) { occupied = value; }

    void display() const {
        cout << "Slot ID: " << slotId
             << " | Type: " << vehicleType
             << " | Zone: " << zone
             << " | Status: " << (occupied ? "Occupied" : "Available")
             << endl;
    }
};

// ===========================================================================
// CLASS: Vehicle
// A vehicle that is CURRENTLY parked.
// ===========================================================================
class Vehicle {
private:
    string plateNumber;       // unique while parked
    string vehicleType;
    DateTime entryTime;       // captured automatically at entry
    string allocatedSlotId;   // which slot it occupies

public:
    Vehicle() {}

    Vehicle(string plate, string type, DateTime entry, string slotId)
        : plateNumber(plate), vehicleType(type),
          entryTime(entry), allocatedSlotId(slotId) {}

    string getPlateNumber() const { return plateNumber; }
    string getVehicleType() const { return vehicleType; }
    DateTime getEntryTime() const { return entryTime; }
    string getAllocatedSlotId() const { return allocatedSlotId; }

    void display() const {
        cout << "Plate: " << plateNumber
             << " | Type: " << vehicleType
             << " | Slot: " << allocatedSlotId
             << " | Entry: " << entryTime.toString()
             << endl;
    }
};

// ===========================================================================
// CLASS: ParkingTransaction
// A COMPLETED parking record. It is immutable: once created it never
// changes (no setters). This is why old records keep their old price
// even after prices are updated.
// ===========================================================================
class ParkingTransaction {
private:
    string plateNumber;
    string vehicleType;
    string slotId;
    DateTime entryTime;
    DateTime exitTime;
    int hoursCharged;
    double priceUsed;     // hourly price at the moment of exit (frozen)
    double totalFee;

public:
    ParkingTransaction() : hoursCharged(0), priceUsed(0), totalFee(0) {}

    ParkingTransaction(string plate, string type, string slot,
                       DateTime entry, DateTime exit, int hours,
                       double price, double fee)
        : plateNumber(plate), vehicleType(type), slotId(slot),
          entryTime(entry), exitTime(exit), hoursCharged(hours),
          priceUsed(price), totalFee(fee) {}

    // Only getters -> the record can be read but never modified
    string getPlateNumber() const { return plateNumber; }
    string getVehicleType() const { return vehicleType; }
    string getSlotId() const { return slotId; }
    DateTime getEntryTime() const { return entryTime; }
    DateTime getExitTime() const { return exitTime; }
    int getHoursCharged() const { return hoursCharged; }
    double getPriceUsed() const { return priceUsed; }
    double getTotalFee() const { return totalFee; }

    void display() const {
        cout << left
             << setw(10) << plateNumber << " | "
             << setw(11) << vehicleType << " | "
             << setw(6)  << slotId << " | "
             << setw(19) << entryTime.toString() << " | "
             << setw(19) << exitTime.toString() << " | "
             << setw(5)  << hoursCharged << " | "
             << setw(8)  << priceUsed << " | "
             << totalFee << " FRW"
             << endl;
    }
};

// ===========================================================================
// STRUCT: PriceRate  (one vehicle type and its current price)
// CLASS:  PriceList  (a LINEAR vector of price rates)
// Kept separate so prices can change without touching old transactions.
// ===========================================================================
struct PriceRate {
    string vehicleType;
    double hourlyRate;
    PriceRate() : hourlyRate(0) {}
    PriceRate(string type, double rate) : vehicleType(type), hourlyRate(rate) {}
};

class PriceList {
private:
    vector<PriceRate> rates;  // linear data structure

    int findIndex(const string& type) const {
        for (int i = 0; i < (int)rates.size(); i++) {
            if (rates[i].vehicleType == type) return i;
        }
        return -1;
    }

public:
    // Default starting tariffs required by the project
    PriceList() {
        rates.push_back(PriceRate("motorcycle", 500));
        rates.push_back(PriceRate("car", 1000));
        rates.push_back(PriceRate("truck", 2000));
    }

    // Returns -1 if the type or price is not valid
    double getPrice(const string& type) const {
        int idx = findIndex(type);
        if (idx == -1) return -1;
        if (rates[idx].hourlyRate <= 0) return -1;
        return rates[idx].hourlyRate;
    }

    void setPrice(const string& type, double newPrice) {
        int idx = findIndex(type);
        if (idx == -1) {
            rates.push_back(PriceRate(type, newPrice));
        } else {
            rates[idx].hourlyRate = newPrice;
        }
    }

    void display() const {
        cout << "\n--- Current Hourly Prices ---\n";
        for (int i = 0; i < (int)rates.size(); i++) {
            cout << rates[i].vehicleType << ": "
                 << rates[i].hourlyRate << " FRW/hour\n";
        }
    }
};

// ===========================================================================
// CLASS: ParkingManagementSystem
// The CONTROLLER. It owns all the (linear) data structures and provides
// the high-level operations behind each menu choice.
// ===========================================================================
class ParkingManagementSystem {
private:
    vector<ParkingSlot> slots;                  // linear: all slots
    vector<Vehicle> activeVehicles;             // linear: parked vehicles
    vector<ParkingTransaction> transactions;    // linear: history
    PriceList priceList;

    const string transactionFile = "transactions.txt";

    bool isValidVehicleType(const string& type) const {
        return type == "motorcycle" || type == "car" || type == "truck";
    }

    // Ask the user for a valid vehicle type (loops until valid)
    string readVehicleType(const string& prompt) {
        while (true) {
            cout << "Allowed types: motorcycle, car, truck\n";
            string type = InputHelper::toLower(InputHelper::readNonEmpty(prompt));
            if (isValidVehicleType(type)) return type;
            cout << "Error: Invalid vehicle type. Try again.\n";
        }
    }

    // Plate format depends on the vehicle type:
    //   - motorcycle : R + 1 letter + 3 digits           (5 chars, e.g. RA123)
    //   - car/truck  : R + 2 letters + 3 digits + 1 letter (7 chars, e.g. RAB123A)
    // Letters must be UPPERCASE; no spaces or symbols are accepted.
    bool isValidPlateForType(const string& plate, const string& type) const {
        if (type == "motorcycle") {
            return InputHelper::matchesRegex(plate, "^R[A-Z][0-9]{3}$");
        }
        // car or truck
        return InputHelper::matchesRegex(plate, "^R[A-Z]{2}[0-9]{3}[A-Z]$");
    }

    // Read a plate number, re-prompting until it matches the type's format.
    string readPlateForType(const string& type) {
        string rule = (type == "motorcycle")
            ? "Plate format for motorcycle: R + 1 letter + 3 digits, e.g. RA123 (5 UPPERCASE chars)."
            : "Plate format for " + type +
              ": R + 2 letters + 3 digits + 1 letter, e.g. RAB123A (7 UPPERCASE chars).";
        while (true) {
            cout << rule << "\n";
            string plate = InputHelper::readNonEmpty("Enter Plate Number: ");
            if (isValidPlateForType(plate, type)) return plate;
            cout << "Error: Invalid plate. Use UPPERCASE letters/digits only, "
                    "no spaces or symbols, and match the format shown above.\n";
        }
    }

    // LINEAR SEARCH: slot index by ID (-1 if missing)
    int findSlotIndexById(const string& slotId) const {
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].getSlotId() == slotId) return i;
        }
        return -1;
    }

    // LINEAR SEARCH: active vehicle index by plate (-1 if missing)
    int findActiveVehicleIndexByPlate(const string& plate) const {
        for (int i = 0; i < (int)activeVehicles.size(); i++) {
            if (activeVehicles[i].getPlateNumber() == plate) return i;
        }
        return -1;
    }

    // First AVAILABLE slot that supports this vehicle type (-1 none)
    int findAvailableSlotForType(const string& type) const {
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].getVehicleType() == type && !slots[i].isOccupied()) {
                return i;
            }
        }
        return -1;
    }

    // Does any slot (free or not) support this type?
    bool anySlotSupportsType(const string& type) const {
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].getVehicleType() == type) return true;
        }
        return false;
    }

    // Ceiling billing: every STARTED hour counts as a full hour.
    // Works on whole seconds so multi-day stays are handled correctly.
    // A completed session is always charged at least one hour.
    int calculateHoursCharged(long long durationSeconds) const {
        if (durationSeconds <= 0) return 1;
        int hours = (int)ceil(durationSeconds / 3600.0);
        return hours < 1 ? 1 : hours;
    }

    // -------------------------------------------------------------------
    // FILE HELPER: split a line into pieces using a delimiter.
    // -------------------------------------------------------------------
    vector<string> splitLine(string line, const string& delimiter) const {
        vector<string> tokens;
        size_t pos = 0;
        while ((pos = line.find(delimiter)) != string::npos) {
            tokens.push_back(line.substr(0, pos));
            line.erase(0, pos + delimiter.length());
        }
        tokens.push_back(line);
        return tokens;
    }

    // -------------------------------------------------------------------
    // SAVE: write ALL transactions to the file as a NEAT TABLE with a
    // header row and aligned columns. Called after every exit.
    // -------------------------------------------------------------------
    void saveTransactionsToFile() const {
        ofstream file(transactionFile.c_str());
        if (!file.is_open()) {
            cout << "Warning: could not open " << transactionFile << " for saving.\n";
            return;
        }

        // Column header row
        file << left
             << setw(10) << "Plate" << " | "
             << setw(11) << "Type" << " | "
             << setw(6)  << "Slot" << " | "
             << setw(19) << "Entry" << " | "
             << setw(19) << "Exit" << " | "
             << setw(5)  << "Hours" << " | "
             << setw(8)  << "Price/hr" << " | "
             << setw(10) << "TotalFee" << "\n";

        // Separator line of dashes
        file << string(104, '-') << "\n";

        // One data row per transaction
        for (int i = 0; i < (int)transactions.size(); i++) {
            const ParkingTransaction& t = transactions[i];
            file << left
                 << setw(10) << t.getPlateNumber() << " | "
                 << setw(11) << t.getVehicleType() << " | "
                 << setw(6)  << t.getSlotId() << " | "
                 << setw(19) << t.getEntryTime().toString() << " | "
                 << setw(19) << t.getExitTime().toString() << " | "
                 << setw(5)  << t.getHoursCharged() << " | "
                 << setw(8)  << t.getPriceUsed() << " | "
                 << setw(10) << t.getTotalFee() << "\n";
        }
        file.close();
    }

    // -------------------------------------------------------------------
    // LOAD: read past transactions when the program starts.
    // The header row and the dashed separator are skipped automatically
    // because they fail the number checks inside the try-catch.
    // -------------------------------------------------------------------
    void loadTransactionsFromFile() {
        ifstream file(transactionFile.c_str());
        if (!file.is_open()) return;   // first run, no file yet

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;

            vector<string> parts = splitLine(line, "|");
            if (parts.size() != 8) continue;   // skips separator line

            for (int i = 0; i < (int)parts.size(); i++) {
                parts[i] = InputHelper::trim(parts[i]);
            }

            try {
                string plate = parts[0];
                string type  = parts[1];
                string slot  = parts[2];
                DateTime entry = DateTime::fromString(parts[3]);  // throws on header
                DateTime exit  = DateTime::fromString(parts[4]);
                int hours    = stoi(parts[5]);
                double price = stod(parts[6]);
                double fee   = stod(parts[7]);

                transactions.push_back(ParkingTransaction(
                    plate, type, slot, entry, exit, hours, price, fee));
            } catch (const exception&) {
                continue;   // header / separator / damaged row -> skip safely
            }
        }
        file.close();
    }

    // Print the table header used by on-screen transaction history
    void printTransactionHeader() const {
        cout << left
             << setw(10) << "Plate" << " | "
             << setw(11) << "Type" << " | "
             << setw(6)  << "Slot" << " | "
             << setw(19) << "Entry" << " | "
             << setw(19) << "Exit" << " | "
             << setw(5)  << "Hours" << " | "
             << setw(8)  << "Price/hr" << " | "
             << "TotalFee" << "\n";
        cout << string(104, '-') << "\n";
    }

public:
    // Load saved transactions at startup so history is kept across runs.
    ParkingManagementSystem() {
        loadTransactionsFromFile();
        if (!transactions.empty()) {
            cout << "Loaded " << transactions.size()
                 << " saved transaction(s) from " << transactionFile << ".\n";
        }
    }

    // MENU 1: Add Parking Slot
    void addParkingSlot() {
        cout << "\n== Add Parking Slot ==\n";
        cout << "Slot ID format: SLOT-<number> in UPPERCASE, e.g. SLOT-1, SLOT-2 "
                "(no spaces or symbols).\n";
        string slotId = InputHelper::readPattern(
            "Enter Slot ID: ",
            "^SLOT-[1-9][0-9]*$",
            "Invalid Slot ID. Use SLOT-<number> in UPPERCASE, e.g. SLOT-1 "
            "(no spaces or other symbols).");

        if (findSlotIndexById(slotId) != -1) {
            cout << "Error: Slot ID already exists. Duplicates not allowed.\n";
            return;
        }

        string type = readVehicleType("Enter Vehicle Type for this slot: ");

        cout << "Zone format: 1 to 10 letters/digits only (no spaces or symbols).\n";
        string zone = InputHelper::readPattern(
            "Enter Zone: ",
            "^[A-Za-z0-9]{1,10}$",
            "Invalid Zone. Use 1 to 10 letters or digits only "
            "(no spaces or other symbols).");

        slots.push_back(ParkingSlot(slotId, type, zone));
        cout << "Parking slot added successfully.\n";
    }

    // MENU 2: Display Parking Slots
    void displayParkingSlots() const {
        cout << "\n== All Parking Slots ==\n";
        if (slots.empty()) {
            cout << "No parking slots registered yet.\n";
            return;
        }
        for (int i = 0; i < (int)slots.size(); i++) {
            slots[i].display();
        }
    }

    // MENU 3: Search Parking Slot (by ID)
    void searchParkingSlot() const {
        cout << "\n== Search Parking Slot ==\n";
        string slotId = InputHelper::readNonEmpty("Enter Slot ID to search: ");
        int index = findSlotIndexById(slotId);
        if (index == -1) {
            cout << "Slot not found.\n";
            return;
        }
        cout << "Slot found:\n";
        slots[index].display();
    }

    // MENU 4: Register Vehicle Entry
    void registerVehicleEntry() {
        cout << "\n== Register Vehicle Entry ==\n";

        // Type is read first because the plate format depends on it.
        string type = readVehicleType("Enter Vehicle Type: ");

        if (!anySlotSupportsType(type)) {
            cout << "Sorry: No slot in this parking supports a " << type << ".\n";
            return;
        }

        int slotIndex = findAvailableSlotForType(type);
        if (slotIndex == -1) {
            cout << "Sorry: No available slot for a " << type << " right now.\n";
            return;
        }

        string plate = readPlateForType(type);

        if (findActiveVehicleIndexByPlate(plate) != -1) {
            cout << "Error: This vehicle is already parked.\n";
            return;
        }

        // Entry time is captured AUTOMATICALLY from the system clock.
        DateTime entry = DateTime::now();

        slots[slotIndex].setOccupied(true);
        string slotId = slots[slotIndex].getSlotId();
        activeVehicles.push_back(Vehicle(plate, type, entry, slotId));

        cout << "Vehicle parked successfully in slot " << slotId << ".\n";
        cout << "Entry time recorded automatically: " << entry.toString() << "\n";
    }

    // MENU 5: Register Vehicle Exit
    void registerVehicleExit() {
        cout << "\n== Register Vehicle Exit ==\n";
        string plate = InputHelper::readNonEmpty("Enter Plate Number: ");

        int vehicleIndex = findActiveVehicleIndexByPlate(plate);
        if (vehicleIndex == -1) {
            cout << "Error: Vehicle not found among parked vehicles.\n";
            return;
        }

        Vehicle vehicle = activeVehicles[vehicleIndex];

        int slotIndex = findSlotIndexById(vehicle.getAllocatedSlotId());
        if (slotIndex == -1) {
            cout << "Error: Vehicle record is invalid (slot missing).\n";
            return;
        }

        // Exit time is captured AUTOMATICALLY from the system clock.
        DateTime exitTime = DateTime::now();

        long long durationSeconds = vehicle.getEntryTime().secondsUntil(exitTime);
        if (durationSeconds < 0) {
            cout << "Error: System clock issue (exit time is before entry time).\n";
            return;
        }

        double price = priceList.getPrice(vehicle.getVehicleType());
        if (price <= 0) {
            cout << "Error: No valid price set for this vehicle type.\n";
            return;
        }

        int hoursCharged = calculateHoursCharged(durationSeconds);
        double totalFee = hoursCharged * price;

        // Create the immutable transaction record (price frozen here)
        transactions.push_back(ParkingTransaction(
            vehicle.getPlateNumber(),
            vehicle.getVehicleType(),
            vehicle.getAllocatedSlotId(),
            vehicle.getEntryTime(),
            exitTime,
            hoursCharged,
            price,
            totalFee
        ));

        // Release the slot and remove the vehicle from active records
        slots[slotIndex].setOccupied(false);
        activeVehicles.erase(activeVehicles.begin() + vehicleIndex);

        // Persist to file so the transaction is kept for future reference
        saveTransactionsToFile();

        // Break the duration down into days / hours / minutes for a clear report.
        long long days = durationSeconds / 86400;
        long long hrs  = (durationSeconds % 86400) / 3600;
        long long mins = (durationSeconds % 3600) / 60;

        cout << "Vehicle exited successfully.\n";
        cout << "Entry time: " << vehicle.getEntryTime().toString() << "\n";
        cout << "Exit time : " << exitTime.toString() << "\n";
        cout << "Duration  : " << days << " day(s), " << hrs
             << " hour(s), " << mins << " minute(s)\n";
        cout << "Hours charged (ceiling): " << hoursCharged << "\n";
        cout << "Price used: " << price << " FRW/hour\n";
        cout << "Total fee : " << totalFee << " FRW\n";
    }

    // MENU 6: View Active Vehicles
    void viewActiveVehicles() const {
        cout << "\n== Active Vehicles ==\n";
        if (activeVehicles.empty()) {
            cout << "No vehicles are currently parked.\n";
            return;
        }
        for (int i = 0; i < (int)activeVehicles.size(); i++) {
            activeVehicles[i].display();
        }
    }

    // MENU 7: View Parking Prices
    void viewPrices() const {
        priceList.display();
    }

    // MENU 8: Update Parking Prices
    void updatePrices() {
        cout << "\n== Update Parking Prices ==\n";
        string type = readVehicleType("Enter vehicle type to update: ");
        double newPrice = InputHelper::readPositiveDouble("Enter new price per hour (> 0): ");
        priceList.setPrice(type, newPrice);
        cout << "Price updated. Old transactions stay unchanged.\n";
    }

    // MENU 9: View Transaction History
    void viewTransactionHistory() const {
        cout << "\n== Transaction History ==\n";
        if (transactions.empty()) {
            cout << "No completed transactions yet.\n";
            return;
        }
        printTransactionHeader();
        for (int i = 0; i < (int)transactions.size(); i++) {
            transactions[i].display();
        }
    }

    // MENU 10: Display System Statistics
    void displayStatistics() const {
        int totalSlots = (int)slots.size();
        int occupied = 0;
        for (int i = 0; i < totalSlots; i++) {
            if (slots[i].isOccupied()) occupied++;
        }
        int available = totalSlots - occupied;

        double totalRevenue = 0;
        for (int i = 0; i < (int)transactions.size(); i++) {
            totalRevenue += transactions[i].getTotalFee();
        }

        cout << "\n== System Statistics ==\n";
        cout << "Total slots           : " << totalSlots << "\n";
        cout << "Available slots       : " << available << "\n";
        cout << "Occupied slots        : " << occupied << "\n";
        cout << "Active vehicles       : " << activeVehicles.size() << "\n";
        cout << "Completed transactions: " << transactions.size() << "\n";
        cout << "Total revenue         : " << totalRevenue << " FRW\n";
    }
};

// ===========================================================================
// Free function: print the menu
// ===========================================================================
void showMenu() {
    cout << "\n========= PARKING MANAGEMENT SYSTEM =========\n";
    cout << "1.  Add Parking Slot\n";
    cout << "2.  Display Parking Slots\n";
    cout << "3.  Search Parking Slot\n";
    cout << "4.  Register Vehicle Entry\n";
    cout << "5.  Register Vehicle Exit\n";
    cout << "6.  View Active Vehicles\n";
    cout << "7.  View Parking Prices\n";
    cout << "8.  Update Parking Prices\n";
    cout << "9.  View Transaction History\n";
    cout << "10. Display System Statistics\n";
    cout << "11. Exit\n";
}

// ===========================================================================
// main: create ONE system object and run the menu loop.
// No global variables: all data lives inside the system object.
// ===========================================================================
int main() {
    ParkingManagementSystem system;

    while (true) {
        showMenu();
        int choice = InputHelper::readIntInRange("Enter your choice (1-11): ", 1, 11);

        switch (choice) {
            case 1:  system.addParkingSlot();        break;
            case 2:  system.displayParkingSlots();   break;
            case 3:  system.searchParkingSlot();     break;
            case 4:  system.registerVehicleEntry();  break;
            case 5:  system.registerVehicleExit();   break;
            case 6:  system.viewActiveVehicles();    break;
            case 7:  system.viewPrices();            break;
            case 8:  system.updatePrices();          break;
            case 9:  system.viewTransactionHistory();break;
            case 10: system.displayStatistics();     break;
            case 11:
                cout << "Thank you for using the Parking System. Goodbye!\n";
                return 0;
        }
    }

    return 0;
}
