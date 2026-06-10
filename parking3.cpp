/*
==============================================================================
  PARKING MANAGEMENT SYSTEM  (OOP + DSA + Input Validation)
  Single-file C++ console application.

  OOP PRINCIPLES USED
  -------------------
  - Encapsulation : every class keeps its data PRIVATE and exposes
                    safe public getters / setters / member functions.
  - Constructors  : each class builds a valid object in one step.
  - Abstraction   : ParkingManagementSystem hides all the data structures
                    and logic behind simple, named menu actions.
  - Modularity    : small focused classes, each with one job.

  DATA STRUCTURES USED (DSA)
  --------------------------
  - vector<ParkingSlot>              : list of all parking slots.
  - unordered_map<string, Vehicle>   : active vehicles keyed by plate number
                                       (fast O(1) average lookup by plate).
  - vector<ParkingTransaction>       : completed (read-only) transactions.

  COMPILE : g++ main.cpp -o parking
  RUN     : ./parking
==============================================================================
*/

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <cstdlib>   // exit() for safe EOF handling
#include <fstream>   // ofstream / ifstream for saving transactions to a file

using namespace std;

// ===========================================================================
// CLASS: Time
// Stores a clock time (hour + minute) and knows how to compare itself.
// Encapsulation: hour/minute are private; we only expose safe methods.
// ===========================================================================
class Time {
private:
    int hour;
    int minute;

public:
    Time() : hour(0), minute(0) {}
    Time(int h, int m) : hour(h), minute(m) {}

    int getHour() const { return hour; }
    int getMinute() const { return minute; }

    // Convert the time to a single number of minutes (easy to compare/subtract)
    int toMinutes() const {
        return hour * 60 + minute;
    }

    // Return a nice "HH:MM" string (adds leading zeros)
    string toString() const {
        stringstream ss;
        ss << (hour < 10 ? "0" : "") << hour << ":"
           << (minute < 10 ? "0" : "") << minute;
        return ss.str();
    }
};

// ===========================================================================
// CLASS: InputHelper
// A small toolbox of STATIC functions for SAFE user input.
// Why a class? It keeps all validation logic in one named place
// instead of using loose global functions.
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

    // Make a string lowercase (used to accept "Car", "CAR", "car" the same way)
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
                    // there were extra non-numeric characters, e.g. "12abc"
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

    // Read a valid time. Accepts "HH:MM" or "HH MM".
    static Time readTime(const string& prompt) {
        while (true) {
            string line = readLine(prompt + " (HH:MM, 24-hour): ");
            // Turn ':' into a space so one parser handles both formats
            for (int i = 0; i < (int)line.size(); i++) {
                if (line[i] == ':') line[i] = ' ';
            }
            stringstream ss(line);
            int h, m;
            if (ss >> h >> m) {
                if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
                    return Time(h, m);
                }
            }
            cout << "Error: Invalid time. Hour 0-23, minute 0-59.\n";
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
    string vehicleType;   // which type it supports: motorcycle / car / truck
    string zone;          // zone label, e.g. "A", "North"
    bool occupied;        // true = a vehicle is parked here

public:
    ParkingSlot() : occupied(false) {}

    ParkingSlot(string id, string type, string z)
        : slotId(id), vehicleType(type), zone(z), occupied(false) {}

    // ----- Getters (read-only access to private data) -----
    string getSlotId() const { return slotId; }
    string getVehicleType() const { return vehicleType; }
    string getZone() const { return zone; }
    bool isOccupied() const { return occupied; }

    // ----- Setter (controlled change of state) -----
    void setOccupied(bool value) { occupied = value; }

    // Show this slot on screen
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
    Time entryTime;
    string allocatedSlotId;   // which slot it occupies

public:
    Vehicle() {}

    Vehicle(string plate, string type, Time entry, string slotId)
        : plateNumber(plate), vehicleType(type),
          entryTime(entry), allocatedSlotId(slotId) {}

    string getPlateNumber() const { return plateNumber; }
    string getVehicleType() const { return vehicleType; }
    Time getEntryTime() const { return entryTime; }
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
    Time entryTime;
    Time exitTime;
    int hoursCharged;
    double priceUsed;     // hourly price at the moment of exit (frozen)
    double totalFee;

public:
    ParkingTransaction() : hoursCharged(0), priceUsed(0), totalFee(0) {}

    ParkingTransaction(string plate, string type, string slot,
                       Time entry, Time exit, int hours,
                       double price, double fee)
        : plateNumber(plate), vehicleType(type), slotId(slot),
          entryTime(entry), exitTime(exit), hoursCharged(hours),
          priceUsed(price), totalFee(fee) {}

    // Only getters -> the record can be read but never modified
    string getPlateNumber() const { return plateNumber; }
    string getVehicleType() const { return vehicleType; }
    string getSlotId() const { return slotId; }
    Time getEntryTime() const { return entryTime; }
    Time getExitTime() const { return exitTime; }
    int getHoursCharged() const { return hoursCharged; }
    double getPriceUsed() const { return priceUsed; }
    double getTotalFee() const { return totalFee; }

    void display() const {
        cout << "Plate: " << plateNumber
             << " | Type: " << vehicleType
             << " | Slot: " << slotId
             << " | Entry: " << entryTime.toString()
             << " | Exit: " << exitTime.toString()
             << " | Hours: " << hoursCharged
             << " | Price/hr: " << priceUsed
             << " | Fee: " << totalFee << " FRW"
             << endl;
    }
};

