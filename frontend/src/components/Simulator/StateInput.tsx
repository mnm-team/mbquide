import { useState, useCallback, useMemo } from "react";
import { ActionButton } from "../Buttons";

interface StateInputProps {
  numQubits: number;
  onSubmit?: (state: string) => void;
  disableSubmit?: boolean;
}

interface Complex {
  re: number;
  im: number;
}

function generateBitstrings(n: number): string[] {
  return Array.from({ length: 2 ** n }, (_, i) =>
    i.toString(2).padStart(n, "0")
  );
}

/**
 * Parse a complex amplitude string like:
 *   "1", "0.5", "1/sqrt(2)", "i", "2i", "1/sqrt(2)i",
 *   "1+2i", "1-1/sqrt(2)i", "-i", "1/sqrt(2)+1/sqrt(2)i"
 *
 * Returns null if the string is unparseable.
 */
function parseAmplitude(str: string): Complex | null {
  if (!str || str.trim() === "") return null;

  // Normalise whitespace, expand known constants
  let expr = str
    .trim()
    .replace(/\s+/g, "")
    .replace(/sqrt\(/g, "Math.sqrt(")
    .replace(/pi/g, "Math.PI");

  // Insert explicit * before bare `i` so "2i" → "2*i", "sqrt(2)i" → "sqrt(2)*i"
  expr = expr.replace(/(\d|\))i/g, "$1*i");

  function evalReal(e: string): number {
    if (e === "" || e === "+") return 1;
    if (e === "-") return -1;
    try {
      // eslint-disable-next-line no-new-func
      return Function('"use strict"; const Math = globalThis.Math; return (' + e + ")")() as number;
    } catch {
      return NaN;
    }
  }

  // Pure real (no imaginary unit present)
  if (!expr.includes("i")) {
    const val = evalReal(expr);
    return isNaN(val) ? null : { re: val, im: 0 };
  }

  // Find the last top-level + or - that separates real and imaginary parts
  let depth = 0;
  let splitIdx = -1;
  for (let i = expr.length - 1; i > 0; i--) {
    const c = expr[i];
    if (c === ")") depth++;
    else if (c === "(") depth--;
    else if ((c === "+" || c === "-") && depth === 0) {
      splitIdx = i;
      break;
    }
  }

  // Strip trailing "*i" or "i" from the imaginary part
  function stripI(s: string): string {
    if (s.endsWith("*i")) return s.slice(0, -2);
    if (s.endsWith("i")) return s.slice(0, -1);
    return s;
  }

  if (splitIdx === -1) {
    // Entire expression is imaginary (e.g. "i", "2*i", "1/sqrt(2)*i")
    const im = evalReal(stripI(expr) || "1");
    return isNaN(im) ? null : { re: 0, im };
  }

  const rePart = expr.slice(0, splitIdx);
  const imRaw = stripI(expr.slice(splitIdx));

  const re = evalReal(rePart);
  const im = evalReal(imRaw === "+" || imRaw === "" ? "1" : imRaw === "-" ? "-1" : imRaw);

  return isNaN(re) || isNaN(im) ? null : { re, im };
}

/** Format a complex number for the output state string, e.g. "0.707107i", "0.5+0.5i" */
function formatComplex(c: Complex): string {
  const fmt = (n: number) => parseFloat(n.toFixed(6)).toString();

  if (c.im === 0) return fmt(c.re);
  if (c.re === 0) return `${fmt(c.im)}i`;

  const sign = c.im < 0 ? "-" : "+";
  return `${fmt(c.re)}${sign}${fmt(Math.abs(c.im))}i`;
}

const NORM_TOLERANCE = 1e-6;

