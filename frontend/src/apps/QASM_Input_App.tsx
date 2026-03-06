import { useState, useEffect, useRef } from 'react'

import QuantumCircuit from 'quantum-circuit';

import { QASMControls } from '../components/QASMControls';


export default function QASMInputApp() {
  const [qasmInput, setQasmInput] = useState<string>('')
  const [error, setError] = useState<string | null>(null)
  const [circuitSvg, setCircuitSvg] = useState<string>('')
  const svgContainerRef = useRef<HTMLDivElement>(null)

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
<div style={{ display: 'flex', height: '100vh', width: '100vw', overflow: 'hidden' }}>
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', minWidth: 0 }}>
        <div style={{ flex: 2, padding: 30 }}>
          <h1 className='pb-5'>Enter your QASM program</h1>
          <textarea
            rows={10}
            placeholder="paste your QASM here..."
            value={qasmInput}
            onChange={(e) => setQasmInput(e.target.value)}
            style={{ 
              width: '100%',
              height: '95%',
              flex: 1,
              fontFamily: 'Monaco, Courier New, monospace',
              fontSize: '18px',
              padding: '20px',
              margin: '20px',
              border: 'none',
              borderRadius: '12px',
              background: 'rgba(255, 255, 255, 0.95)',
              boxShadow: '0 8px 32px rgba(0,0,0,0.2)',
              resize: 'none',
              outline: 'none',
              color: '#2d3748'
            }}
          />
        </div>
        <div style={{ 
          flex: 1, 
          padding: 30, 
          display: 'flex',
          flexDirection: 'column',
          minHeight: 0
        }}>
          <div style={{
            flex: 1,
            overflow: 'auto',
            display: 'flex',
            justifyContent: 'center',
            alignItems: 'center',
            minHeight: 0
          }}>
            <div 
              ref={svgContainerRef}
              dangerouslySetInnerHTML={{ __html: circuitSvg }}
              style={{ minHeight: 50 }}
            />
            {!circuitSvg && !error && qasmInput.trim() && (
              <p>Finish your input to see the circuit...</p>
            )}
            {!circuitSvg && !qasmInput.trim() && (
              <p>Enter QASM code to see the circuit</p>
            )}
          </div>
          <p>Powered by <a target='_blank' href='https://github.com/quantastica/quantum-circuit'>Quantastica quantum-circuit</a></p>
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