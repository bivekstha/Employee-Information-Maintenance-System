#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

// ---------- Data and rule tables ----------
struct Employee { int id; string name, dept; int exp, pos; };  // pos = index into POSITIONS
struct Salary { double base, mult, basic, allowance, gross, annualTax, monthlyTax, net; };
struct Position { const char* name; double mult; };
struct Slab { double from; int pct; };                          // 'pct' applies to income above 'from'

const Position POSITIONS[] = {{"Junior Employee", 1.0}, {"Senior Employee", 1.2}, {"Team Leader", 1.5},
                              {"Manager", 1.8}, {"General Manager", 2.2}};
const pair<int, double> PAY[] = {{1, 18000}, {3, 25000}, {5, 35000}, {8, 45000}, {12, 60000}, {99, 80000}};  // {max years, base pay}
const Slab SLABS[] = {{300000, 5}, {600000, 10}, {900000, 15}};
const int NUM_POS = sizeof POSITIONS / sizeof *POSITIONS, NUM_SLABS = sizeof SLABS / sizeof *SLABS;
const string FILE_NAME = "employees.txt", LINE(44, '='), DASH(44, '-');
vector<Employee> staff;

// ---------- Payroll rules (calculations only) ----------
double annualTax(double income) {   // each slab adds only the *extra* rate on income above its threshold
    double tax = 0; int prev = 0;
    for (auto [from, pct] : SLABS) { tax += max(0.0, income - from) * (pct - prev) / 100; prev = pct; }
    return tax;
}

Salary calc(const Employee& e) {
    Salary s{};
    for (auto [maxYears, pay] : PAY) if (e.exp <= maxYears) { s.base = pay; break; }
    s.mult = POSITIONS[e.pos].mult;
    s.basic = s.base * s.mult;
    s.allowance = e.exp * 1000.0;
    s.gross = s.basic + s.allowance;
    s.annualTax = annualTax(s.gross * 12);
    s.monthlyTax = s.annualTax / 12;
    s.net = s.gross - s.monthlyTax;
    return s;
}

// ---------- Input helpers ----------
string prompt(const string& msg) {
    string s;
    cout << msg;
    if (!getline(cin, s)) { cout << "\nInput closed. Goodbye!\n"; exit(0); }   // data is already saved
    return s;
}

int readInt(const string& msg, int lo, int hi) {
    while (true) {
        stringstream ss(prompt(msg));
        int v; char extra;
        if (ss >> v && !(ss >> extra) && v >= lo && v <= hi) return v;
        cout << "Invalid input! Enter a number from " << lo << " to " << hi << ".\n";
    }
}

string readText(const string& msg) {   // non-empty, no '|' (it is the file separator)
    while (true) {
        string v = prompt(msg);
        if (v.empty()) cout << "Invalid input! This cannot be empty.\n";
        else if (v.find('|') != string::npos) cout << "Invalid input! Do not use the '|' character.\n";
        else return v;
    }
}

bool ask(const string& q, const string& yes, const string& no) {
    cout << "\n" << q << "\n1. " << yes << "\n0. " << no << "\n";
    return readInt("Enter choice: ", 0, 1) == 1;
}

int selectPosition() {
    cout << "\nSelect Employee Position:\n";
    for (int i = 0; i < NUM_POS; i++) cout << i + 1 << ". " << POSITIONS[i].name << "\n";
    return readInt("Enter choice: ", 1, NUM_POS) - 1;
}

int readId() {
    int id = readInt("Enter Employee ID (0 to cancel): ", 0, 1000000);
    if (!id) cout << "Cancelled.\n";
    return id;
}

void readDetails(Employee& e, bool isNew) {   // shared by Add and Update
    string n = isNew ? "New " : "";
    e.name = readText("Enter " + n + "Name: ");
    e.dept = readText("Enter " + n + "Department: ");
    e.exp = readInt("Enter " + n + "Experience in years (0-50): ", 0, 50);
    e.pos = selectPosition();
}

// ---------- Records and file storage ----------
Employee* byId(int id) {
    for (auto& e : staff) if (e.id == id) return &e;
    return nullptr;
}

