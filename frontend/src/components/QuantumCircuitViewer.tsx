import { useState, useRef, useMemo } from "react";

// ─── TYPES ───────────────────────────────────────────────────────────────────

type QASMOp =
  | { type: "gate";    name: string; params: string[]; targets: string[] }
  | { type: "measure"; qubit: string; creg: string }
  | { type: "barrier"; targets: string[] };

interface ParsedCircuit {
  qubits: string[];
  qregs:  Record<string, number>;
  cregs:  Record<string, number>;
  ops:    QASMOp[];
}

interface LayoutColumn {
  col:          number;
  op:           QASMOp;
  qubitIndices: number[];
}

interface CircuitLayout {
  columns: LayoutColumn[];
  qubits:  string[];
}

interface GateColor {
  bg:     string;
  border: string;
  text:   string;
  dot:    string;   // control-dot fill (slightly darker than border)
}

interface CircuitStats {
  qubits:       number;
  gates:        number;
  depth:        number;
}

// ─── QASM PARSER ─────────────────────────────────────────────────────────────

function parseQASM(qasm: string): ParsedCircuit {
  const lines = qasm
    .split("\n")
    .map((l) => l.trim())
    .filter((l) => l && !l.startsWith("//") && !l.startsWith("OPENQASM") && !l.startsWith("include"));

  const qregs: Record<string, number> = {};
  const cregs: Record<string, number> = {};
  const ops: QASMOp[] = [];

  for (const line of lines) {
    const qregM = line.match(/^qreg\s+(\w+)\[(\d+)\]/);
    if (qregM) { qregs[qregM[1]] = parseInt(qregM[2], 10); continue; }

    const cregM = line.match(/^creg\s+(\w+)\[(\d+)\]/);
    if (cregM) { cregs[cregM[1]] = parseInt(cregM[2], 10); continue; }

    const measM = line.match(/^measure\s+(\w+)\[(\d+)\]\s*->\s*(\w+)\[(\d+)\]/);
    if (measM) {
      ops.push({
        type:  "measure",
        qubit: `${measM[1]}[${measM[2]}]`,
        creg:  `${measM[3]}[${measM[4]}]`,
      });
      continue;
    }

    const barrierM = line.match(/^barrier\s+(.*)/);
    if (barrierM) {
      ops.push({ type: "barrier", targets: barrierM[1].replace(/;$/, "").split(",").map((s) => s.trim()) });
      continue;
    }

    const gateM = line.match(/^([a-zA-Z][a-zA-Z0-9_]*(?:\([^)]*\))?)\s+(.*)/);
    if (gateM) {
      let name = gateM[1];
      const paramsM = name.match(/^([a-zA-Z][a-zA-Z0-9_]*)\(([^)]*)\)$/);
      let params: string[] = [];
      if (paramsM) { name = paramsM[1]; params = paramsM[2].split(",").map((s) => s.trim()); }
      const targets = gateM[2].replace(/;$/, "").split(",").map((s) => s.trim()).filter(Boolean);
      if (targets.length > 0) ops.push({ type: "gate", name, params, targets });
    }
  }

  const qubits: string[] = [];
  for (const [reg, size] of Object.entries(qregs)) {
    for (let i = 0; i < size; i++) qubits.push(`${reg}[${i}]`);
  }

  return { qubits, qregs, cregs, ops };
}

// ─── LAYOUT ENGINE ───────────────────────────────────────────────────────────

function layoutCircuit(parsed: ParsedCircuit): CircuitLayout {
  const { qubits, ops } = parsed;
  if (!qubits.length) return { columns: [], qubits };

  const colOf: Record<string, number> = {};
  qubits.forEach((q) => { colOf[q] = 0; });

  const columns: LayoutColumn[] = [];

  for (const op of ops) {
    if (op.type === "gate" || op.type === "measure") {
      const targets = op.type === "measure" ? [op.qubit] : op.targets;
      const idxs = targets.map((t) => qubits.indexOf(t)).filter((i) => i >= 0);
      if (!idxs.length) continue;
      const col = Math.max(...idxs.map((i) => colOf[qubits[i]]));
      columns.push({ col, op, qubitIndices: idxs });
      idxs.forEach((i) => { colOf[qubits[i]] = col + 1; });
    } else if (op.type === "barrier") {
      const col = qubits.length ? Math.max(...qubits.map((q) => colOf[q])) : 0;
      columns.push({ col, op, qubitIndices: qubits.map((_, i) => i) });
      qubits.forEach((q) => { colOf[q] = col + 1; });
    }
  }

  return { columns, qubits };
}