export default function StateInput({
  numQubits,
  onSubmit,
  disableSubmit,
}: StateInputProps) {
  const [amplitudeStrings, setAmplitudeStrings] = useState<Record<string, string>>({});
  const [focusedKey, setFocusedKey] = useState<string | null>(null);

  const bitstrings = generateBitstrings(numQubits);

  const handleAmplitudeChange = useCallback(
    (bitstring: string, value: string) => {
      setAmplitudeStrings((prev) => ({ ...prev, [bitstring]: value }));
    },
    []
  );

  /** Parse every filled amplitude and compute the squared norm. */
  const { parsedAmplitudes, normSq, parseError } = useMemo(() => {
    const parsed: Record<string, Complex> = {};
    let normSq = 0;
    let parseError = false;

    for (const bs of bitstrings) {
      const raw = amplitudeStrings[bs];
      if (!raw || raw.trim() === "") continue;

      const c = parseAmplitude(raw);
      if (c === null) {
        parseError = true;
        break;
      }
      parsed[bs] = c;
      normSq += c.re * c.re + c.im * c.im;
    }

    return { parsedAmplitudes: parsed, normSq, parseError };
  }, [amplitudeStrings, bitstrings]);

  const filledCount = Object.values(amplitudeStrings).filter(Boolean).length;
  const hasAnyAmplitude = filledCount > 0;
  const isNormalised = hasAnyAmplitude && !parseError && Math.abs(normSq - 1) <= NORM_TOLERANCE;

  const handleSubmit = () => {
    const state = bitstrings
      .filter((bs) => parsedAmplitudes[bs] !== undefined)
      .map((bs) => `(${formatComplex(parsedAmplitudes[bs])})|${bs}>`)
      .join(" + ");

    onSubmit?.(state);
  };

  const statusHint = (() => {
    if (!hasAnyAmplitude) return {
        text: `‖ψ‖² = 0`,
        color: "text-slate-500",
      };
    if (parseError) return { text: "parse error", color: "text-red-400" };
    if (!isNormalised)
      return {
        text: `‖ψ‖² = ${parseFloat(normSq.toFixed(6))}`,
        color: "text-amber-500",
      };
    return { text: "‖ψ‖² = 1 ✓", color: "text-emerald-500" };
  })();

  return (
    <div className="bg-white font-mono border-b border-slate-200 flex items-stretch z-10">

      {/* Left: compact title column */}
      <div className="flex flex-col justify-center px-4 py-2 border-r border-slate-200 shrink-0 gap-0.5 w-30">
        <span className="text-xs tracking-widest uppercase text-slate-600 font-medium whitespace-nowrap">
          |ψ⟩ State
        </span>
        {statusHint && (
          <span className={`text-xs whitespace-nowrap ${statusHint.color}`}>
            {statusHint.text}
          </span>
        )}
      </div>

      {/* Center: horizontally scrollable basis states */}
      <div className="overflow-x-auto flex-1">
        <div
          className="flex flex-row gap-px px-3 py-2"
          style={{ width: "max-content", height: "100%" }}
        >
          {bitstrings.map((bs) => {
            const raw = amplitudeStrings[bs];
            const parsed = raw ? parseAmplitude(raw) : undefined;
            const hasError = raw && raw.trim() !== "" && parsed === null;

            return (
              <div
                key={bs}
                className={`flex items-center border rounded-sm overflow-hidden transition-colors duration-150 shrink-0 h-full ${
                  hasError
                    ? "border-red-300 bg-red-50"
                    : focusedKey === bs
                    ? "border-blue-400 bg-blue-50"
                    : "border-slate-200 hover:border-slate-300 hover:bg-slate-50"
                }`}
                style={{ width: "160px" }}
              >
                {/* Ket label */}
                <div
                  className={`flex items-center justify-center px-2 self-stretch border-r text-xs tracking-wider shrink-0 ${
                    hasError
                      ? "border-red-200 bg-red-100/50"
                      : focusedKey === bs
                      ? "border-blue-200 bg-blue-100/50"
                      : "border-slate-200 bg-slate-50"
                  }`}
                >
                  <span className="text-slate-500">|</span>
                  <span className="text-slate-700">{bs}</span>
                  <span className="text-slate-500">⟩</span>
                </div>

                {/* α = input inline */}
                <div className="flex items-center px-2 flex-1 min-w-0">
                  <span className="text-slate-500 text-xs shrink-0">α = </span>
                  <input
                    type="text"
                    placeholder="0 + 0i"
                    value={amplitudeStrings[bs] ?? ""}
                    onChange={(e) => handleAmplitudeChange(bs, e.target.value)}
                    onFocus={() => setFocusedKey(bs)}
                    onBlur={() => setFocusedKey(null)}
                    spellCheck={false}
                    className="flex-1 min-w-0 bg-transparent border-none outline-none text-slate-800 text-xs placeholder-slate-300 caret-blue-500 pl-1"
                  />
                </div>
              </div>
            );
          })}
        </div>
      </div>

      {/* Right: submit button */}
      <div className="flex items-center px-3 py-2 border-l border-slate-200 shrink-0">
        <ActionButton
          onClick={handleSubmit}
          disabled={disableSubmit || !isNormalised}
          label="Submit"
          arrow={true}
        />
      </div>

    </div>
  );
}