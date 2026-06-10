/*
  Smart Parking Management System


  Storage:
    vector<ParkingSlot> parkingSlots;
    vector<Vehicle> activeVehicles;
    vector<ParkingTransaction> transactionHistory;

  Compile: g++ main.cpp -o parking
  Run:     ./parking
*/

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cctype>

using namespace std;

// Allowed vehicle types in this parking system
const string VEHICLE_TYPES[] = {"Car", "Motorcycle", "Truck", "Bus"};
const int VEHICLE_TYPE_COUNT = 4;

// ---------------------------------------------------------------------------
// Helper: remove spaces at start and end of a string
// ---------------------------------------------------------------------------
string trim(const string& text) {
    int start = 0;
    int end = (int)text.size() - 1;

    while (start <= end && isspace((unsigned char)text[start])) {
        start++;
    }
    while (end >= start && isspace((unsigned char)text[end])) {
        end--;
    }

    if (start > end) {
        return "";
    }
    return text.substr(start, end - start + 1);
}

// ---------------------------------------------------------------------------
// Helper: clear bad input so program does not crash or loop forever
// ---------------------------------------------------------------------------
void clearInputBuffer() {
    cin.clear();
    cin.ignore(10000, '\n');
}

// ---------------------------------------------------------------------------
// Helper: read a full line of text safely
// ---------------------------------------------------------------------------
string readLine(const string& prompt) {
    string value;
    cout << prompt;
    getline(cin, value);
    return trim(value);
}