// ─── GATE PALETTE — light-mode, ink-on-paper feel ────────────────────────────
// Each gate family has a saturated-but-legible color.
// bg: pale tint; border: mid saturated; text: deep saturated; dot: for ctrl node.

const GATE_COLORS: Record<string, GateColor> = {
  h:       { bg: "#dbeafe", border: "#3b82f6", text: "#1d4ed8", dot: "#1e40af" },
  x:       { bg: "#fee2e2", border: "#ef4444", text: "#b91c1c", dot: "#991b1b" },
  y:       { bg: "#dcfce7", border: "#22c55e", text: "#15803d", dot: "#166534" },
  z:       { bg: "#fef9c3", border: "#eab308", text: "#854d0e", dot: "#713f12" },
  s:       { bg: "#e0f2fe", border: "#0ea5e9", text: "#0369a1", dot: "#075985" },
  t:       { bg: "#ede9fe", border: "#8b5cf6", text: "#6d28d9", dot: "#5b21b6" },
  sdg:     { bg: "#e0f2fe", border: "#0ea5e9", text: "#0369a1", dot: "#075985" },
  tdg:     { bg: "#ede9fe", border: "#8b5cf6", text: "#6d28d9", dot: "#5b21b6" },
  cx:      { bg: "#fdf4ff", border: "#a855f7", text: "#7e22ce", dot: "#6b21a8" },
  cy:      { bg: "#fdf4ff", border: "#a855f7", text: "#7e22ce", dot: "#6b21a8" },
  cz:      { bg: "#fdf4ff", border: "#a855f7", text: "#7e22ce", dot: "#6b21a8" },
  ccx:     { bg: "#fff1f2", border: "#f43f5e", text: "#be123c", dot: "#9f1239" },
  swap:    { bg: "#f0fdfa", border: "#14b8a6", text: "#0f766e", dot: "#0d9488" },
  rx:      { bg: "#fff7ed", border: "#f97316", text: "#c2410c", dot: "#9a3412" },
  ry:      { bg: "#f0fdf4", border: "#4ade80", text: "#15803d", dot: "#166534" },
  rz:      { bg: "#fffbeb", border: "#fbbf24", text: "#92400e", dot: "#78350f" },
  u:       { bg: "#f8fafc", border: "#64748b", text: "#334155", dot: "#1e293b" },
  u1:      { bg: "#f8fafc", border: "#64748b", text: "#334155", dot: "#1e293b" },
  u2:      { bg: "#f8fafc", border: "#64748b", text: "#334155", dot: "#1e293b" },
  u3:      { bg: "#f8fafc", border: "#64748b", text: "#334155", dot: "#1e293b" },
  cu1:     { bg: "#fdf4ff", border: "#a855f7", text: "#7e22ce", dot: "#6b21a8" },
  measure: { bg: "rgb(209, 209, 209)", border: "#979797", text: "#8b8b8b", dot: "#848484" },
  id:      { bg: "#f8fafc", border: "#94a3b8", text: "#475569", dot: "#334155" },
  reset:   { bg: "#fef2f2", border: "#f87171", text: "#7f1d1d", dot: "#6b1a1a" },
  default: { bg: "#f8fafc", border: "#94a3b8", text: "#334155", dot: "#1e293b" },
};

const GATE_DISPLAY: Record<string, string> = {
  h: "H", x: "X", y: "Y", z: "Z",
  s: "S", t: "T", sdg: "S†", tdg: "T†",
  cx: "CX", cy: "CY", cz: "CZ",
  ccx: "CCX", swap: "SW",
  rx: "Rx", ry: "Ry", rz: "Rz",
  u: "U", u1: "U₁", u2: "U₂", u3: "U₃",
  cu1: "CR", id: "I", reset: "RST", measure: "M",
};

function getGateColor(name: string): GateColor {
  return GATE_COLORS[name.toLowerCase()] ?? GATE_COLORS.default;
}

