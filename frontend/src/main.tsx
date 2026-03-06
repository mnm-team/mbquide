import React, { useState } from 'react';
import { BrowserRouter, Routes, Route, useNavigate } from "react-router-dom";
import ReactDOM from 'react-dom/client';
import MBQC_App from './apps/MBQC';
import ZX_App from './apps/ZX_App';
import QASMInput_App from './apps/QASM_Input_App';
import Home from './home';
import './styles/graph.css';
import SimulatorApp from './apps/Simulator_App';


function Launcher() {

  return (
    <React.StrictMode>
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/MBQC" element={<MBQC_App />} />
          <Route path="/QASM" element={<QASMInput_App />} />
          <Route path="/ZX" element={<ZX_App />} />
          <Route path="/SIM" element={<SimulatorApp />} />
        </Routes>
      </BrowserRouter>
  </React.StrictMode>
  );
}

ReactDOM.createRoot(document.getElementById('root')!).render(<Launcher />);