int positionIndex(const string& n) {
    for (int i = 0; i < NUM_POS; i++) if (n == POSITIONS[i].name) return i;
    return -1;
}

void save() {
    ofstream f(FILE_NAME);
    if (!f) { cout << "Error saving employee records!\n"; return; }
    for (auto& e : staff)
        f << e.id << '|' << e.name << '|' << e.dept << '|' << e.exp << '|' << POSITIONS[e.pos].name << '\n';
}

bool parseLine(const string& line, Employee& e) {   // false = damaged or duplicate line
    string t[6]; int n = 0;
    stringstream ss(line);
    while (n < 6 && getline(ss, t[n], '|')) n++;
    if (n != 5) return false;                       // must be exactly 5 fields
    try {
        size_t p1, p2;
        e = {stoi(t[0], &p1), t[1], t[2], stoi(t[3], &p2), positionIndex(t[4])};
        return p1 == t[0].size() && p2 == t[3].size() && e.id > 0 && e.exp >= 0 && e.exp <= 50 &&
               !e.name.empty() && !e.dept.empty() && e.pos >= 0 && !byId(e.id);
    } catch (...) { return false; }
}

int load() {   // returns how many bad lines were skipped
    ifstream f(FILE_NAME);
    string line; int skipped = 0;
    while (getline(f, line)) {
        if (line.empty()) continue;
        Employee e{};
        if (parseLine(line, e)) staff.push_back(e); else skipped++;
    }
    return skipped;
}

// ---------- Output helpers ----------
void title(const string& t) { cout << "\n========== " << t << " ==========\n"; }
void box(const string& t) { cout << LINE << "\n" << string((44 - t.size()) / 2, ' ') << t << "\n" << LINE << "\n"; }

template <class T>
void field(const string& label, const T& v, const string& pre = "", const string& suf = "") {
    cout << left << setw(22) << label << ": " << pre << v << suf << "\n";
}
void rs(const string& label, double v) { field(label, v, "Rs. "); }

void printRow(const string& a, const string& b, const string& c, const string& d, const string& e) {
    cout << left << setw(10) << a << setw(22) << b << setw(18) << c << setw(12) << d << setw(22) << e << "\n";
}
void header() { printRow("ID", "NAME", "DEPARTMENT", "EXPERIENCE", "POSITION"); cout << string(84, '-') << "\n"; }
void printRow(const Employee& e) { printRow(to_string(e.id), e.name, e.dept, to_string(e.exp), POSITIONS[e.pos].name); }

void showPay(const Salary& s) {
    rs("Base Pay (by exp.)", s.base);
    field("Pos. Multiplier", s.mult, "", "x");
    rs("Basic Salary", s.basic);
    rs("Experience Allow.", s.allowance);
    rs("Gross Monthly Salary", s.gross);
}

void showTax(const Salary& s) {
    rs("Annual Salary", s.gross * 12);
    rs("Annual Tax", s.annualTax);
    rs("Monthly Tax", s.monthlyTax);
}

void slip(const Employee& e) {
    Salary s = calc(e);
    cout << "\n"; box("EMPLOYEE SALARY SLIP");
    field("Employee ID", e.id); field("Employee Name", e.name); field("Department", e.dept);
    field("Experience", e.exp, "", " years"); field("Position", POSITIONS[e.pos].name);
    cout << DASH << "\n"; showPay(s);
    cout << DASH << "\n"; showTax(s);
    cout << DASH << "\n"; rs("NET MONTHLY SALARY", s.net);
    cout << LINE << "\nNote: Tax is based on illustrative project slabs.\n";
}

string inr(long n) {   // Indian digit grouping: 882000 -> 8,82,000
    string s = to_string(n), out = s.substr(max(0, (int)s.size() - 3));
    for (int i = (int)s.size() - 3; i > 0; i -= 2) out = s.substr(max(0, i - 2), min(2, i)) + "," + out;
    return out;
}

