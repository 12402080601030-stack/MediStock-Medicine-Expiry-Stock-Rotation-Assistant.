// Medicine Expiry and Stock Rotation Assistant  (C++17, console, standard library only)
// Inventory tool only - it gives no medical or dosage advice.
//
// Build:  g++ -std=c++17 -Wall -Wextra -o medicine_assistant main.cpp
//
// One record = one medicine BATCH (same medicine can have many batches).
// Searching and sorting are written by hand (no std::sort / std::find).
#define _CRT_SECURE_NO_WARNINGS  // stops the localtime warning on Visual Studio
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using namespace std;

struct Medicine {
    int id;
    string name, batch;
    int qty, minStock;
    string expiry;  // "YYYY-MM-DD" (this format also sorts correctly as plain text)
};

const string FILE_NAME = "medicines.csv";
vector<Medicine> stock;    // all batches, kept in memory
string today;              // reference date used for every expiry calculation
long long compares = 0;    // counts comparisons so the DAA menu can show real numbers

// ---------------------------------------------------------------- helpers
string lower(string s) {
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

// ------------------------------------------------------------------ dates
bool validDate(const string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)s[i])) return false;
    int y = stoi(s.substr(0, 4)), m = stoi(s.substr(5, 2)), d = stoi(s.substr(8, 2));
    if (y < 2000 || y > 2099 || m < 1 || m > 12 || d < 1) return false;
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) days[1] = 29;  // leap year
    return d <= days[m - 1];
}

time_t toTime(const string& s) {  // noon on that date, so daylight saving cannot shift the day
    tm t = {};
    t.tm_year = stoi(s.substr(0, 4)) - 1900;
    t.tm_mon = stoi(s.substr(5, 2)) - 1;
    t.tm_mday = stoi(s.substr(8, 2));
    t.tm_hour = 12;
    t.tm_isdst = -1;
    return mktime(&t);
}

int daysLeft(const string& expiry) {  // negative = already expired
    return (int)lround(difftime(toTime(expiry), toTime(today)) / 86400.0);
}

string systemToday() {
    time_t t = time(nullptr);
    char buf[16];
    strftime(buf, sizeof buf, "%Y-%m-%d", localtime(&t));
    return buf;
}

string status(const Medicine& m) {
    int d = daysLeft(m.expiry);
    return d < 0 ? "EXPIRED" : d <= 30 ? "<=30 DAYS" : d <= 90 ? "<=90 DAYS" : "OK";
}

// ------------------------------------------------------------------ input
// Every function keeps asking until the user types something valid.
string readLine(const string& prompt) {
    cout << prompt << flush;
    string s;
    if (!getline(cin, s)) {
        cout << "\nInput closed. Exiting.\n";
        exit(0);
    }
    size_t b = s.find_first_not_of(" \t\r\n"), e = s.find_last_not_of(" \t\r\n");
    return b == string::npos ? "" : s.substr(b, e - b + 1);
}

int readInt(const string& prompt, int lo, int hi) {
    while (true) {
        string s = readLine(prompt);
        if (!s.empty() && s.size() <= 9 && s.find_first_not_of("0123456789") == string::npos) {
            int v = stoi(s);
            if (v >= lo && v <= hi) return v;
        }
        cout << "  Enter a whole number from " << lo << " to " << hi << ".\n";
    }
}

string readText(const string& prompt) {  // no commas, because the file is comma-separated
    while (true) {
        string s = readLine(prompt);
        if (!s.empty() && s.size() <= 24 && s.find(',') == string::npos) return s;
        cout << "  Enter 1-24 characters, no commas.\n";
    }
}

string readDate(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        if (validDate(s)) return s;
        cout << "  Invalid date. Use YYYY-MM-DD, year 2000-2099, a real date (e.g. 2027-02-28).\n";
    }
}

bool readYes(const string& prompt) {
    while (true) {
        string s = lower(readLine(prompt));
        if (s == "y") return true;
        if (s == "n") return false;
        cout << "  Type y or n.\n";
    }
}

