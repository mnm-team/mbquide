import { useState, useEffect, useRef } from 'react'

import QuantumCircuit from 'quantum-circuit';

import { QASMControls } from '../components/QASMControls';


export default function QASMInputApp() {
  const [qasmInput, setQasmInput] = useState<string>('')
  const [error, setError] = useState<string | null>(null)
  const [circuitSvg, setCircuitSvg] = useState<string>('')
  const svgContainerRef = useRef<HTMLDivElement>(null)
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

  useEffect(() => {
    if (!qasmInput.trim()) {
      setCircuitSvg('')
      setError(null)
      return
    }

    try {
      const circuit = new QuantumCircuit()
        circuit.importQASM(
          qasmInput,
          (_: any) => {
            setCircuitSvg('');
            setError(null);
          },
          false
        );
      const svg = circuit.exportSVG(true)
      setCircuitSvg(svg)
      setError(null)
    } catch (err: any) {
      setCircuitSvg('')
      setError(null)
    }
  }, [qasmInput])


  return (
    <div className="flex h-screen w-screen overflow-hidden">
      <div className="flex flex-1 flex-col min-w-0">
        <div className="flex-[2] p-[30px] z-10">
          <h1 className="pb-5">Enter your QASM program</h1>
          <textarea
            rows={10}
            placeholder="paste your QASM here..."
            value={qasmInput}
            onChange={(e) => setQasmInput(e.target.value)}
            className="w-full h-[90%] flex-1 font-mono text-[18px] p-5 m-5 border-none rounded-xl bg-white/95 shadow-[0_8px_32px_rgba(0,0,0,0.2)] resize-none outline-none text-[#2d3748]"
          />
        </div>

        <div className="flex-1 p-[30px] flex flex-col min-h-0">
          <div className="flex-1 overflow-auto min-h-0">
            <div className="flex justify-center items-center min-h-full min-w-full">
              <div
                ref={svgContainerRef}
                dangerouslySetInnerHTML={{ __html: circuitSvg }}
                className="min-h-[50px] min-w-[100px]"
              />
              {!circuitSvg && !error && qasmInput.trim() && (
                <p>Finish your input to see the circuit...</p>
              )}
              {!circuitSvg && !qasmInput.trim() && (
                <p>Enter QASM code to see the circuit</p>
              )}
            </div>
          </div>
          <p>
            Powered by{" "}
            <a target="_blank" href="https://github.com/quantastica/quantum-circuit">
              Quantastica quantum-circuit
            </a>
          </p>
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