// ===========================================================================
// CLASS: PriceList
// Stores the CURRENT hourly price for each vehicle type.
// Kept separate so prices can change without touching old transactions.
// ===========================================================================
class PriceList {
private:
    unordered_map<string, double> prices;  // type -> price per hour

public:
    // Set the default prices required by the project
    PriceList() {
        prices["motorcycle"] = 500;
        prices["car"] = 1000;
        prices["truck"] = 2000;   // sensible default so trucks can also exit
    }

    bool hasType(const string& type) const {
        return prices.find(type) != prices.end();
    }

    // Returns -1 if the type or price is not valid
    double getPrice(const string& type) const {
        unordered_map<string, double>::const_iterator it = prices.find(type);
        if (it == prices.end()) return -1;
        if (it->second <= 0) return -1;
        return it->second;
    }

    void setPrice(const string& type, double newPrice) {
        prices[type] = newPrice;
    }

    void display() const {
        cout << "\n--- Current Hourly Prices ---\n";
        // Print in a fixed friendly order
        const string order[] = {"motorcycle", "car", "truck"};
        for (int i = 0; i < 3; i++) {
            unordered_map<string, double>::const_iterator it = prices.find(order[i]);
            if (it != prices.end()) {
                cout << order[i] << ": " << it->second << " FRW/hour\n";
            }
        }
    }
};

// ===========================================================================
// CLASS: ParkingManagementSystem
// The CONTROLLER. It owns all the data structures and provides the
// high-level operations behind each menu choice. The rest of the program
// (main) never touches the vectors/maps directly -> abstraction.
// ===========================================================================
class ParkingManagementSystem {
private:
    vector<ParkingSlot> slots;                     // DSA: vector of slots
    unordered_map<string, Vehicle> activeVehicles; // DSA: map plate -> vehicle
    vector<ParkingTransaction> transactions;       // DSA: vector of history
    PriceList priceList;

    // Name of the file where completed transactions are stored permanently.
    // Change this string if you want a different file name.
    const string transactionFile = "transactions.txt";

    // The only allowed vehicle types
    bool isValidVehicleType(const string& type) const {
        return type == "motorcycle" || type == "car" || type == "truck";
    }

    // -------------------------------------------------------------------
    // FILE HANDLING HELPER: split one line into pieces using a delimiter.
    // Example: "RAD9|car|S1|8|0|9|0|1|1000|1000" split by "|"
    // returns ["RAD9","car","S1","8","0","9","0","1","1000","1000"].
    // (Same technique used in fileHandling/fileHandlingTutorial.cpp.)
    // -------------------------------------------------------------------
    vector<string> splitLine(string line, const string& delimiter) const {
        vector<string> tokens;
        size_t pos = 0;
        while ((pos = line.find(delimiter)) != string::npos) {
            tokens.push_back(line.substr(0, pos));     // text before delimiter
            line.erase(0, pos + delimiter.length());   // remove it + delimiter
        }
        tokens.push_back(line);                        // last piece
        return tokens;
    }

