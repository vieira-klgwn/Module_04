#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>

using namespace std;

class ParkingSlot {
public:
    int slotId;
    string vehicleType;
    string zone;
    bool occupied;

    ParkingSlot(int id, string type, string z) {
        slotId = id;
        vehicleType = type;
        zone = z;
        occupied = false;
    }
};

class Vehicle {
public:
    string plateNumber;
    string vehicleType;
    time_t entryTime;
    int slotId;

    Vehicle(string plate, string type, int slot) {
        plateNumber = plate;
        vehicleType = type;
        slotId = slot;
        entryTime = time(nullptr);
    }
};

class Transaction {
public:
    string plateNumber;
    string vehicleType;
    int slotId;
    int hoursParked;
    double fee;

    Transaction(string plate,
                string type,
                int slot,
                int hours,
                double amount) {
        plateNumber = plate;
        vehicleType = type;
        slotId = slot;
        hoursParked = hours;
        fee = amount;
    }
};

class ParkingSystem {
private:
    vector<ParkingSlot> slots;
    vector<Vehicle> parkedVehicles;
    vector<Transaction> history;

    double motorcycleRate;
    double carRate;

public:
    ParkingSystem() {
        motorcycleRate = 500;
        carRate = 1000;
    }

    void configureSlot() {
        int id;
        string type, zone;

        cout << "\nEnter Slot ID: ";
        cin >> id;

        for (auto &slot : slots) {
            if (slot.slotId == id) {
                cout << "Error: Slot ID already exists.\n";
                return;
            }
        }

        cout << "Enter Vehicle Type (Motorcycle/Car): ";
        cin >> type;

        if (type != "Motorcycle" && type != "Car") {
            cout << "Invalid vehicle type.\n";

            return;
        }

        

        cout << "Enter Zone: ";
        cin >> zone;

        slots.push_back(ParkingSlot(id, type, zone));

        cout << "Parking slot added successfully.\n";
    }

    void registerVehicle() {
        string plate, type;

        cout << "\nEnter Plate Number: ";
        cin >> plate;

        for (auto &vehicle : parkedVehicles) {
            if (vehicle.plateNumber == plate) {
                cout << "Error: Vehicle is already parked.\n";
                return;
            }
        }

        cout << "Enter Vehicle Type (Motorcycle/Car): ";
        cin >> type;

        if (type != "Motorcycle" && type != "Car") {
            cout << "Invalid vehicle type.\n";
            return;
        }

        int slotIndex = -1;

        for (int i = 0; i < slots.size(); i++) {
            if (!slots[i].occupied &&
                slots[i].vehicleType == type) {
                slotIndex = i;
                break;
            }
        }

        if (slotIndex == -1) {
            cout << "No suitable parking slot available.\n";
            return;
        }




        slots[slotIndex].occupied = true;

        parkedVehicles.push_back(
            Vehicle(
                plate,
                type,
                slots[slotIndex].slotId
            )
        );

        cout << "Vehicle parked successfully.\n";
        cout << "Allocated Slot ID: "
             << slots[slotIndex].slotId << endl;
    }

    void vehicleExit() {
        string plate;

        cout << "\nEnter Plate Number: ";
        cin >> plate;

        int vehicleIndex = -1;

        for (int i = 0; i < parkedVehicles.size(); i++) {
            if (parkedVehicles[i].plateNumber == plate) {
                vehicleIndex = i;
                break;
            }
        }

        if (vehicleIndex == -1) {
            cout << "Vehicle not found.\n";
            return;
        }

        Vehicle vehicle = parkedVehicles[vehicleIndex];

        time_t exitTime = time(nullptr);

        double seconds =
            difftime(exitTime, vehicle.entryTime);

        int hoursParked =
            max(1, (int)ceil(seconds / 3600.0));

        double rate;

        if (vehicle.vehicleType == "Motorcycle")
            rate = motorcycleRate;
        else
            rate = carRate;

        double fee = hoursParked * rate;

        for (auto &slot : slots) {
            if (slot.slotId == vehicle.slotId) {
                slot.occupied = false;
                break;
            }
        }

        history.push_back(
            Transaction(
                vehicle.plateNumber,
                vehicle.vehicleType,
                vehicle.slotId,
                hoursParked,
                fee
            )
        );

        parkedVehicles.erase(
            parkedVehicles.begin() + vehicleIndex
        );

        cout << "\n===== PARKING RECEIPT =====\n";
        cout << "Plate Number : "
             << vehicle.plateNumber << endl;

        cout << "Vehicle Type : "
             << vehicle.vehicleType << endl;

        cout << "Slot ID      : "
             << vehicle.slotId << endl;

        cout << "Hours Parked : "
             << hoursParked << endl;

        cout << "Parking Fee  : "
             << fee << " FRW" << endl;

        cout << "Slot Released Successfully.\n";
    }

