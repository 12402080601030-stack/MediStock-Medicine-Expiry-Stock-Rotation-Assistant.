# MediStock 💊
### Medicine Expiry & Stock Rotation Assistant

MediStock is a C++17-based project that helps manage medicine inventory, track expiry dates, monitor stock levels, and organize medicine batches efficiently.

## ✨ Features

- **Inventory Management:** Add, view, update, and delete medicine batches.
- **Expiry Alerts:** Identify expired medicines and batches nearing expiry.
- **Low-Stock Monitoring:** Find medicines that need restocking.
- **Smart Stock Issuing:** Uses the First-Expired, First-Out (FEFO) method.
- **Searching Algorithms:** Search by medicine ID or name using linear and binary search.
- **Sorting Algorithms:** Sort inventory by ID, name, or expiry date using selection sort and merge sort.
- **CSV File Handling:** Save and load inventory data between program runs.
- **DAA Analysis:** Display time and space complexity and compare sorting performance.

## 🛠️ Built With

- C++17
- Standard Template Library (STL)
- File Handling (CSV)
- Data Structures and Algorithms (DAA)

## 🚀 Getting Started

### Prerequisites

- A C++ compiler that supports C++17, such as GCC (g++).
- A terminal or an IDE such as Visual Studio Code.

### Installation

1. Clone this repository or download the project files.
2. Open the project folder in your terminal.
3. Compile the program:

   ```bash
   g++ -std=c++17 -Wall -Wextra main.cpp -o main
   ```

4. Run the program:

   **Windows PowerShell:**
   ```powershell
   .\main.exe
   ```

   **Linux/macOS:**
   ```bash
   ./main
   ```

## 📂 Project Structure

```text
MediStock/
├── main.cpp
├── medicines.csv
└── README.md
```

- `main.cpp` — Contains the program logic and algorithms.
- `medicines.csv` — Stores medicine inventory records. The program creates or updates this file when saving data.
- `README.md` — Project documentation.

## 📄 CSV Data Format

The program uses the following columns to store medicine batch information:

```csv
id,name,batch,quantity,min_stock,expiry
101,Paracetamol,PCM101,100,20,2027-12-31
102,Amoxicillin,AMX102,15,20,2026-11-10
```

Each row represents one medicine batch. Expiry dates must use the `YYYY-MM-DD` format.

If no CSV file exists when the program starts, it begins with an empty inventory.

## 🧠 Algorithms Used

| Algorithm | Purpose | Time Complexity |
|---|---|---|
| Linear Search | Search by ID or name | O(n) |
| Binary Search | Search sorted records | O(log n) |
| Selection Sort | Sort inventory records | O(n²) |
| Merge Sort | Efficiently sort inventory records | O(n log n) |
| FEFO | Issue stock from the earliest-expiring usable batches | O(n + k log k) |

Here, `n` is the number of records and `k` is the number of usable batches of the selected medicine.

## 🎯 Project Objective

The objective of MediStock is to demonstrate the practical application of searching, sorting, file handling, and algorithm analysis in a real-world inventory management scenario.

## ⚠️ Disclaimer

MediStock is an educational inventory management tool. It does not provide medical advice, dosage recommendations, or treatment guidance.

## 👩‍💻 Author

Developed as a C++ and Data Structures and Algorithms project.
