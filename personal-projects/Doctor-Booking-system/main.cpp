#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <windows.h>
#include <fstream>

using namespace std;


void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

const int COLOR_DEFAULT = 7;
const int COLOR_TITLE = 11;
const int COLOR_SUCCESS = 10;
const int COLOR_ERROR = 12;
const int COLOR_PROMPT = 14;

void typeEffect(const string& text, int delay = 30, int color = COLOR_DEFAULT);
void clearScreen();
void welcomeScreen();
void loadingDots();
void progressBarAnimation(int length = 20, int delay = 50);
void MainMenu();
void drawBorder(int width, char borderChar = '=');
void UserDashboard(const string& username );

[[nodiscard]] int GetLastIDFromFile(const string& filename) {
    fstream file(filename, ios::in);
    string line;
    int lastID = 0;

    while (getline(file, line)) {
        size_t commaPos = line.find(',');
        if (commaPos != string::npos) {
            try {
                int id = stoi(line.substr(0, commaPos));
                if (id > lastID) lastID = id;
            } catch (...) {
                continue;
            }
        }
    }

    file.close();
    return lastID;
}


class User {
private:
    string UserName, Gender, Country, Address, UserTel, password, confirmPassword;
    int Age, ID;
    static int IDCounter;

public:
    User() : Age(0) {
        ID = ++IDCounter;
    }

    static void LoadLastID() {
        IDCounter = GetLastIDFromFile("users.csv");
    }

    void FillUserInfo();
    void SaveUserInfo();
    void DisplayUserInfo();
};

int User::IDCounter = 0;

void User::FillUserInfo() {
    clearScreen();
    drawBorder(40);
    typeEffect("     User Registration     \n", 30, COLOR_TITLE);
    drawBorder(40);
    typeEffect("\nPlease fill in your information...\n\n", 20, COLOR_PROMPT);

ت    do {
        typeEffect("Username: ", 20, COLOR_PROMPT);
        getline(cin, UserName);
        if (UserName.empty())
            typeEffect("------------------------- \nUsername required.\n------------------------- \n", 20, COLOR_ERROR);
        else if (UserName.length() < 8 || UserName.length() > 100)
            typeEffect("------------------------- \nInvalid username! Must be 8-100 characters.\n------------------------- \n", 20, COLOR_ERROR);
    } while (UserName.empty() || UserName.length() < 8 || UserName.length() > 100);

    do {
        typeEffect("Password: ", 20, COLOR_PROMPT);
        getline(cin, password);
        if (password.empty())
            typeEffect("------------ \nPassword required!\n------------ \n", 20, COLOR_ERROR);
        else if (password.length() < 8)
            typeEffect("--------------------------------------- \nPassword must contain at least 8 characters!\n--------------------------------------- \n", 20, COLOR_ERROR);
        else {
            typeEffect("Confirm Password: ", 20, COLOR_PROMPT);
            getline(cin, confirmPassword);
        }
        if (password != confirmPassword)
            typeEffect("--------------------------------------- \nPasswords do not match. Please try again.\n--------------------------------------- \n", 20, COLOR_ERROR);
    } while (password.empty() || password.length() < 8 || password != confirmPassword);

    typeEffect("Gender: ", 20, COLOR_PROMPT); cin >> Gender;
    typeEffect("Age: ", 20, COLOR_PROMPT);
    while (!(cin >> Age) || Age <= 0 || Age > 100) {
        typeEffect("Invalid age. Try again: ", 20, COLOR_ERROR);
        cin.clear(); cin.ignore(1000, '\n');
    }

    typeEffect("Telephone: ", 20, COLOR_PROMPT); cin >> UserTel;
    typeEffect("Country: ", 20, COLOR_PROMPT); cin >> Country;
    typeEffect("Address: ", 20, COLOR_PROMPT); cin.ignore(); getline(cin, Address);
    clearScreen();
}

void User::SaveUserInfo() {
    fstream UsersInfo("users.csv", ios::app);
    UsersInfo << ID << "," << UserName << "," << Gender << "," << Age << ","
              << UserTel << "," << Country << "," << Address << "," << password << endl;
    UsersInfo.close();
}