function getGateDisplay(name: string): string {
  return GATE_DISPLAY[name.toLowerCase()] ?? name.toUpperCase().slice(0, 3);
}

// ─── LAYOUT CONSTANTS ────────────────────────────────────────────────────────

const WIRE_Y_START   = 52;
const WIRE_Y_GAP     = 75;
const LABEL_W        = 88;
const COL_W          = 70;
const GATE_W         = 55;
const GATE_H         = 50;
const PADDING_R      = 30;
const BASE_FONT_SIZE = 16;

const colX  = (col: number): number => LABEL_W + col * COL_W;

// ─── SUB-COMPONENTS ──────────────────────────────────────────────────────────

interface GateBoxProps {
  name:      string;
  x:         number;
  y:         number;
  w?:        number;
  h?:        number;
  color:     GateColor;
  params?: string[];
  isMeasure?: boolean;
}

function GateBox({ name, x, y, w = GATE_W, h = GATE_H, color, params, isMeasure = false }: GateBoxProps) {
  const cx = x + w / 2;
  const cy = y + h / 2;
  
  const base = getGateDisplay(name);

  const formatParam = (p: string) => p.replace(/pi/g, "π");

  return (
    <g>
      <rect
        x={x} y={y} width={w} height={h} rx={7}
        fill={color.bg}
        stroke={color.border}
        strokeWidth={1.5}
      />
      {isMeasure ? (
        <g transform={`translate(${cx}, ${cy})`}>
            <path
            d={`M -9 5 A 10 9 0 0 1 9 5`}
            stroke={color.text}
            strokeWidth={1.5}
            fill="none"
            />
            <line
            x1={0}
            y1={5}
            x2={7}
            y2={-5}
            stroke={color.text}
            strokeWidth={1.5}
            strokeLinecap="round"
            />
            <circle cx={0} cy={5} r={1.5} fill={color.text} />
        </g>
        ) : (
        <>
            <text
            x={cx}
            y={cy}
            textAnchor="middle"
            dominantBaseline="central"
            fontSize={BASE_FONT_SIZE}
            fontWeight={700}
            fontFamily="ui-monospace, 'JetBrains Mono', 'Fira Code', monospace"
            fill={color.text}
            letterSpacing={-0.3}
            >
            {base}
            </text>

            {params &&
            params.length > 0 &&
            ["rx", "ry", "rz", "u", "u1", "u2", "u3"].includes(name.toLowerCase()) && (
                <text
                x={cx}
                y={y + h + 14}
                textAnchor="middle"
                fontSize={BASE_FONT_SIZE-4}
                fontWeight={500}
                fontFamily="ui-monospace, 'JetBrains Mono', 'Fira Code', monospace"
                fill={color.text}
                >
                {params.map(formatParam).join(",")}
                </text>
            )}
        </>
        )}
    </g>
  );
}

// ─── CIRCUIT SVG ─────────────────────────────────────────────────────────────

interface CircuitSVGProps {
  layout:      CircuitLayout;
  hoveredCol:  number | null;
  onHoverCol:  (col: number | null) => void;
}

