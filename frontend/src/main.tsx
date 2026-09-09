import React, { useState } from 'react';
import { BrowserRouter, Routes, Route, useNavigate } from "react-router-dom";
import ReactDOM from 'react-dom/client';
import MBQC_App from './apps/MBQC';
import ZX_App from './apps/ZX_App';
import QASMInput_App from './apps/QASM_Input_App';
import Home from './home';
import './styles/graph.css';
import SimulatorApp from './apps/Simulator_App';
import TutorialApp from './apps/Tutorial';
import TutorialDetailApp from './apps/Tutorial/Detail';
import TutorialHelpButton from './components/TutorialHelpButton';
import { TutorialOverlayProvider } from './apps/Tutorial/TutorialOverlayContext';
import TutorialOverlay from './apps/Tutorial/TutorialOverlay';


function Launcher() {

  return (
    <React.StrictMode>
      <BrowserRouter>
        <TutorialOverlayProvider>
          <Routes>
            <Route path="/" element={<Home />} />
            <Route path="/MBQC" element={<MBQC_App />} />
            <Route path="/QASM" element={<QASMInput_App />} />
            <Route path="/ZX" element={<ZX_App />} />
            <Route path="/SIM" element={<SimulatorApp />} />
            <Route path="/TUTORIAL" element={<TutorialApp />} />
            <Route path="/TUTORIAL/:id" element={<TutorialDetailApp />} />
          </Routes>
          <TutorialHelpButton />
          <TutorialOverlay />
        </TutorialOverlayProvider>
      </BrowserRouter>
  </React.StrictMode>
  );
}

ReactDOM.createRoot(document.getElementById('root')!).render(<Launcher />);