    void updatePrices() {
        cout << "\nCurrent Prices:\n";
        cout << "Motorcycle: "
             << motorcycleRate
             << " FRW/hour\n";

        cout << "Car: "
             << carRate
             << " FRW/hour\n";

        double newMotorcycleRate;
        double newCarRate;

        cout << "\nEnter New Motorcycle Rate: ";
        cin >> newMotorcycleRate;

        cout << "Enter New Car Rate: ";
        cin >> newCarRate;

        if (newMotorcycleRate <= 0 ||
            newCarRate <= 0) {
            cout << "Rates must be positive.\n";
            return;
        }

        motorcycleRate = newMotorcycleRate;
        carRate = newCarRate;

        cout << "Prices updated successfully.\n";
    }

    void viewSlots() {
        cout << "\n===== PARKING SLOTS =====\n";

        if (slots.empty()) {
            cout << "No parking slots configured.\n";
            return;
        }

        for (auto &slot : slots) {
            cout << "Slot ID: "
                 << slot.slotId
                 << " | Type: "
                 << slot.vehicleType
                 << " | Zone: "
                 << slot.zone
                 << " | Status: "
                 << (slot.occupied
                     ? "Occupied"
                     : "Available")
                 << endl;
        }
    }

    void viewParkedVehicles() {
        cout << "\n===== PARKED VEHICLES =====\n";

        if (parkedVehicles.empty()) {
            cout << "No vehicles currently parked.\n";
            return;
        }

        for (auto &vehicle : parkedVehicles) {
            cout << "Plate: "
                 << vehicle.plateNumber
                 << " | Type: "
                 << vehicle.vehicleType
                 << " | Slot ID: "
                 << vehicle.slotId
                 << endl;
        }
    }

    void viewHistory() {
        cout << "\n===== TRANSACTION HISTORY =====\n";

        if (history.empty()) {
            cout << "No completed transactions.\n";
            return;
        }

        for (auto &record : history) {
            cout << "Plate: "
                 << record.plateNumber
                 << " | Type: "
                 << record.vehicleType
                 << " | Slot: "
                 << record.slotId
                 << " | Hours: "
                 << record.hoursParked
                 << " | Fee: "
                 << record.fee
                 << " FRW"
                 << endl;
        }
    }

    void menu() {
        int choice;

        do {
            cout << "\n=================================\n";
            cout << " PARKING MANAGEMENT SYSTEM\n";
            cout << "=================================\n";
            cout << "1. Configure Parking Slot\n";
            cout << "2. Register Vehicle Entry\n";
            cout << "3. Vehicle Exit\n";
            cout << "4. Update Parking Prices\n";
            cout << "5. View Parking Slots\n";
            cout << "6. View Parked Vehicles\n";
            cout << "7. View Transaction History\n";
            cout << "8. Exit\n";
            cout << "=================================\n";
            cout << "Enter Choice: ";

            cin >> choice;

            switch (choice) {
                case 1:
                    configureSlot();
                    break;

                case 2:
                    registerVehicle();
                    break;

                case 3:
                    vehicleExit();
                    break;

                case 4:
                    updatePrices();
                    break;

                case 5:
                    viewSlots();
                    break;

                case 6:
                    viewParkedVehicles();
                    break;

                case 7:
                    viewHistory();
                    break;

                case 8:
                    cout << "Exiting Program...\n";
                    break;

                default:
                    cout << "Invalid choice.\n";
            }

        } while (choice != 8);
    }
};

int main() {
    ParkingSystem system;
    system.menu();
    return 0;
}