// Prints a banner, asks for an employee ID, then runs 'action' on that employee.
// This removes the same "ask, check, bail out" block from seven menu options.
template <class F>
void withEmployee(const string& banner, F action, const string& note = "") {
    title(banner);
    cout << note;
    int id = readId();
    if (!id) return;
    Employee* e = byId(id);
    if (!e) { cout << "Employee not found! (Use option 2 to see all IDs.)\n"; return; }
    action(*e);
}

// ---------- Menu options ----------
void addEmployee() {
    title("ADD NEW EMPLOYEE");
    int id = readId();
    if (!id) return;
    if (byId(id)) { cout << "Employee ID already exists! Every ID must be unique.\n"; return; }
    Employee e{id, "", "", 0, 0};
    readDetails(e, false);
    staff.push_back(e); save();
    cout << "\nEmployee added successfully!\n";
}

void viewEmployees() {
    title("ALL EMPLOYEES");
    if (staff.empty()) { cout << "No employee records found! Use option 1 to add one.\n"; return; }
    header();
    for (auto& e : staff) printRow(e);
    cout << "\nTotal employees: " << staff.size() << "\n";
}

void searchEmployee() {
    withEmployee("SEARCH EMPLOYEE", [](Employee& e) { cout << "\nEmployee Found!\n\n"; header(); printRow(e); slip(e); });
}

void updateEmployee() {
    withEmployee("UPDATE EMPLOYEE", [](Employee& e) {
        cout << "\nCurrent Details:\n\n"; header(); printRow(e);
        cout << "\nEnter Updated Details:\n";
        readDetails(e, true); save();
        cout << "\nEmployee updated successfully!\n";
    });
}

void deleteEmployee() {
    withEmployee("DELETE EMPLOYEE", [](Employee& e) {
        cout << "\n"; header(); printRow(e);
        if (ask("Are you sure you want to delete this employee?", "Yes, delete", "No, keep")) {
            staff.erase(staff.begin() + (&e - staff.data())); save();
            cout << "Employee deleted successfully!\n";
        } else cout << "Nothing was deleted.\n";
    });
}

void salaryCalculator() {
    withEmployee("SALARY CALCULATOR", [](Employee& e) {
        cout << "\n"; field("Employee", e.name); field("Position", POSITIONS[e.pos].name); field("Experience", e.exp, "", " years");
        cout << "\n"; showPay(calc(e));
    });
}

void taxCalculator() {
    withEmployee("TAX CALCULATOR", [](Employee& e) {
        Salary s = calc(e);
        cout << "\n"; field("Employee", e.name); field("Position", POSITIONS[e.pos].name);
        cout << "\n"; showTax(s); rs("Salary WITHOUT TAX", s.gross); rs("Salary AFTER TAX", s.net);
    });
}

void salarySlip() { withEmployee("GENERATE SALARY SLIP", [](Employee& e) { slip(e); }); }

// Previews a position change on a temporary copy; the real record changes only if the user confirms.
void simulate() {
    withEmployee("SMART CAREER PROGRESSION SIMULATOR", [](Employee& e) {
        Salary now = calc(e);
        cout << "\n"; field("Employee", e.name); field("Current Position", POSITIONS[e.pos].name);
        rs("Current Gross Salary", now.gross); rs("Current Net Salary", now.net);
        cout << "\nWhich position do you want to simulate?";
        int pos = selectPosition();
        if (pos == e.pos) { cout << "\n" << e.name << " already holds this position. Nothing to simulate.\n"; return; }
        Employee proposed = e; proposed.pos = pos;
        Salary then = calc(proposed);
        double change = then.gross - now.gross;

        cout << "\n"; box("SALARY COMPARISON REPORT");
        field("Employee", e.name); field("Old Position", POSITIONS[e.pos].name); field("Proposed Position", POSITIONS[pos].name);
        cout << DASH << "\n"; rs("CURRENT GROSS SALARY", now.gross); rs("EXPECTED GROSS SALARY", then.gross);
        cout << DASH << "\n"; rs("CURRENT NET SALARY", now.net); rs("EXPECTED NET SALARY", then.net);
        cout << DASH << "\n"; rs("MONTHLY GROSS CHANGE", change); rs("MONTHLY NET CHANGE", then.net - now.net);
        cout << "\nRESULT: " << (change > 0 ? "PROMOTION / SALARY INCREASE" : change < 0 ? "DEMOTION / SALARY DECREASE" : "NO SALARY CHANGE") << "\n";

        if (ask("Do you want to apply this position change?", "Yes", "No")) {
            e.pos = pos; save();
            cout << "\nPosition updated successfully!\nNew salary has been saved.\n";
        } else cout << "\nSimulation completed.\nOriginal employee position retained.\n";
    }, "Preview the salary change of a new position.\nNothing is saved unless you confirm at the end.\n");
}

