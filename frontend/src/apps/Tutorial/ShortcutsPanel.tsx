import { useEffect, useRef, useState } from "react";

type Shortcut = {
  keys: string[];
  description: string;
};

type ShortcutGroup = {
  heading: string;
  shortcuts: Shortcut[];
};

// Compiled from the actual keydown handlers in the app (apps/MBQC/index.tsx,
// components/Graph/hooks/useGraphSimulation.ts, components/Graph/index.tsx,
// apps/Simulator_App.tsx, components/TutorialHelpButton.tsx) plus the two mouse-driven
// interactions worth surfacing alongside them (Ctrl+drag pan, middle-click delete).
const SHORTCUT_GROUPS: ShortcutGroup[] = [
  {
    heading: "Global",
    shortcuts: [{ keys: ["?"], description: "Open Help & Tutorials" }],
  },
  {
    heading: "MBQC Editor",
    shortcuts: [
      { keys: ["B"], description: "Toggle Building Mode" },
      { keys: ["C"], description: "Recenter the view" },
      { keys: ["Ctrl", "Z"], description: "Undo" },
      { keys: ["Ctrl", "Y"], description: "Redo" },
      { keys: ["Ctrl", "drag"], description: "Pan the canvas" },
      { keys: ["Middle-click"], description: "Delete a node (Building Mode)" },
      { keys: ["Delete"], description: "Delete selection (Building Mode)" },
      { keys: ["Enter"], description: "Edit phase of selected node (Building Mode)" },
      { keys: ["Right-click", "drag"], description: "Create/remove an edge (Building Mode)" },
    ],
  },
  {
    heading: "Simulator",
    shortcuts: [{ keys: ["C"], description: "Recenter the view" }],
  },
];

function KeyBadge({ label }: { label: string }) {
  return (
    <kbd className="inline-flex items-center justify-center min-w-[1.6rem] px-1.5 py-0.5 rounded-md border border-b-2 border-gray-300 bg-white text-[11px] font-mono font-semibold text-gray-700 shadow-sm">
      {label}
    </kbd>
  );
}

/**
 * Fixed-to-the-viewport keyboard-shortcuts reference, mirroring TutorialQuickNav's fixed
 * left panel but on the right - a small icon that expands into a glass-style card on click,
 * rather than its own tutorial card/detail page (shortcuts don't have a video to demo, so
 * they don't fit that format well). Shared between index.tsx and Detail.tsx.
 */
export default function ShortcutsPanel() {
  const [open, setOpen] = useState(false);
  const containerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;

    const handleClickOutside = (e: MouseEvent) => {
      if (containerRef.current && !containerRef.current.contains(e.target as Node)) {
        setOpen(false);
      }
    };
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === "Escape") setOpen(false);
    };

    window.addEventListener("mousedown", handleClickOutside);
    window.addEventListener("keydown", handleEscape);
    return () => {
      window.removeEventListener("mousedown", handleClickOutside);
      window.removeEventListener("keydown", handleEscape);
    };
  }, [open]);

  return (
    <div ref={containerRef} className="fixed bottom-6 right-6 z-40">
      <button
        type="button"
        onClick={() => setOpen((prev) => !prev)}
        title="Keyboard shortcuts"
        className={`flex h-11 w-11 items-center justify-center rounded-full border shadow-sm transition-all duration-200 ${
          open
            ? "border-blue-400 bg-blue-50 text-blue-600"
            : "border-gray-200 bg-white text-gray-500 hover:border-blue-300 hover:text-blue-600 hover:shadow-md"
        }`}
      >
        <svg width="20" height="20" viewBox="0 0 18 18" fill="none">
          <rect x="1.5" y="4" width="15" height="10" rx="2" stroke="currentColor" strokeWidth="1.3" />
          <path
            d="M4 7h.01M6.5 7h.01M9 7h.01M11.5 7h.01M14 7h.01M4 10h.01M6.5 10h6.5"
            stroke="currentColor"
            strokeWidth="1.6"
            strokeLinecap="round"
          />
        </svg>
      </button>

      <div
        className={`absolute right-0 bottom-full mb-3 w-80 origin-bottom-right rounded-2xl border border-black/10 bg-white/90 backdrop-blur-xl shadow-xl transition-all duration-150 ${
          open
            ? "opacity-100 scale-100 pointer-events-auto"
            : "opacity-0 scale-95 pointer-events-none"
        }`}
      >
        <div className="px-4 py-3 border-b border-gray-200/70">
          <p className="text-sm font-semibold text-gray-800">Keyboard shortcuts</p>
        </div>
        <div className="max-h-[70vh] overflow-y-auto px-4 py-3 space-y-4">
          {SHORTCUT_GROUPS.map((group) => (
            <div key={group.heading}>
              <p className="text-[11px] font-semibold tracking-widest uppercase text-gray-400 mb-2">
                {group.heading}
              </p>
              <div className="space-y-2">
                {group.shortcuts.map((shortcut, i) => (
                  <div key={i} className="flex items-center justify-between gap-3">
                    <span className="text-xs text-gray-600">{shortcut.description}</span>
                    <span className="flex items-center gap-1 shrink-0">
                      {shortcut.keys.map((key, j) => (
                        <span key={j} className="flex items-center gap-1">
                          {j > 0 && <span className="text-gray-300 text-[10px]">+</span>}
                          <KeyBadge label={key} />
                        </span>
                      ))}
                    </span>
                  </div>
                ))}
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