// -------------------------------------------------------------- searching
int linearSearchId(const vector<Medicine>& v, int id) {
    for (size_t i = 0; i < v.size(); i++) {
        compares++;
        if (v[i].id == id) return (int)i;
    }
    return -1;
}

vector<int> linearSearchName(const vector<Medicine>& v, const string& name) {
    vector<int> found;
    for (size_t i = 0; i < v.size(); i++) {
        compares++;
        if (lower(v[i].name) == lower(name)) found.push_back((int)i);
    }
    return found;
}

// Binary search: the vector MUST be sorted by ID first.
int binarySearchId(const vector<Medicine>& v, int id) {
    int lo = 0, hi = (int)v.size() - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        compares++;
        if (v[mid].id == id) return mid;
        if (v[mid].id < id) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

// Binary search by name: the vector MUST be sorted by name first.
// Finds the first matching position, then collects every batch with that name.
vector<int> binarySearchName(const vector<Medicine>& v, const string& name) {
    string key = lower(name);
    int lo = 0, hi = (int)v.size();
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        compares++;
        if (lower(v[mid].name) < key) lo = mid + 1;
        else hi = mid;
    }
    vector<int> found;
    for (int i = lo; i < (int)v.size() && lower(v[i].name) == key; i++) {
        compares++;
        found.push_back(i);
    }
    return found;
}

// ---------------------------------------------------------------- sorting
// key: 1 = ID, 2 = name, 3 = expiry date. Ties are broken by ID.
// Returns true when a must come before b. Every call counts as one comparison.
bool before(const Medicine& a, const Medicine& b, int key) {
    compares++;
    if (key == 2 && lower(a.name) != lower(b.name)) return lower(a.name) < lower(b.name);
    if (key == 3 && a.expiry != b.expiry) return a.expiry < b.expiry;
    return a.id < b.id;
}

// Selection sort: pick the smallest remaining item and swap it into place.
void selectionSort(vector<Medicine>& v, int key) {
    for (size_t i = 0; i + 1 < v.size(); i++) {
        size_t smallest = i;
        for (size_t j = i + 1; j < v.size(); j++)
            if (before(v[j], v[smallest], key)) smallest = j;
        swap(v[i], v[smallest]);
    }
}

// Merge sort: sort v[lo..hi) by splitting in half, sorting each half, then merging.
void mergeSort(vector<Medicine>& v, int lo, int hi, int key) {
    if (hi - lo < 2) return;
    int mid = (lo + hi) / 2;
    mergeSort(v, lo, mid, key);
    mergeSort(v, mid, hi, key);
    vector<Medicine> merged;
    int i = lo, j = mid;
    while (i < mid && j < hi) merged.push_back(before(v[j], v[i], key) ? v[j++] : v[i++]);
    while (i < mid) merged.push_back(v[i++]);
    while (j < hi) merged.push_back(v[j++]);
    for (size_t k = 0; k < merged.size(); k++) v[lo + k] = merged[k];
}

// ------------------------------------------------------------ file storage
// File format: id,name,batch,quantity,min_stock,expiry
void saveFile() {
    ofstream out(FILE_NAME);
    if (!out) {
        cout << "  ERROR: cannot write " << FILE_NAME << " - changes are not saved.\n";
        return;
    }
    out << "id,name,batch,quantity,min_stock,expiry\n";
    for (const Medicine& m : stock)
        out << m.id << ',' << m.name << ',' << m.batch << ',' << m.qty << ',' << m.minStock << ',' << m.expiry << '\n';
}

bool isNumber(const string& s) { return !s.empty() && s.size() <= 7 && s.find_first_not_of("0123456789") == string::npos; }

