import { useCallback, useState } from "react";
import { useNavigate } from "react-router-dom";
import { executeAPIOperation } from "../apps/MBQC/api/graphApi";
import {
  createTransformToZXOperation,
  createSimulateOperation,
  createGetFlowOperation,
} from "../apps/MBQC/api/operations";

import { ActionButton, ExampleButton } from "./Buttons";
import { ZXIcon, MBQCIcon, MeasurementIcon } from "./Icons";
import LoadingOverlay from "./LoadingOverlay";

type Props = {
  qasmInput: string;
  setQasmInput: (value: string) => void;
};

export function QASMControls({ qasmInput, setQasmInput }: Props) {
  const navigate = useNavigate();
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const toZX = useCallback(async () => {
    if (!qasmInput.trim()) {
      alert("Please enter some QASM before submitting.");
      return;
    }
    try {
      setLoading(true);
      setError(null);
      await executeAPIOperation("qasm", { qasm: qasmInput });
      navigate("/ZX");
    } catch (err: any) {
      setError("Failed to generate ZX diagram.");
    } finally {
      setLoading(false);
    }
  }, [qasmInput, navigate]);

  const toMBQC = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      await executeAPIOperation("qasm", { qasm: qasmInput });
      await executeAPIOperation("qasm", createTransformToZXOperation());
      navigate("/MBQC");
    } catch (err: any) {
      setError("Failed to generate MBQC diagram.");
    } finally {
      setLoading(false);
    }
  }, [qasmInput, navigate]);

  const toSimulator = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      await executeAPIOperation("qasm", { qasm: qasmInput });
      await executeAPIOperation("qasm", createTransformToZXOperation());
      await executeAPIOperation("graph", createGetFlowOperation());
      await executeAPIOperation("graph", createSimulateOperation());
      navigate("/SIM");
    } catch (err: any) {
      setError("Simulation failed.");
    } finally {
      setLoading(false);
    }
  }, [qasmInput, navigate]);

  const bellQasm = `OPENQASM 2.0;\nqreg q[2];\nh q[0];\ncx q[0],q[1];`;
  const hQasm = `OPENQASM 2.0;\nqreg q[1];\nh q[0];`;
  const toffoliQasm = `OPENQASM 2.0;\nqreg a[3];\nx a[0];\nx a[1];\nh a[2];\ncx a[1],a[2];\nrz(-pi/4) a[2];\ncx a[0],a[2];\nrz(pi/4) a[2];\ncx a[1],a[2];\nrz(-pi/4) a[1];\nrz(-pi/4) a[2];\ncx a[0],a[2];\ncx a[0],a[1];\nrz(-pi/4) a[1];\ncx a[0],a[1];\nrz(pi/4) a[0];\nrz(pi/4) a[2];\nh a[2];`;
  const DeJoQasm = `OPENQASM 2.0;\nqreg q[3];\nx q[2];\nh q[0];\nh q[1];\nh q[2];\n// Balanced oracle example: f(x0,x1)=x0 XOR x1\ncx q[0], q[2];\ncx q[1], q[2];\nh q[0];\nh q[1];\n`;

  return (
    <div className="w-[350px] shrink-0 bg-slate-50 border-r border-gray-200 flex flex-col h-full font-sans">
      {/* Header */}
      <div className="px-4 pt-5 pb-4 border-b border-gray-200">
        <div className="flex items-center gap-2 mb-1">
          <div className="w-1.5 h-1.5 rounded-full bg-blue-500" />
          <span className="text-sm text-blue-500 tracking-widest uppercase font-semibold">
            Controls
          </span>
        </div>
        <p className="text-xs text-gray-500 m-0">QASM 2.0</p>
      </div>

      {/* Scrollable content */}
      <div className="flex-1 overflow-y-auto px-3 py-4">
        {/* Actions */}
        <div className="mb-6">
          <div className="flex items-center justify-between mb-3">
            <span className="text-[11px] font-semibold text-gray-500 tracking-widest uppercase">
              Actions
            </span>
            <LoadingOverlay isLoading={loading} />
          </div>

          <div className="flex flex-col gap-2.5">
            <ActionButton
              onClick={toZX}
              disabled={!qasmInput}
              label="ZX Diagram"
              sublabel="Transform the Circuit to ZX"
              icon={<ZXIcon />}
              arrow={true}
            />
            <ActionButton
              onClick={toMBQC}
              disabled={!qasmInput}
              label="MBQC Diagram"
              sublabel="Transform the Circuit to MBQC"
              icon={<MBQCIcon />}
              arrow={true}
            />
            <ActionButton
              onClick={toSimulator}
              disabled={!qasmInput}
              label="Run Simulation"
              sublabel="Simulate the MBQC Pattern"
              icon={<MeasurementIcon />}
              arrow={true}
            />
          </div>
        </div>

        {/* Error */}
        {error && (
          <div className="px-3 py-2.5 bg-red-50 border border-red-200 rounded-lg mb-5 text-xs text-red-600">
            {error}
          </div>
        )}

        {/* Divider */}
        <div className="border-t border-gray-200 mb-5" />

        {/* Examples */}
        <div>
          <span className="block text-[11px] font-semibold text-gray-500 tracking-widest uppercase mb-3">
            Example Circuits
          </span>

          <div className="flex flex-col gap-2">
            <ExampleButton
              onClick={() => setQasmInput(hQasm)}
              label="Coin Toss"
              description="Uniform superposition"
              qubits="1q"
            />
            <ExampleButton
              onClick={() => setQasmInput(bellQasm)}
              label="Bell State"
              description="Maximal entanglement"
              qubits="2q"
            />
            <ExampleButton
              onClick={() => setQasmInput(toffoliQasm)}
              label="Toffoli Gate"
              description="CCX decomposition"
              qubits="3q"
            />
            <ExampleButton
              onClick={() => setQasmInput(DeJoQasm)}
              label="Deutsch Josza"
              description="Deutsch–Jozsa w/ balanced function"
              qubits="3q"
            />
          </div>
        </div>
      </div>

      {/* Footer */}
      <div className="px-4 py-3 border-t border-gray-200 text-[11px] text-gray-400 tracking-widest">
        © MNM-Team
      </div>
    </div>
  );
}