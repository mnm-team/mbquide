import { useState, useEffect, useRef } from 'react'

import QuantumCircuitViewer from '../components/QuantumCircuitViewer';

import { QASMControls } from '../components/QASMControls';


export default function QASMInputApp() {
  const [qasmInput, setQasmInput] = useState<string>('')
  const STORAGE_KEY = 'mbquide:qasmInput'

  // Restore QASM text when reloading the site.
  useEffect(() => {
    try {
      const saved = window.localStorage.getItem(STORAGE_KEY)
      if (saved !== null) setQasmInput(saved)
    } catch {
      // Ignore storage errors
    }
  }, [])

  // Save QASM during typing
  useEffect(() => {
    const timeoutId = window.setTimeout(() => {
      try {
        if (qasmInput.trim()) window.localStorage.setItem(STORAGE_KEY, qasmInput)
        else window.localStorage.removeItem(STORAGE_KEY)
      } catch {
        // Ignore storage errors.
      }
    }, 250)
    return () => window.clearTimeout(timeoutId)
  }, [qasmInput])

  return (
    <div className="flex h-screen w-screen overflow-hidden">
      <div className="flex h-full flex-1 flex-col min-w-0">
        <div className="flex-[1] p-[30px] z-10">
          <h1 className="pb-5">Enter your QASM program</h1>
          <textarea
            rows={10}
            placeholder="paste your QASM here..."
            value={qasmInput}
            onChange={(e) => setQasmInput(e.target.value)}
            className="w-full h-[90%] flex-1 font-mono text-[18px] p-5 m-5 border-none rounded-xl bg-white/95 shadow-[0_8px_32px_rgba(0,0,0,0.2)] resize-none outline-none text-[#2d3748]"
          />
        </div>

        <div className="flex-[3] p-[30px] flex flex-col min-h-0">
          <div className="flex-1 overflow-auto min-h-0">
            <div className="w-fit mx-auto min-h-full flex items-center">
              <QuantumCircuitViewer qasmInput={qasmInput} />
            </div>
          </div>
        </div>

      </div>

      {/* Controls */}
      <QASMControls
        qasmInput={qasmInput}
        setQasmInput={setQasmInput}
      />
    </div>
  )
}