void User::DisplayUserInfo(){
    typeEffect("     Your Profile     \n", 30, COLOR_TITLE);
    drawBorder(40);
    typeEffect("Username: " + UserName + "\n", 20, COLOR_DEFAULT);
    typeEffect("Gender: " + Gender + "\n", 20, COLOR_DEFAULT);
    typeEffect("Age: " + to_string(Age) + "\n", 20, COLOR_DEFAULT);
    typeEffect("Telephone: " + UserTel + "\n", 20, COLOR_DEFAULT);
    typeEffect("Country: " + Country + "\n", 20, COLOR_DEFAULT);
    typeEffect("Address: " + Address + "\n", 20, COLOR_DEFAULT);
    drawBorder(40);
    typeEffect("\nPress Enter to return to dashboard...\n", 20, COLOR_PROMPT);
    cin.get();
}


class Doctor {
private:
    string DocName, DocDegree, MedicalSpecialty, Address, password, confirmPassword;
    int DocNum, BookingFee, ID;
    static int DocIDCounter;

public:
    Doctor() : DocNum(0), BookingFee(0) {
        ID = ++DocIDCounter;
    }

    static void LoadLastID() {
        DocIDCounter = GetLastIDFromFile("Doctors.csv");
    }

    void FillDocInfo();
    void SaveDocInfo();
};

int Doctor::DocIDCounter = 0;

void Doctor::FillDocInfo() {
    clearScreen();
    drawBorder(40);
    typeEffect("     Doctor Registration     \n", 30, COLOR_TITLE);
    drawBorder(40);
    typeEffect("\nPlease fill in your information...\n\n", 20, COLOR_PROMPT);

    do {
        typeEffect("Username: ", 20, COLOR_PROMPT);
        getline(cin, DocName);
        if (DocName.empty())
            typeEffect("------------------------- \nUsername required.\n------------------------- \n", 20, COLOR_ERROR);
        else if (DocName.length() < 8 || DocName.length() > 100)
            typeEffect("------------------------- \nInvalid username! Must be 8-100 characters.\n------------------------- \n", 20, COLOR_ERROR);
    } while (DocName.empty() || DocName.length() < 8 || DocName.length() > 100);

    do {
        typeEffect("Password: ", 20, COLOR_PROMPT);
        getline(cin, password);
        if (password.empty())
            typeEffect("------------ \nPassword required!\n------------ \n", 20, COLOR_ERROR);
        else if (password.length() < 8)
            typeEffect("--------------------------------------- \nPassword must contain at least 8 characters!\n--------------------------------------- \n", 20, COLOR_ERROR);
        else {
            typeEffect("Confirm Password: ", 20, COLOR_PROMPT);
            getline(cin, confirmPassword);
        }
        if (password != confirmPassword)
            typeEffect("--------------------------------------- \nPasswords do not match. Please try again.\n--------------------------------------- \n", 20, COLOR_ERROR);
    } while (password.empty() || password.length() < 8 || password != confirmPassword);

    typeEffect("Degree Title: ", 20, COLOR_PROMPT); cin >> DocDegree;
    typeEffect("Medical Specialty: ", 20, COLOR_PROMPT); cin >> MedicalSpecialty;
    typeEffect("Telephone: ", 20, COLOR_PROMPT); cin >> DocNum;
    typeEffect("Booking Price: ", 20, COLOR_PROMPT); cin >> BookingFee;
    typeEffect("Address: ", 20, COLOR_PROMPT); cin.ignore(); getline(cin, Address);
    clearScreen();
}

void Doctor::SaveDocInfo() {
    fstream DoctorsInfo("Doctors.csv", ios::app);
    DoctorsInfo << ID << "," << DocName << "," << DocDegree << "," << MedicalSpecialty << ","
                << DocNum << "," << BookingFee << "," << Address << "," << password << endl;
    DoctorsInfo.close();
}

class App {
public:
    string userLog, passLog;

    void UserLogin();
    void DoctorLogin();
    [[nodiscard]] bool checkLog(const string& filename) const;
};