// ---------------------------------------------------------------------------
// Helper: read menu number between min and max (handles non-numeric input)
// ---------------------------------------------------------------------------
int readMenuChoice(int minChoice, int maxChoice) {
    int choice = 0;

    while (true) {
        cout << "Enter choice (" << minChoice << "-" << maxChoice << "): ";

        if (!(cin >> choice)) {
            clearInputBuffer();
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        clearInputBuffer();

        if (choice >= minChoice && choice <= maxChoice) {
            return choice;
        }

        cout << "Invalid menu choice. Try again.\n";
    }
}

// ---------------------------------------------------------------------------
// Helper: read a positive price (rejects negative, zero, and bad input)
// ---------------------------------------------------------------------------
bool readPositivePrice(const string& prompt, double& priceOut) {
    cout << prompt;

    if (!(cin >> priceOut)) {
        clearInputBuffer();
        cout << "Invalid numeric input.\n";
        return false;
    }

    clearInputBuffer();

    if (priceOut <= 0) {
        cout << "Price must be greater than zero.\n";
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Helper: check if vehicle type is one of the allowed values
// ---------------------------------------------------------------------------
bool isValidVehicleType(const string& type) {
    for (int i = 0; i < VEHICLE_TYPE_COUNT; i++) {
        if (type == VEHICLE_TYPES[i]) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Helper: show allowed vehicle types
// ---------------------------------------------------------------------------
void showVehicleTypes() {
    cout << "Allowed vehicle types: ";
    for (int i = 0; i < VEHICLE_TYPE_COUNT; i++) {
        cout << VEHICLE_TYPES[i];
        if (i < VEHICLE_TYPE_COUNT - 1) {
            cout << ", ";
        }
    }
    cout << endl;
}

// ---------------------------------------------------------------------------
// Time is stored as hour (0-23) and minute (0-59)
// ---------------------------------------------------------------------------
struct Time {
    int hour;
    int minute;
};

// Convert Time to total minutes for easy comparison
int toMinutes(const Time& t) {
    return t.hour * 60 + t.minute;
}

// Read time with validation
bool readTime(const string& prompt, Time& timeOut) {
    cout << prompt << " (HH MM, 24-hour format): ";

    if (!(cin >> timeOut.hour >> timeOut.minute)) {
        clearInputBuffer();
        cout << "Invalid time input.\n";
        return false;
    }

    clearInputBuffer();

    if (timeOut.hour < 0 || timeOut.hour > 23 || timeOut.minute < 0 || timeOut.minute > 59) {
        cout << "Invalid time. Hour must be 0-23 and minute must be 0-59.\n";
        return false;
    }

    return true;
}

// Partial-hour billing: any extra minute counts as one full hour
// Example: 1 hour 1 minute -> 2 hours charged
int calculateBillableHours(int durationMinutes) {
    if (durationMinutes <= 0) {
        return 0;
    }
    return (int)ceil(durationMinutes / 60.0);
}

// ---------------------------------------------------------------------------
// CLASS: ParkingSlot
// One physical parking space in the lot
// ---------------------------------------------------------------------------
class ParkingSlot {
public:
    string slotId;
    string zone;
    string vehicleType;  // which type of vehicle can use this slot
    bool isOccupied;

    ParkingSlot() {
        slotId = "";
        zone = "";
        vehicleType = "";
        isOccupied = false;
    }

    ParkingSlot(string id, string z, string type) {
        slotId = id;
        zone = z;
        vehicleType = type;
        isOccupied = false;
    }
};

// ---------------------------------------------------------------------------
// CLASS: Vehicle (currently parked)
// ---------------------------------------------------------------------------
class Vehicle {
public:
    string plateNumber;
    string vehicleType;
    string slotId;
    Time entryTime;

    Vehicle() {
        plateNumber = "";
        vehicleType = "";
        slotId = "";
        entryTime.hour = 0;
        entryTime.minute = 0;
    }

    Vehicle(string plate, string type, string slot, Time entry) {
        plateNumber = plate;
        vehicleType = type;
        slotId = slot;
        entryTime = entry;
    }
};

// ---------------------------------------------------------------------------
// CLASS: ParkingTransaction (completed parking - never edited after creation)
// ---------------------------------------------------------------------------
class ParkingTransaction {
public:
    string plateNumber;
    string slotId;
    string zone;
    string vehicleType;
    Time entryTime;
    Time exitTime;
    int billableHours;
    double hourlyRateUsed;  // rate at exit time (saved so old records never change)
    double totalFee;

    ParkingTransaction() {
        plateNumber = "";
        slotId = "";
        zone = "";
        vehicleType = "";
        entryTime.hour = 0;
        entryTime.minute = 0;
        exitTime.hour = 0;
        exitTime.minute = 0;
        billableHours = 0;
        hourlyRateUsed = 0;
        totalFee = 0;
    }

    ParkingTransaction(string plate, string slot, string z, string type,
                       Time entry, Time exit, int hours, double rate, double fee) {
        plateNumber = plate;
        slotId = slot;
        zone = z;
        vehicleType = type;
        entryTime = entry;
        exitTime = exit;
        billableHours = hours;
        hourlyRateUsed = rate;
        totalFee = fee;
    }
};

// ---------------------------------------------------------------------------
// CLASS: PriceRate - current hourly price per vehicle type
// Updating this does NOT change old transactions (they store their own rate)
// ---------------------------------------------------------------------------
class PriceRate {
public:
    string vehicleType;
    double hourlyRate;

    PriceRate() {
        vehicleType = "";
        hourlyRate = 0;
    }

    PriceRate(string type, double rate) {
        vehicleType = type;
        hourlyRate = rate;
    }
};

// Global storage (simple exam style)
vector<ParkingSlot> parkingSlots;
vector<Vehicle> activeVehicles;
vector<ParkingTransaction> transactionHistory;
vector<PriceRate> priceRates;

// ---------------------------------------------------------------------------
// Initialize default prices (all must be > 0)
// ---------------------------------------------------------------------------
void initializePrices() {
    priceRates.clear();
    priceRates.push_back(PriceRate("Car", 500));
    priceRates.push_back(PriceRate("Motorcycle", 200));
    priceRates.push_back(PriceRate("Truck", 800));
    priceRates.push_back(PriceRate("Bus", 1000));
}

// ---------------------------------------------------------------------------
// LINEAR SEARCH: find parking slot index by slot ID
// Returns -1 if not found
// ---------------------------------------------------------------------------
int findSlotIndexById(const string& slotId) {
    for (int i = 0; i < (int)parkingSlots.size(); i++) {
        if (parkingSlots[i].slotId == slotId) {
            return i;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// LINEAR SEARCH: find active vehicle index by plate number
// Returns -1 if not found
// ---------------------------------------------------------------------------
int findActiveVehicleIndexByPlate(const string& plate) {
    for (int i = 0; i < (int)activeVehicles.size(); i++) {
        if (activeVehicles[i].plateNumber == plate) {
            return i;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// LINEAR SEARCH: get current hourly rate for a vehicle type
// Returns -1 if price not set or invalid
// ---------------------------------------------------------------------------
double getHourlyRate(const string& vehicleType) {
    for (int i = 0; i < (int)priceRates.size(); i++) {
        if (priceRates[i].vehicleType == vehicleType) {
            if (priceRates[i].hourlyRate <= 0) {
                return -1;
            }
            return priceRates[i].hourlyRate;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Find first free slot that matches vehicle type
// Returns slot index or -1
// ---------------------------------------------------------------------------
int findAvailableSlotForType(const string& vehicleType) {
    for (int i = 0; i < (int)parkingSlots.size(); i++) {
        if (parkingSlots[i].vehicleType == vehicleType && !parkingSlots[i].isOccupied) {
            return i;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Check if any slot exists for this vehicle type (occupied or not)
// ---------------------------------------------------------------------------
bool slotExistsForType(const string& vehicleType) {
    for (int i = 0; i < (int)parkingSlots.size(); i++) {
        if (parkingSlots[i].vehicleType == vehicleType) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// MENU 1: Add parking slot
// ---------------------------------------------------------------------------
void addParkingSlot() {
    string slotId = readLine("Enter Slot ID: ");
    if (slotId.empty()) {
        cout << "Error: Slot ID cannot be empty.\n";
        return;
    }

    if (findSlotIndexById(slotId) != -1) {
        cout << "Error: Duplicate Slot ID. Slot already exists.\n";
        return;
    }

    string zone = readLine("Enter Zone: ");
    if (zone.empty()) {
        cout << "Error: Zone cannot be empty.\n";
        return;
    }

    showVehicleTypes();
    string vehicleType = readLine("Enter Vehicle Type for this slot: ");
    if (!isValidVehicleType(vehicleType)) {
        cout << "Error: Invalid vehicle type.\n";
        return;
    }

    ParkingSlot newSlot(slotId, zone, vehicleType);
    parkingSlots.push_back(newSlot);
    cout << "Parking slot added successfully.\n";
}

// ---------------------------------------------------------------------------
// MENU 2: Display all parking slots
// ---------------------------------------------------------------------------
void displayParkingSlots() {
    if (parkingSlots.size() == 0) {
        cout << "No parking slots registered.\n";
        return;
    }

    cout << "\n--- Parking Slots ---\n";
    for (int i = 0; i < (int)parkingSlots.size(); i++) {
        cout << "Slot ID: " << parkingSlots[i].slotId
             << " | Zone: " << parkingSlots[i].zone
             << " | Type: " << parkingSlots[i].vehicleType
             << " | Status: " << (parkingSlots[i].isOccupied ? "Occupied" : "Available")
             << endl;
    }
}

// ---------------------------------------------------------------------------
// MENU 3: Vehicle entry (park)
// ---------------------------------------------------------------------------
void parkVehicle() {
    string plate = readLine("Enter Plate Number: ");
    if (plate.empty()) {
        cout << "Error: Plate number cannot be empty.\n";
        return;
    }

    if (findActiveVehicleIndexByPlate(plate) != -1) {
        cout << "Error: This vehicle is already parked. Duplicate plate not allowed.\n";
        return;
    }

    showVehicleTypes();
    string vehicleType = readLine("Enter Vehicle Type: ");
    if (!isValidVehicleType(vehicleType)) {
        cout << "Error: Invalid vehicle type.\n";
        return;
    }

    if (!slotExistsForType(vehicleType)) {
        cout << "Error: No matching slot exists for this vehicle type.\n";
        return;
    }

    int slotIndex = findAvailableSlotForType(vehicleType);
    if (slotIndex == -1) {
        cout << "Error: No available slot for this vehicle type.\n";
        return;
    }

    Time entryTime;
    if (!readTime("Enter Entry Time", entryTime)) {
        return;
    }

    // Park vehicle
    string assignedSlotId = parkingSlots[slotIndex].slotId;
    parkingSlots[slotIndex].isOccupied = true;

    Vehicle newVehicle(plate, vehicleType, assignedSlotId, entryTime);
    activeVehicles.push_back(newVehicle);

    cout << "Vehicle parked successfully in slot " << assignedSlotId << ".\n";
}

// ---------------------------------------------------------------------------
// MENU 4: Vehicle exit
// ---------------------------------------------------------------------------
void exitVehicle() {
    string plate = readLine("Enter Plate Number: ");
    if (plate.empty()) {
        cout << "Error: Plate number cannot be empty.\n";
        return;
    }

    int vehicleIndex = findActiveVehicleIndexByPlate(plate);
    if (vehicleIndex == -1) {
        cout << "Error: Vehicle not found in active parking list.\n";
        return;
    }

    Vehicle parked = activeVehicles[vehicleIndex];

    int slotIndex = findSlotIndexById(parked.slotId);
    if (slotIndex == -1) {
        cout << "Error: Invalid vehicle record. Assigned slot no longer exists.\n";
        return;
    }

    Time exitTime;
    if (!readTime("Enter Exit Time", exitTime)) {
        return;
    }

    int entryMinutes = toMinutes(parked.entryTime);
    int exitMinutes = toMinutes(exitTime);

    if (exitMinutes < entryMinutes) {
        cout << "Error: Exit time cannot be before entry time.\n";
        return;
    }

    int durationMinutes = exitMinutes - entryMinutes;
    if (durationMinutes == 0) {
        cout << "Error: Zero duration is not allowed.\n";
        return;
    }

    double hourlyRate = getHourlyRate(parked.vehicleType);
    if (hourlyRate <= 0) {
        cout << "Error: Invalid or missing hourly price for this vehicle type.\n";
        return;
    }

    int billableHours = calculateBillableHours(durationMinutes);
    double totalFee = billableHours * hourlyRate;

    // Create transaction ONCE (immutable record with rate frozen at exit time)
    ParkingTransaction transaction(
        parked.plateNumber,
        parked.slotId,
        parkingSlots[slotIndex].zone,
        parked.vehicleType,
        parked.entryTime,
        exitTime,
        billableHours,
        hourlyRate,
        totalFee
    );
    transactionHistory.push_back(transaction);

    // Free slot and remove active vehicle
    parkingSlots[slotIndex].isOccupied = false;
    activeVehicles.erase(activeVehicles.begin() + vehicleIndex);

    cout << "Vehicle exited successfully.\n";
    cout << "Duration: " << durationMinutes << " minute(s)\n";
    cout << "Billable hours (partial hour counts as full): " << billableHours << endl;
    cout << "Hourly rate used: " << hourlyRate << endl;
    cout << "Total fee: " << totalFee << endl;
}

// ---------------------------------------------------------------------------
// MENU 5: Display active vehicles
// ---------------------------------------------------------------------------
void displayActiveVehicles() {
    if (activeVehicles.size() == 0) {
        cout << "No vehicles currently parked.\n";
        return;
    }

    cout << "\n--- Active Vehicles ---\n";
    for (int i = 0; i < (int)activeVehicles.size(); i++) {
        cout << "Plate: " << activeVehicles[i].plateNumber
             << " | Type: " << activeVehicles[i].vehicleType
             << " | Slot: " << activeVehicles[i].slotId
             << " | Entry: " << activeVehicles[i].entryTime.hour << ":"
             << (activeVehicles[i].entryTime.minute < 10 ? "0" : "")
             << activeVehicles[i].entryTime.minute
             << endl;
    }
}

// ---------------------------------------------------------------------------
// MENU 6: Display transaction history
// ---------------------------------------------------------------------------
void displayTransactionHistory() {
    if (transactionHistory.size() == 0) {
        cout << "No completed transactions yet.\n";
        return;
    }

    cout << "\n--- Transaction History (read-only records) ---\n";
    for (int i = 0; i < (int)transactionHistory.size(); i++) {
        cout << "Plate: " << transactionHistory[i].plateNumber
             << " | Slot: " << transactionHistory[i].slotId
             << " | Zone: " << transactionHistory[i].zone
             << " | Type: " << transactionHistory[i].vehicleType
             << " | Entry: " << transactionHistory[i].entryTime.hour << ":"
             << (transactionHistory[i].entryTime.minute < 10 ? "0" : "")
             << transactionHistory[i].entryTime.minute
             << " | Exit: " << transactionHistory[i].exitTime.hour << ":"
             << (transactionHistory[i].exitTime.minute < 10 ? "0" : "")
             << transactionHistory[i].exitTime.minute
             << " | Hours billed: " << transactionHistory[i].billableHours
             << " | Rate used: " << transactionHistory[i].hourlyRateUsed
             << " | Fee: " << transactionHistory[i].totalFee
             << endl;
    }
}

// ---------------------------------------------------------------------------
// MENU 7: Update hourly price (does not change old transactions)
// ---------------------------------------------------------------------------
void updateHourlyPrice() {
    showVehicleTypes();
    string vehicleType = readLine("Enter vehicle type to update price: ");

    if (!isValidVehicleType(vehicleType)) {
        cout << "Error: Invalid vehicle type.\n";
        return;
    }

    double newPrice = 0;
    if (!readPositivePrice("Enter new hourly price (> 0): ", newPrice)) {
        return;
    }

    for (int i = 0; i < (int)priceRates.size(); i++) {
        if (priceRates[i].vehicleType == vehicleType) {
            priceRates[i].hourlyRate = newPrice;
            cout << "Price updated for " << vehicleType << ". Old transactions are unchanged.\n";
            return;
        }
    }

    cout << "Error: Price record not found.\n";
}

// ---------------------------------------------------------------------------
// MENU 8: Display current prices
// ---------------------------------------------------------------------------
void displayCurrentPrices() {
    cout << "\n--- Current Hourly Prices ---\n";
    for (int i = 0; i < (int)priceRates.size(); i++) {
        cout << priceRates[i].vehicleType << ": " << priceRates[i].hourlyRate << " per hour\n";
    }
}

// ---------------------------------------------------------------------------
// Show main menu
// ---------------------------------------------------------------------------
void showMenu() {
    cout << "\n===== Smart Parking Management System =====\n";
    cout << "1. Add Parking Slot\n";
    cout << "2. Display Parking Slots\n";
    cout << "3. Park Vehicle (Entry)\n";
    cout << "4. Exit Vehicle\n";
    cout << "5. Display Active Vehicles\n";
    cout << "6. Display Transaction History\n";
    cout << "7. Update Hourly Price\n";
    cout << "8. Display Current Prices\n";
    cout << "9. Exit Program\n";
}

int main() {
    initializePrices();

    int choice = 0;

    while (true) {
        showMenu();
        choice = readMenuChoice(1, 9);

        switch (choice) {
            case 1:
                addParkingSlot();
                break;
            case 2:
                displayParkingSlots();
                break;
            case 3:
                parkVehicle();
                break;
            case 4:
                exitVehicle();
                break;
            case 5:
                displayActiveVehicles();
                break;
            case 6:
                displayTransactionHistory();
                break;
            case 7:
                updateHourlyPrice();
                break;
            case 8:
                displayCurrentPrices();
                break;
            case 9:
                cout << "Program ended. Goodbye.\n";
                return 0;
            default:
                cout << "Invalid choice.\n";
                break;
        }
    }

    return 0;
}