function CircuitSVG({ layout, hoveredCol, onHoverCol }: CircuitSVGProps) {
  
  const wireY = (qi: number): number => WIRE_Y_START + (qubits.length - 1 - qi) * WIRE_Y_GAP;

  const { columns, qubits } = layout;
  if (!qubits.length) return null;

  const numCols = columns.length > 0 ? Math.max(...columns.map((c) => c.col)) + 1 : 0;
  const SVG_W = LABEL_W + numCols * COL_W + PADDING_R;
  const SVG_H = WIRE_Y_START + (qubits.length - 1) * WIRE_Y_GAP + 48;

  return (
    <svg
      width={SVG_W}
      height={SVG_H}
      viewBox={`0 0 ${SVG_W} ${SVG_H}`}
      style={{ display: "block", minWidth: SVG_W }}
    >
      {/* ── Hovered column highlight ───────────────────────────── */}
      {hoveredCol !== null && (
        <rect
          x={colX(hoveredCol) + 2}
          y={wireY(0) - 26}
          width={COL_W - 4}
          height={wireY(qubits.length - 1) - wireY(0) + 52}
          rx={8}
          fill="#3b82f6"
          opacity={0.07}
        />
      )}

      {/* ── Grid dots ─────────────────────────────────────────── */}
      {qubits.map((_, qi) =>
        Array.from({ length: Math.max(numCols, 1) }, (_, ci) => (
          <circle
            key={`dot-${qi}-${ci}`}
            cx={colX(ci) + COL_W / 2}
            cy={wireY(qi)}
            r={1.5}
            fill="#cbd5e1"
            opacity={0.6}
          />
        ))
      )}

      {/* ── Wires ─────────────────────────────────────────────── */}
      {qubits.map((_, qi) => (
        <line
          key={`wire-${qi}`}
          x1={LABEL_W - 6}
          y1={wireY(qi)}
          x2={SVG_W - PADDING_R + 6}
          y2={wireY(qi)}
          stroke="#94a3b8"
          strokeWidth={1.5}
        />
      ))}

      {/* ── Qubit labels ──────────────────────────────────────── */}
      {qubits.map((q, qi) => (
        <g key={`label-${qi}`}>
          <rect
            x={2}
            y={wireY(qi) - 13}
            width={LABEL_W - 10}
            height={26}
            rx={6}
            fill="#f1f5f9"
            stroke="#cbd5e1"
            strokeWidth={1}
          />
          <text
            x={LABEL_W - 16}
            y={wireY(qi)}
            textAnchor="end"
            dominantBaseline="central"
            fontSize={11}
            fontFamily="ui-monospace, 'JetBrains Mono', monospace"
            fill="#1e40af"
            fontWeight={600}
          >
            |{q}⟩
          </text>
        </g>
      ))}

      {/* ── Gates ─────────────────────────────────────────────── */}
      {columns.map((item, idx) => {
        const { col, op, qubitIndices } = item;
        const cx = colX(col) + COL_W / 2;

        // Barrier
        if (op.type === "barrier") {
          return (
            <g key={idx}>
              <line
                x1={cx} y1={wireY(0) - 18}
                x2={cx} y2={wireY(qubits.length - 1) + 18}
                stroke="#64748b"
                strokeWidth={1.2}
                strokeDasharray="4 3"
                opacity={0.5}
              />
            </g>
          );
        }

        // Measure
        if (op.type === "measure") {
          const qi = qubitIndices[0];
          const color = getGateColor("measure");
          const x0 = cx - GATE_W / 2;
          const y0 = wireY(qi) - GATE_H / 2;
          return (
            <g
              key={idx}
              onMouseEnter={() => onHoverCol(col)}
              onMouseLeave={() => onHoverCol(null)}
            >
              <GateBox name="measure" x={x0} y={y0} color={color} isMeasure />
            </g>
          );
        }

        // Gate
        if (op.type === "gate") {
          const name = op.name.toLowerCase();
          const color = getGateColor(name);
          const x0 = cx - GATE_W / 2;

          // SWAP
          if (name === "swap" && qubitIndices.length === 2) {
            const [qi1, qi2] = qubitIndices;
            return (
              <g key={idx} onMouseEnter={() => onHoverCol(col)} onMouseLeave={() => onHoverCol(null)}>
                <line
                  x1={cx} y1={wireY(qi1)}
                  x2={cx} y2={wireY(qi2)}
                  stroke={color.border} strokeWidth={1.5}
                />
                {([qi1, qi2] as number[]).map((qi) => (
                  <g key={qi}>
                    <line x1={cx - 8} y1={wireY(qi) - 8} x2={cx + 8} y2={wireY(qi) + 8}
                      stroke={color.border} strokeWidth={2.2} strokeLinecap="round" />
                    <line x1={cx + 8} y1={wireY(qi) - 8} x2={cx - 8} y2={wireY(qi) + 8}
                      stroke={color.border} strokeWidth={2.2} strokeLinecap="round" />
                  </g>
                ))}
              </g>
            );
          }

          // Single-qubit
          if (qubitIndices.length === 1) {
            const qi = qubitIndices[0];
            return (
              <g key={idx} onMouseEnter={() => onHoverCol(col)} onMouseLeave={() => onHoverCol(null)}>
                <GateBox name={name} x={x0} y={wireY(qi) - GATE_H / 2} params={op.params} color={color} />
              </g>
            );
          }

          // Controlled gate
          const controlIdxs = qubitIndices.slice(0, -1);
          const targetIdx   = qubitIndices[qubitIndices.length - 1];
          const allSorted   = [...qubitIndices].sort((a, b) => a - b);
          const isCNOT      = name === "cx" || name === "ccx";

          return (
            <g key={idx} onMouseEnter={() => onHoverCol(col)} onMouseLeave={() => onHoverCol(null)}>
              {/* Connector spine */}
              <line
                x1={cx} y1={wireY(allSorted[0])}
                x2={cx} y2={wireY(allSorted[allSorted.length - 1])}
                stroke={color.border} strokeWidth={1.5} opacity={0.7}
              />
              {/* Control dots */}
              {controlIdxs.map((qi) => (
                <circle
                  key={qi}
                  cx={cx} cy={wireY(qi)} r={6}
                  fill={color.dot}
                />
              ))}
              {/* Target */}
              {isCNOT ? (
                <g>
                  <circle
                    cx={cx} cy={wireY(targetIdx)} r={15}
                    fill={color.bg}
                    stroke={color.border}
                    strokeWidth={1.5}
                  />
                  <line x1={cx - 9} y1={wireY(targetIdx)} x2={cx + 9} y2={wireY(targetIdx)}
                    stroke={color.text} strokeWidth={2} strokeLinecap="round" />
                  <line x1={cx} y1={wireY(targetIdx) - 9} x2={cx} y2={wireY(targetIdx) + 9}
                    stroke={color.text} strokeWidth={2} strokeLinecap="round" />
                </g>
              ) : name === "cz" ? (
                // CZ: second control dot
                <circle cx={cx} cy={wireY(targetIdx)} r={6} fill={color.dot} />
              ) : (
                <GateBox
                  name={name.replace(/^c+/, "")}
                  x={x0}
                  y={wireY(targetIdx) - GATE_H / 2}
                  color={getGateColor(name.replace(/^c+/, ""))}
                />
              )}
            </g>
          );
        }

        return null;
      })}

      {/* ── Wire end caps ─────────────────────────────────────── */}
      {qubits.map((_, qi) => (
        <circle
          key={`cap-${qi}`}
          cx={SVG_W - PADDING_R + 6}
          cy={wireY(qi)}
          r={3}
          fill="#94a3b8"
        />
      ))}
    </svg>
  );
}