    // -------------------------------------------------------------------
    // SAVE: write ALL completed transactions to the file.
    // Called after every exit so the file is always up to date.
    // We OVERWRITE the whole file with the current list (simple & safe).
    // One transaction = one line, fields separated by '|'.
    // -------------------------------------------------------------------
    void saveTransactionsToFile() const {
        ofstream file(transactionFile.c_str());   // open for WRITING
        if (!file.is_open()) {
            cout << "Warning: could not open " << transactionFile
                 << " for saving.\n";
            return;
        }
        for (int i = 0; i < (int)transactions.size(); i++) {
            const ParkingTransaction& t = transactions[i];
            file << t.getPlateNumber() << "|"
                 << t.getVehicleType() << "|"
                 << t.getSlotId() << "|"
                 << t.getEntryTime().getHour() << "|"
                 << t.getEntryTime().getMinute() << "|"
                 << t.getExitTime().getHour() << "|"
                 << t.getExitTime().getMinute() << "|"
                 << t.getHoursCharged() << "|"
                 << t.getPriceUsed() << "|"
                 << t.getTotalFee() << endl;
        }
        file.close();
    }

    // -------------------------------------------------------------------
    // LOAD: read past transactions from the file when the program starts.
    // If the file does not exist yet (first run), we simply start empty.
    // We do NOT call save here (that would rewrite the file while reading).
    // -------------------------------------------------------------------
    void loadTransactionsFromFile() {
        ifstream file(transactionFile.c_str());   // open for READING
        if (!file.is_open()) {
            // First run, no file yet. Not an error.
            return;
        }
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;            // skip blank lines
            vector<string> parts = splitLine(line, "|");
            if (parts.size() != 10) continue;      // skip damaged lines

            try {
                string plate = parts[0];
                string type  = parts[1];
                string slot  = parts[2];
                Time entry(stoi(parts[3]), stoi(parts[4]));
                Time exit(stoi(parts[5]), stoi(parts[6]));
                int hours    = stoi(parts[7]);
                double price = stod(parts[8]);
                double fee   = stod(parts[9]);

                transactions.push_back(ParkingTransaction(
                    plate, type, slot, entry, exit, hours, price, fee));
            } catch (const exception&) {
                // A line had bad numbers: skip it instead of crashing.
                continue;
            }
        }
        file.close();
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

    // LINEAR SEARCH: find the index of a slot by its ID (-1 if missing)
    int findSlotIndexById(const string& slotId) const {
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].getSlotId() == slotId) return i;
        }
        return -1;
    }

    // Find the first AVAILABLE slot that supports this vehicle type (-1 none)
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

    // Ceiling billing: every started hour counts as a full hour.
    // 15 min -> 1 hour, 1h20 -> 2 hours, 2h01 -> 3 hours.
    int calculateHoursCharged(int durationMinutes) const {
        return (int)ceil(durationMinutes / 60.0);
    }