void showHelp() {   // generated from the rule tables, so it always matches the real calculation
    title("HOW SALARY IS CALCULATED");
    cout << "\nSTEP 1: Base pay (from years of experience)\n";
    int from = 0;
    for (auto [maxYears, pay] : PAY) {
        string range = to_string(from) + (maxYears > 50 ? "+" : " - " + to_string(maxYears)) + " years";
        cout << "   " << left << setw(14) << range << ": Rs. " << inr(pay) << "\n";
        from = maxYears + 1;
    }
    cout << "\nSTEP 2: Multiply by the position multiplier\n" << setprecision(1);
    for (auto& p : POSITIONS) cout << "   " << left << setw(16) << p.name << ": " << p.mult << "x\n";
    cout << setprecision(2) << "\nSTEP 3: Add Rs. 1,000 for every year of experience\n   The result is the GROSS monthly salary.\n"
         << "\nSTEP 4: Tax is calculated on the annual salary (gross x 12)\n   Up to Rs. " << inr(SLABS[0].from) << " : 0%\n";
    for (int i = 0; i < NUM_SLABS; i++)
        cout << "   " << (i == NUM_SLABS - 1 ? "Above Rs. " + inr(SLABS[i].from)
                                             : "Rs. " + inr(SLABS[i].from) + " - Rs. " + inr(SLABS[i + 1].from))
             << " : " << SLABS[i].pct << "%\n";
    cout << "   Only the part of the income inside a slab is taxed at that slab's rate.\n"
         << "\nSTEP 5: Net monthly salary = gross - (annual tax / 12)\n"
         << "\nEXAMPLE: " << POSITIONS[2].name << " with 6 years of experience\n";
    Salary s = calc({0, "", "", 6, 2});
    showPay(s); showTax(s); rs("NET MONTHLY SALARY", s.net);
    cout << "\nNote: the tax slabs are illustrative project slabs.\n";
}

// ---------- Main menu ----------
int main() {
    cout << fixed << setprecision(2);
    int skipped = load();
    cout << "\n"; box("WELCOME TO THE EMPLOYEE MANAGEMENT SYSTEM");
    cout << "Loaded " << staff.size() << " employee record(s) from " << FILE_NAME << ".\n";
    if (skipped) cout << "Warning: " << skipped << " invalid line(s) in the file were skipped.\n";

    void (*actions[])() = {nullptr, addEmployee, viewEmployees, searchEmployee, updateEmployee, deleteEmployee,
                           salaryCalculator, taxCalculator, salarySlip, simulate, showHelp};
    while (true) {
        cout << "\n\n"; box("EMPLOYEE MANAGEMENT SYSTEM");
        cout << R"(MANAGE RECORDS
  1. Add New Employee
  2. View All Employees
  3. Search Employee (shows salary slip)
  4. Update Employee Details
  5. Delete Employee

SALARY & TAX
  6. Experience-Based Salary Calculator
  7. Salary With / Without Tax
  8. Generate Salary Slip

CAREER PLANNING
  9. Promotion / Demotion Salary Simulator

HELP
  10. How Is Salary Calculated?

  0. Exit
)" << LINE << "\n";
        int choice = readInt("Enter your choice (0-10): ", 0, 10);
        if (choice == 0) { cout << "\nAll records are saved.\nThank you for using Employee Management System!\n"; return 0; }
        actions[choice]();
        prompt("\nPress Enter to return to the menu...");
    }
}