// ─── STAT CHIP ────────────────────────────────────────────────────────────────

interface StatChipProps {
  label:   string;
  value:   number;
  accent:  string; // Tailwind text color class
  bg:      string; // Tailwind bg color class
  border:  string; // Tailwind border color class
}

function StatChip({ label, value, accent, bg, border }: StatChipProps) {
  return (
    <div className={`flex items-baseline gap-2 px-3 py-1.5 rounded-lg border ${bg} ${border}`}>
      <span className="text-xs font-mono text-slate-400">{label}</span>
      <span className={`text-base font-bold font-mono ${accent}`}>{value}</span>
    </div>
  );
}

// ─── LEGEND ITEM ─────────────────────────────────────────────────────────────

interface LegendItemProps {
  name:  string;
  label: string;
}

function LegendItem({ name, label }: LegendItemProps) {
  const c = getGateColor(name);
  return (
    <div className="flex items-center gap-1.5">
      <div
        className="w-3 h-3 rounded-sm border flex-shrink-0"
        style={{ background: c.bg, borderColor: c.border }}
      />
      <span className="text-xs font-mono text-slate-400">{label}</span>
    </div>
  );
}

// ─── EMPTY STATE ─────────────────────────────────────────────────────────────

function EmptyState({ message }: { message: string }) {
  return (
    <div className="flex flex-col items-center justify-center gap-3 min-h-[160px]">
      <svg width="38" height="38" viewBox="0 0 38 38" fill="none">
        <circle cx="19" cy="19" r="17" stroke="#cbd5e1" strokeWidth="1.5" />
        <path
          d="M10 19 Q19 7 28 19 Q19 31 10 19Z"
          stroke="#3b82f6" strokeWidth="1.3" fill="none" opacity="0.5"
        />
        <circle cx="19" cy="19" r="3" fill="#3b82f6" opacity="0.4" />
      </svg>
      <p className="text-xs font-mono text-slate-400">{message}</p>
    </div>
  );
}

