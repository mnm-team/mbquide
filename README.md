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
- **Crow**: https://crowcpp.org/
- **libasio-dev**
- **Eigen** – https://gitlab.com/libeigen/eigen/  
  Copy the Eigen headers into the `backend/include/` directory.

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
- Node.js + npm

Install dependencies:
```bash
cd frontend
npm install
```


## Running the application


### Run the backend 
from the project root:
```
./build/Server
```
The backend server will start on:
```
http://localhost:18080
```

### Run the Frontend:
```bash
cd frontend
npm run dev
```
The development server will start at:
```
http://localhost:5173/
```
and connect to the backend automatically.