void loadFile() {
    ifstream in(FILE_NAME);
    if (!in) {
        cout << "No data file yet - starting with an empty inventory.\n";
        return;
    }
    string line;
    int skipped = 0;
    getline(in, line);  // header row
    while (getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        stringstream ss(line);
        string f[6], extra;
        bool ok = true;
        for (int i = 0; i < 6; i++)
            if (!getline(ss, f[i], ',')) ok = false;
        if (getline(ss, extra, ',')) ok = false;  // too many fields
        ok = ok && isNumber(f[0]) && isNumber(f[3]) && isNumber(f[4]) && !f[1].empty() && f[1].size() <= 24 &&
             !f[2].empty() && f[2].size() <= 24 && validDate(f[5]);
        if (ok) {
            Medicine m{stoi(f[0]), f[1], f[2], stoi(f[3]), stoi(f[4]), f[5]};
            ok = m.id >= 1 && m.id <= 999999 && m.qty <= 100000 && m.minStock <= 100000 && linearSearchId(stock, m.id) < 0;
            if (ok) stock.push_back(m);
        }
        if (!ok) skipped++;
    }
    cout << "Loaded " << stock.size() << " record(s)";
    if (skipped > 0) cout << " (" << skipped << " invalid or duplicate line(s) skipped)";
    cout << ".\n";
}

// ----------------------------------------------------------------- display
void printTable(const vector<Medicine>& v) {
    cout << left << setw(7) << "ID" << setw(26) << "Name" << setw(14) << "Batch" << right << setw(6) << "Qty" << setw(6)
         << "Min" << "  " << left << setw(12) << "Expiry" << right << setw(6) << "Days" << "  " << left << "Status\n"
         << string(88, '-') << "\n";
    for (const Medicine& m : v)
        cout << left << setw(7) << m.id << setw(26) << m.name << setw(14) << m.batch << right << setw(6) << m.qty
             << setw(6) << m.minStock << "  " << left << setw(12) << m.expiry << right << setw(6) << daysLeft(m.expiry)
             << "  " << left << status(m) << "\n";
    cout << "(" << v.size() << " record(s))\n";
}

// -------------------------------------------------------------------- CRUD
void addMedicine() {
    Medicine m;
    while (true) {  // duplicate-ID prevention
        m.id = readInt("ID (1-999999): ", 1, 999999);
        if (linearSearchId(stock, m.id) < 0) break;
        cout << "  That ID already exists.\n";
    }
    m.name = readText("Medicine name: ");
    m.batch = readText("Batch number: ");
    m.qty = readInt("Quantity (0-100000): ", 0, 100000);
    m.minStock = readInt("Minimum stock level (0-100000): ", 0, 100000);
    m.expiry = readDate("Expiry date (YYYY-MM-DD): ");
    stock.push_back(m);
    saveFile();
    cout << "Added.\n";
}

void viewStock() {
    if (stock.empty()) {
        cout << "Inventory is empty.\n";
        return;
    }
    int key = readInt("Sort by (0=none, 1=ID, 2=Name, 3=Expiry): ", 0, 3);
    vector<Medicine> v = stock;  // sort a copy; the stored order stays unchanged
    if (key > 0) {
        int algo = readInt("Algorithm (1=Selection sort, 2=Merge sort): ", 1, 2);
        compares = 0;
        if (algo == 1) selectionSort(v, key);
        else mergeSort(v, 0, (int)v.size(), key);
        cout << "(" << compares << " comparisons)\n";
    }
    printTable(v);
}

void updateMedicine() {
    int p = linearSearchId(stock, readInt("ID to update: ", 1, 999999));
    if (p < 0) {
        cout << "Not found.\n";
        return;
    }
    printTable({stock[p]});
    cout << "Enter the new values (the ID cannot be changed).\n";
    stock[p].name = readText("Medicine name: ");
    stock[p].batch = readText("Batch number: ");
    stock[p].qty = readInt("Quantity (0-100000): ", 0, 100000);
    stock[p].minStock = readInt("Minimum stock level (0-100000): ", 0, 100000);
    stock[p].expiry = readDate("Expiry date (YYYY-MM-DD): ");
    saveFile();
    cout << "Updated.\n";
}

void deleteMedicine() {
    int p = linearSearchId(stock, readInt("ID to delete: ", 1, 999999));
    if (p < 0) {
        cout << "Not found.\n";
        return;
    }
    printTable({stock[p]});
    if (readYes("Delete this batch? (y/n): ")) {
        stock.erase(stock.begin() + p);
        saveFile();
        cout << "Deleted.\n";
    }
}