public:
    // Constructor: when the system starts, load any transactions that were
    // saved during previous runs so the history is kept "for future reference".
    ParkingManagementSystem() {
        loadTransactionsFromFile();
        if (!transactions.empty()) {
            cout << "Loaded " << transactions.size()
                 << " saved transaction(s) from " << transactionFile << ".\n";
        }
    }

    // ----------------------------------------------------------------
    // MENU 1: Add Parking Slot
    // ----------------------------------------------------------------
    void addParkingSlot() {
        cout << "\n== Add Parking Slot ==\n";
        string slotId = InputHelper::readNonEmpty("Enter Slot ID: ");

        if (findSlotIndexById(slotId) != -1) {
            cout << "Error: Slot ID already exists. Duplicates not allowed.\n";
            return;
        }

        string type = readVehicleType("Enter Vehicle Type for this slot: ");
        string zone = InputHelper::readNonEmpty("Enter Zone: ");

        slots.push_back(ParkingSlot(slotId, type, zone));
        cout << "Parking slot added successfully.\n";
    }

    // ----------------------------------------------------------------
    // MENU 2: Display Parking Slots
    // ----------------------------------------------------------------
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

    // ----------------------------------------------------------------
    // MENU 3: Search Parking Slot (by ID)
    // ----------------------------------------------------------------
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

    // ----------------------------------------------------------------
    // MENU 4: Register Vehicle Entry
    // ----------------------------------------------------------------
    void registerVehicleEntry() {
        cout << "\n== Register Vehicle Entry ==\n";
        string plate = InputHelper::readNonEmpty("Enter Plate Number: ");

        // A vehicle cannot be parked twice at the same time
        if (activeVehicles.find(plate) != activeVehicles.end()) {
            cout << "Error: This vehicle is already parked.\n";
            return;
        }

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

        Time entry = InputHelper::readTime("Enter Entry Time");

        // Occupy the slot and store the vehicle
        slots[slotIndex].setOccupied(true);
        string slotId = slots[slotIndex].getSlotId();
        activeVehicles[plate] = Vehicle(plate, type, entry, slotId);

        cout << "Vehicle parked successfully in slot " << slotId << ".\n";
    }

    // ----------------------------------------------------------------
    // MENU 5: Register Vehicle Exit
    // ----------------------------------------------------------------
    void registerVehicleExit() {
        cout << "\n== Register Vehicle Exit ==\n";
        string plate = InputHelper::readNonEmpty("Enter Plate Number: ");

        unordered_map<string, Vehicle>::iterator it = activeVehicles.find(plate);
        if (it == activeVehicles.end()) {
            cout << "Error: Vehicle not found among parked vehicles.\n";
            return;
        }

        Vehicle vehicle = it->second;

        int slotIndex = findSlotIndexById(vehicle.getAllocatedSlotId());
        if (slotIndex == -1) {
            cout << "Error: Vehicle record is invalid (slot missing).\n";
            return;
        }

        Time exit = InputHelper::readTime("Enter Exit Time");

        int entryMinutes = vehicle.getEntryTime().toMinutes();
        int exitMinutes = exit.toMinutes();

        if (exitMinutes <= entryMinutes) {
            cout << "Error: Exit time must be AFTER entry time.\n";
            return;
        }

        int durationMinutes = exitMinutes - entryMinutes;
        double price = priceList.getPrice(vehicle.getVehicleType());
        if (price <= 0) {
            cout << "Error: No valid price set for this vehicle type.\n";
            return;
        }

        int hoursCharged = calculateHoursCharged(durationMinutes);
        double totalFee = hoursCharged * price;

        // Create the immutable transaction record (price frozen here)
        transactions.push_back(ParkingTransaction(
            vehicle.getPlateNumber(),
            vehicle.getVehicleType(),
            vehicle.getAllocatedSlotId(),
            vehicle.getEntryTime(),
            exit,
            hoursCharged,
            price,
            totalFee
        ));

        // Free the slot and remove the vehicle from active records
        slots[slotIndex].setOccupied(false);
        activeVehicles.erase(it);

        // Persist to file so the transaction is kept for future reference
        saveTransactionsToFile();

        cout << "Vehicle exited successfully.\n";
        cout << "Duration: " << durationMinutes << " minute(s)\n";
        cout << "Hours charged (ceiling): " << hoursCharged << "\n";
        cout << "Price used: " << price << " FRW/hour\n";
        cout << "Total fee: " << totalFee << " FRW\n";
    }

    // ----------------------------------------------------------------
    // MENU 6: View Active Vehicles
    // ----------------------------------------------------------------
    void viewActiveVehicles() const {
        cout << "\n== Active Vehicles ==\n";
        if (activeVehicles.empty()) {
            cout << "No vehicles are currently parked.\n";
            return;
        }
        unordered_map<string, Vehicle>::const_iterator it;
        for (it = activeVehicles.begin(); it != activeVehicles.end(); ++it) {
            it->second.display();
        }
    }

    // ----------------------------------------------------------------
    // MENU 7: View Parking Prices
    // ----------------------------------------------------------------
    void viewPrices() const {
        priceList.display();
    }

    // ----------------------------------------------------------------
    // MENU 8: Update Parking Prices
    // ----------------------------------------------------------------
    void updatePrices() {
        cout << "\n== Update Parking Prices ==\n";
        string type = readVehicleType("Enter vehicle type to update: ");
        double newPrice = InputHelper::readPositiveDouble("Enter new price per hour (> 0): ");
        priceList.setPrice(type, newPrice);
        cout << "Price updated. Old transactions stay unchanged.\n";
    }

    // ----------------------------------------------------------------
    // MENU 9: View Transaction History
    // ----------------------------------------------------------------
    void viewTransactionHistory() const {
        cout << "\n== Transaction History ==\n";
        if (transactions.empty()) {
            cout << "No completed transactions yet.\n";
            return;
        }
        for (int i = 0; i < (int)transactions.size(); i++) {
            transactions[i].display();
        }
    }

    // ----------------------------------------------------------------
    // MENU 10: Display System Statistics
    // ----------------------------------------------------------------
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
        cout << "Total slots          : " << totalSlots << "\n";
        cout << "Available slots      : " << available << "\n";
        cout << "Occupied slots       : " << occupied << "\n";
        cout << "Active vehicles      : " << activeVehicles.size() << "\n";
        cout << "Completed transactions: " << transactions.size() << "\n";
        cout << "Total revenue        : " << totalRevenue << " FRW\n";
    }
};

// ===========================================================================
// Free function: print the menu (not a global variable, just a helper)
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
    ParkingManagementSystem system;  // the whole program state lives here

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