void App::UserLogin() {
    clearScreen();
    drawBorder(40);
    typeEffect("     User Login     \n", 30, COLOR_TITLE);
    drawBorder(40);

    do {
        typeEffect("Username: ", 20, COLOR_PROMPT);
        getline(cin, userLog);
        if (userLog.empty())
            typeEffect("------------------------- \nUsername required.\n------------------------- \n", 20, COLOR_ERROR);
        else if (userLog.length() < 8 || userLog.length() > 100)
            typeEffect("------------------------- \nInvalid username! Must be 8-100 characters.\n------------------------- \n", 20, COLOR_ERROR);
    } while (userLog.empty() || userLog.length() < 8 || userLog.length() > 100);

    do {
        typeEffect("Password: ", 20, COLOR_PROMPT);
        getline(cin, passLog);
        if (passLog.empty())
            typeEffect("------------ \nPassword required!\n------------ \n", 20, COLOR_ERROR);
        else if (passLog.length() < 8)
            typeEffect("--------------------------------------- \nPassword must contain at least 8 characters!\n--------------------------------------- \n", 20, COLOR_ERROR);
    } while (passLog.empty() || passLog.length() < 8);

    if (checkLog("users.csv")) {
        typeEffect("\nLogin Successful!\n", 30, COLOR_SUCCESS);
        this_thread::sleep_for(chrono::milliseconds(1500));
        clearScreen();
        UserDashboard(userLog);
    } else {
        typeEffect("\nInvalid username or password.\n", 30, COLOR_ERROR);
        this_thread::sleep_for(chrono::milliseconds(1500));
    }
}

void App::DoctorLogin() {
    clearScreen();
    drawBorder(40);
    typeEffect("     Doctor Login     \n", 30, COLOR_TITLE);
    drawBorder(40);

    do {
        typeEffect("Username: ", 20, COLOR_PROMPT);
        getline(cin, userLog);
        if (userLog.empty())
            typeEffect("------------------------- \nUsername required.\n------------------------- \n", 20, COLOR_ERROR);
        else if (userLog.length() < 8 || userLog.length() > 100)
            typeEffect("------------------------- \nInvalid username! Must be 8-100 characters.\n------------------------- \n", 20, COLOR_ERROR);
    } while (userLog.empty() || userLog.length() < 8 || userLog.length() > 100);

    do {
        typeEffect("Password: ", 20, COLOR_PROMPT);
        getline(cin, passLog);
        if (passLog.empty())
            typeEffect("------------ \nPassword required!\n------------ \n", 20, COLOR_ERROR);
        else if (passLog.length() < 8)
            typeEffect("--------------------------------------- \nPassword must contain at least 8 characters!\n--------------------------------------- \n", 20, COLOR_ERROR);
    } while (passLog.empty() || passLog.length() < 8);

    if (checkLog("Doctors.csv")) {
        typeEffect("\nLogin Successful!\n", 30, COLOR_SUCCESS);
        this_thread::sleep_for(chrono::milliseconds(1500));
        clearScreen();
    } else {
        typeEffect("\nInvalid username or password.\n", 30, COLOR_ERROR);
        this_thread::sleep_for(chrono::milliseconds(1500));
    }
}

bool App::checkLog(const string& filename) const {
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        size_t pos;
        int fieldCount = 0;
        string fields[8];

        while ((pos = line.find(',')) != string::npos && fieldCount < 7) {
            fields[fieldCount++] = line.substr(0, pos);
            line.erase(0, pos + 1);
        }
        fields[fieldCount] = line;

        if (fields[1] == userLog && fields[7] == passLog)
            return true;
    }
    return false;
}

int main() {
    welcomeScreen();
    loadingDots();
    progressBarAnimation();
    clearScreen();
    User::LoadLastID();
    Doctor::LoadLastID();
    MainMenu();
    return 0;
}

void typeEffect(const string& text, int delay, int color) {
    setColor(color);
    for (char c : text) {
        cout << c << flush;
        this_thread::sleep_for(chrono::milliseconds(delay));
    }
    setColor(COLOR_DEFAULT);
}

void clearScreen() {
    system("cls");
}

void drawBorder(int width, char borderChar) {
    setColor(COLOR_TITLE);
    for (int i = 0; i < width; ++i) {
        cout << borderChar;
    }
    cout << endl;
    setColor(COLOR_DEFAULT);
}

void welcomeScreen() {
    clearScreen();
    drawBorder(40);
    typeEffect("     Welcome to Doctor Booking App     \n", 40, COLOR_TITLE);
    drawBorder(40);
    this_thread::sleep_for(chrono::milliseconds(1500));
}

void loadingDots() {
    typeEffect("Loading", 100, COLOR_PROMPT);
    for (int i = 0; i < 3; ++i) {
        setColor(COLOR_PROMPT);
        cout << "." << flush;
        this_thread::sleep_for(chrono::milliseconds(400));
    }
    setColor(COLOR_DEFAULT);
    cout << "\n";
}