// ------------------------------------------------------------------ search
void searchMenu() {
    cout << "1. Linear by ID   2. Linear by name   3. Binary by ID   4. Binary by name\n";
    int c = readInt("Choice: ", 1, 4);
    int id = 0;
    string name;
    if (c == 1 || c == 3) id = readInt("ID: ", 1, 999999);
    else name = readText("Exact name (any letter case): ");

    vector<Medicine> sorted = stock;  // binary search needs sorted data
    if (c >= 3) mergeSort(sorted, 0, (int)sorted.size(), c == 3 ? 1 : 2);

    vector<Medicine> found;
    compares = 0;  // count only the search itself
    if (c == 1) {
        int p = linearSearchId(stock, id);
        if (p >= 0) found.push_back(stock[p]);
    } else if (c == 3) {
        int p = binarySearchId(sorted, id);
        if (p >= 0) found.push_back(sorted[p]);
    } else {
        const vector<Medicine>& source = (c == 2) ? stock : sorted;
        vector<int> idx = (c == 2) ? linearSearchName(stock, name) : binarySearchName(sorted, name);
        for (int i : idx) found.push_back(source[i]);
    }
    if (found.empty()) cout << "Not found.\n";
    else printTable(found);
    cout << "Search used " << compares << " comparison(s).\n";
}

// ------------------------------------------------------------ expiry alerts
void showAlerts() {
    vector<Medicine> v = stock;
    mergeSort(v, 0, (int)v.size(), 3);  // soonest expiry first
    const char* title[3] = {"EXPIRED", "EXPIRES WITHIN 30 DAYS", "EXPIRES IN 31-90 DAYS"};
    cout << "Alerts for " << today << " (batches with quantity 0 are ignored)\n";
    for (int g = 0; g < 3; g++) {
        vector<Medicine> group;
        for (const Medicine& m : v) {
            if (m.qty == 0) continue;
            int d = daysLeft(m.expiry);
            int which = d < 0 ? 0 : d <= 30 ? 1 : d <= 90 ? 2 : 3;
            if (which == g) group.push_back(m);
        }
        cout << "\n" << title[g] << "\n";
        if (group.empty()) cout << "  None.\n";
        else printTable(group);
    }
}

// ------------------------------------------------------- FEFO stock issue
// First-Expired, First-Out: use the batch that expires first.
void issueFEFO() {
    string name = readText("Medicine name: ");
    int need = readInt("Quantity needed (1-100000): ", 1, 100000);

    vector<Medicine> usable;  // skip expired batches and empty batches
    for (const Medicine& m : stock)
        if (lower(m.name) == lower(name) && m.qty > 0 && daysLeft(m.expiry) >= 0) usable.push_back(m);
    if (usable.empty()) {
        cout << "No usable (not expired, in stock) batch of that medicine.\n";
        return;
    }
    mergeSort(usable, 0, (int)usable.size(), 3);  // earliest expiry first

    vector<pair<int, int>> plan;  // (batch ID, units to take)
    int remaining = need;
    for (const Medicine& m : usable) {
        if (remaining == 0) break;
        int take = remaining < m.qty ? remaining : m.qty;
        plan.push_back({m.id, take});
        remaining -= take;
        cout << "  Take " << take << " from batch " << m.batch << " (ID " << m.id << ", expires " << m.expiry << ")\n";
    }
    if (remaining > 0) cout << "  Shortage: " << remaining << " unit(s) cannot be supplied.\n";
    if (!readYes(remaining > 0 ? "Issue the available stock anyway? (y/n): " : "Confirm issue? (y/n): ")) {
        cout << "Cancelled. Stock unchanged.\n";
        return;
    }
    for (const auto& step : plan) stock[linearSearchId(stock, step.first)].qty -= step.second;
    saveFile();
    cout << "Stock issued.\n";
}