// ─── MAIN COMPONENT ──────────────────────────────────────────────────────────

interface QuantumCircuitViewerProps {
  qasmInput: string;
}

export default function QuantumCircuitViewer({ qasmInput }: QuantumCircuitViewerProps) {
  const [hoveredCol, setHoveredCol] = useState<number | null>(null);
  const containerRef = useRef<HTMLDivElement>(null);

  const parsed = useMemo<ParsedCircuit | null>(() => {
    if (!qasmInput.trim()) return null;
    try { return parseQASM(qasmInput); } catch { return null; }
  }, [qasmInput]);

  const layout = useMemo<CircuitLayout | null>(() => {
    if (!parsed) return null;
    try { return layoutCircuit(parsed); } catch { return null; }
  }, [parsed]);

  const stats = useMemo<CircuitStats | null>(() => {
    if (!parsed || !layout) return null;
    return {
      qubits:       parsed.qubits.length,
      gates:        parsed.ops.filter((o) => o.type === "gate").length,
      depth:        layout.columns.length > 0 ? Math.max(...layout.columns.map((c) => c.col)) + 1 : 0,
    };
  }, [parsed, layout]);

  // ── Determine which gate families are present (for legend) ────────────────
  const presentGates = useMemo<Set<string>>(() => {
    if (!parsed) return new Set();
    const s = new Set<string>();
    parsed.ops.forEach((op) => {
      if (op.type === "gate")    s.add(op.name.toLowerCase());
      if (op.type === "measure") s.add("measure");
    });
    return s;
  }, [parsed]);

  const LEGEND_ENTRIES: { key: string; label: string }[] = [
    { key: "h",       label: "Hadamard" },
    { key: "x",       label: "Pauli-X"  },
    { key: "y",       label: "Pauli-Y"  },
    { key: "z",       label: "Pauli-Z"  },
    { key: "s",       label: "Phase S"  },
    { key: "t",       label: "Phase T"  },
    { key: "cx",      label: "CNOT"     },
    { key: "cz",      label: "CZ"       },
    { key: "ccx",     label: "Toffoli"  },
    { key: "swap",    label: "SWAP"     },
    { key: "rx",      label: "Rx"    },
    { key: "ry",      label: "Ry"    },
    { key: "rz",      label: "Rz"    },
    { key: "measure", label: "Measure(ignored)"  },
  ];

  const visibleLegend = LEGEND_ENTRIES.filter(({ key }) => presentGates.has(key));

  // ── Render ────────────────────────────────────────────────────────────────
  if (!qasmInput.trim()) {
    return <EmptyState message="enter QASM to visualize circuit" />;
  }

  if (!layout || !layout.qubits.length) {
    return <EmptyState message="no qubits found — check qreg declarations" />;
  }

  return (
    <div className="flex flex-col gap-3">

      {/* Stats bar */}
      {stats && (
        <div className="flex flex-wrap gap-2">
          <StatChip
            label="qubits"
            value={stats.qubits}
            accent="text-blue-600"
            bg="bg-blue-50"
            border="border-blue-200"
          />
          <StatChip
            label="gates"
            value={stats.gates}
            accent="text-violet-600"
            bg="bg-violet-50"
            border="border-violet-200"
          />
          <StatChip
            label="depth"
            value={stats.depth}
            accent="text-amber-600"
            bg="bg-amber-50"
            border="border-amber-200"
          />
        </div>
      )}

      {/* Circuit diagram */}
      <div
        ref={containerRef}
        className="overflow-x-auto overflow-y-hidden rounded-xl border border-slate-200 bg-slate-50 px-4 py-5 shadow-[inset_0_1px_3px_rgba(0,0,0,0.06)]"
      >
        <CircuitSVG
          layout={layout}
          hoveredCol={hoveredCol}
          onHoverCol={setHoveredCol}
        />
      </div>

      {/* Legend */}
      {visibleLegend.length > 0 && (
        <div className="flex flex-wrap gap-x-4 gap-y-1.5 px-1">
          {visibleLegend.map(({ key, label }) => (
            <LegendItem key={key} name={key} label={label} />
          ))}
        </div>
      )}

    </div>
  );
}