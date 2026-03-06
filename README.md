# MBQuIDE

An interactive graphical editor for measurement-based quantum computing

---

## Project Structure
```
MBQuIDE/
│
├── backend/    # C++ backend server
├── frontend/   # Web-based UI
└── README.md
```

## Installation

### Backend
#### Requirements:
- **CMake ≥ 3.14**
- C++ compiler: Must support **C++17**
- Install **boost:**
```bash
apt install libboost-graph-dev
```

#### Build:

From the project root:
```bash
cmake -S backend -B backend/build
```
 
Then compile with: 
```bash
cmake --build backend/build
```

### Frontend
#### Requirements:
- Node.js ≥ 18 with npm ≥ 9

Install dependencies:
```bash
cd frontend
npm install
```


## Running the application

Backend and Frontend can be started with the script:
```bash
bash start.sh
```

The backend server will start on port 18080 while the frontend server will run on port 5173.