// --------------------------------------------------------------- low stock
// Usable quantity of a medicine = its non-expired batches added together.
// Reorder level = the largest min-stock value among its batches.
void showLowStock() {
    cout << "LOW STOCK (expired batches are not counted)\n";
    bool any = false;
    for (size_t i = 0; i < stock.size(); i++) {
        bool seen = false;  // report each medicine name only once
        for (size_t j = 0; j < i; j++)
            if (lower(stock[j].name) == lower(stock[i].name)) seen = true;
        if (seen) continue;

        long usable = 0;
        int reorder = 0;
        for (const Medicine& m : stock) {
            if (lower(m.name) != lower(stock[i].name)) continue;
            if (m.minStock > reorder) reorder = m.minStock;
            if (m.qty > 0 && daysLeft(m.expiry) >= 0) usable += m.qty;
        }
        if (usable <= reorder) {
            any = true;
            cout << "  " << left << setw(26) << stock[i].name << "usable " << setw(6) << usable << "reorder level " << setw(6)
                 << reorder << (usable == 0 ? "OUT OF STOCK" : "LOW") << "\n";
        }
    }
    if (!any) cout << "  All medicines are above their reorder level.\n";
}

// ------------------------------------------------------------ DAA analysis
void daaMenu() {
    cout << "\nTIME AND SPACE COMPLEXITY (n = number of records)\n"
            "Algorithm          Best        Average     Worst       Extra space\n"
            "-----------------------------------------------------------------\n"
            "Linear search      O(1)        O(n)        O(n)        O(1)\n"
            "Binary search      O(1)        O(log n)    O(log n)    O(1)   (data must be sorted)\n"
            "Selection sort     O(n^2)      O(n^2)      O(n^2)      O(1)   (not stable)\n"
            "Merge sort         O(n log n)  O(n log n)  O(n log n)  O(n)   (stable)\n"
            "Date validation    O(1)        O(1)        O(1)        O(1)\n"
            "Expiry alerts      O(n log n)  O(n log n)  O(n log n)  O(n)   (merge sort + scan)\n"
            "FEFO issue         O(n + k log k) where k = batches of that one medicine\n"
            "Low-stock report   O(n^2) with the simple loops used here\n";
    if (stock.size() < 2) {
        cout << "\nAdd at least 2 records to compare the two sorts on your own data.\n";
        return;
    }
    vector<Medicine> a = stock, b = stock;
    compares = 0;
    selectionSort(a, 3);
    long long selection = compares;
    compares = 0;
    mergeSort(b, 0, (int)b.size(), 3);
    long long merge = compares;
    bool same = true;
    for (size_t i = 0; i < a.size(); i++)
        if (a[i].id != b[i].id) same = false;
    cout << "\nSorting your " << stock.size() << " records by expiry date:\n"
         << "  Selection sort: " << selection << " comparisons  (always n(n-1)/2)\n"
         << "  Merge sort:     " << merge << " comparisons\n"
         << "  Same result: " << (same ? "yes" : "NO") << "\n"
         << "With more records the gap grows: n^2 against n log n.\n";
}

// -------------------------------------------------------------------- main
int main() {
    today = systemToday();
    loadFile();
    while (true) {
        cout << "\n=== Medicine Expiry and Stock Rotation Assistant === (reference date " << today << ")\n"
                "Inventory tool only - no medical or dosage advice.\n"
                " 1 Add batch        2 View / sort      3 Search\n"
                " 4 Update batch     5 Delete batch     6 Expiry alerts\n"
                " 7 Issue stock (FEFO)   8 Low stock    9 DAA analysis\n"
                "10 Set reference date (for demos)      0 Exit\n";
        switch (readInt("Choice: ", 0, 10)) {
            case 1: addMedicine(); break;
            case 2: viewStock(); break;
            case 3: searchMenu(); break;
            case 4: updateMedicine(); break;
            case 5: deleteMedicine(); break;
            case 6: showAlerts(); break;
            case 7: issueFEFO(); break;
            case 8: showLowStock(); break;
            case 9: daaMenu(); break;
            case 10: {
                string s = readLine("New date (YYYY-MM-DD, or 'today'): ");
                if (s == "today") today = systemToday();
                else if (validDate(s)) today = s;
                else cout << "Invalid date.\n";
                break;
            }
            case 0:
                saveFile();
                cout << "Saved. Goodbye.\n";
                return 0;
        }
    }
}