void progressBarAnimation(int length, int delay) {
    typeEffect("Initializing: [", 30, COLOR_PROMPT);
    for (int i = 0; i < length; ++i) {
        setColor(COLOR_SUCCESS);
        cout << "|"<< flush;
        this_thread::sleep_for(chrono::milliseconds(delay));
    }
    setColor(COLOR_PROMPT);
    cout << "] Done!\n";
    setColor(COLOR_DEFAULT);
    this_thread::sleep_for(chrono::milliseconds(500));
}

void MainMenu() {
    int choice;
    do {
        clearScreen();
        drawBorder(40);
        typeEffect("          Main Menu          \n", 30, COLOR_TITLE);
        drawBorder(40);
        cout << endl;
        typeEffect("1. User Login\n", 20, COLOR_DEFAULT);
        typeEffect("2. Doctor Login\n", 20, COLOR_DEFAULT);
        typeEffect("3. User Registration\n", 20, COLOR_DEFAULT);
        typeEffect("4. Doctor Registration\n", 20, COLOR_DEFAULT);
        typeEffect("5. Exit\n", 20, COLOR_DEFAULT);
        drawBorder(40);
        typeEffect("Enter your choice (1-5): ", 20, COLOR_PROMPT);

        while (!(cin >> choice)) {
            typeEffect("Invalid input. Please enter a number (1-5): ", 20, COLOR_ERROR);
            cin.clear();
            cin.ignore(1000, '\n');
        }
        cin.ignore();

        User user;
        Doctor doctor;
        App login;

        switch (choice) {
            case 1:
                login.UserLogin();
                break;
            case 2:
                login.DoctorLogin();
                break;
            case 3:
                user.FillUserInfo();
                user.SaveUserInfo();
                drawBorder(40);
                typeEffect("Registration successful! Please log in.\n", 30, COLOR_SUCCESS);
                this_thread::sleep_for(chrono::milliseconds(2000));
                break;
            case 4:
                doctor.FillDocInfo();
                doctor.SaveDocInfo();
                drawBorder(40);
                typeEffect("Registration successful! Please log in.\n", 30, COLOR_SUCCESS);
                this_thread::sleep_for(chrono::milliseconds(2000));
                break;
            case 5:
                clearScreen();
                drawBorder(40);
                typeEffect("Exiting Doctor Booking App. Goodbye!\n", 30, COLOR_TITLE);
                drawBorder(40);
                this_thread::sleep_for(chrono::milliseconds(1000));
                break;
            default:
                typeEffect("Invalid choice. Please try again.\n", 30, COLOR_ERROR);
                this_thread::sleep_for(chrono::milliseconds(1000));
        }
    } while (choice != 5);
}

void UserDashboard (const string& username ) {
    int choice;
    do {
        clearScreen();
        drawBorder(40);
        typeEffect("     Welcome, " +  username + "!     \n", 30, COLOR_TITLE);
        drawBorder(40);
        cout << endl;
        typeEffect("1. View Profile\n", 20, COLOR_DEFAULT);
        typeEffect("2. View Available Doctors\n", 20, COLOR_DEFAULT);
        typeEffect("3. Log Out\n", 20, COLOR_DEFAULT);
        drawBorder(40);
        typeEffect("Enter your choice (1-3): ", 20, COLOR_PROMPT);

        while (!(cin >> choice)) {
            typeEffect("Invalid input. Please enter a number (1-3): ", 20, COLOR_ERROR);
            cin.clear();
            cin.ignore(1000, '\n');
        }
        cin.ignore();
        clearScreen();
        User user;
        switch (choice) {
            case 1:
                user.DisplayUserInfo();
                break;
            case 2:
                clearScreen();
                drawBorder(40);
                typeEffect("Available Doctors (Coming Soon)!\n", 30, COLOR_PROMPT);
                drawBorder(40);
                typeEffect("\nPress Enter to return to dashboard...\n", 20, COLOR_PROMPT);
                cin.get();
                break;
            case 3:
                clearScreen();
                drawBorder(40);
                typeEffect("Logging out...", 30, COLOR_SUCCESS);
                drawBorder(40);
                this_thread::sleep_for(chrono::milliseconds(1500));
                return;
            default:
                typeEffect("Invalid choice. Please try again.\n", 30, COLOR_ERROR);
                this_thread::sleep_for(chrono::milliseconds(1000));
        }
    } while (true);
}
