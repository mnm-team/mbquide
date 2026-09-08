import React from "react";

// Shared visual language for hover tooltips: a dark rounded pill with white text that fades in
// above its target. `Tooltip` renders it for React/DOM callers (e.g. ActionButton's
// `sublabelAsTooltip`); the raw constants below let SVG/d3-rendered callers (e.g. the
// unfusion-handle tooltip in Graph/rendering/renderUnfusionHandles.ts) draw the same pill by
// hand, since a React component can't be mounted directly into a d3-managed <svg> tree.
export const TOOLTIP_BG = "#111827"; // tailwind gray-900
export const TOOLTIP_TEXT_COLOR = "#ffffff";
export const TOOLTIP_RADIUS = 6; // px, matches rounded-md
export const TOOLTIP_FONT_SIZE = 13; // px, default for React tooltips (e.g. ActionButton)
export const TOOLTIP_PADDING_X = 8; // px, matches px-2
export const TOOLTIP_PADDING_Y = 4; // px, matches py-1
export const TOOLTIP_GAP = 8; // px, matches mb-2

// The unfusion-handle tooltip lives on the graph canvas, close to other small labels, so it
// uses a smaller font than the default (SVG-only; see Graph/rendering/renderUnfusionHandles.ts).
export const UNFUSION_TOOLTIP_FONT_SIZE = 14; // px

// Pill height for a given font size (padding above/below plus a little breathing room), so SVG
// callers can size their background rect to match whatever font size they render at.
export const getTooltipHeight = (fontSize: number) => fontSize + TOOLTIP_PADDING_Y * 2 + 4;

type TooltipProps = {
  children: React.ReactNode;
  fontSize?: number;
  transitionMs?: number;
};

// Hover-only pill positioned above its parent. The parent must be a `group` with non-static
// positioning (e.g. `relative`) for the `group-hover` + `absolute` placement to work.
//
// Color/size/spacing come from inline styles sourced from the constants above rather than
// Tailwind utility classes (bg-gray-900, text-xs, etc.), so those constants are the actual
// single source of truth shared with the SVG tooltip in renderUnfusionHandles.ts - not just a
// description of a hardcoded class.
export function Tooltip({ children, fontSize = TOOLTIP_FONT_SIZE, transitionMs = 150 }: TooltipProps) {
  return (
    <div
      role="tooltip"
      className="
        pointer-events-none absolute bottom-full left-1/2 z-50 -translate-x-1/2
        whitespace-nowrap opacity-0 scale-95 transition-all ease-out
        group-hover:opacity-100 group-hover:scale-100
      "
      style={{
        marginBottom: TOOLTIP_GAP,
        borderRadius: TOOLTIP_RADIUS,
        backgroundColor: TOOLTIP_BG,
        color: TOOLTIP_TEXT_COLOR,
        fontSize,
        padding: `${TOOLTIP_PADDING_Y}px ${TOOLTIP_PADDING_X}px`,
        transitionDuration: `${transitionMs}ms`,
      }}
    >
      {children}
    </div>
  );
}
