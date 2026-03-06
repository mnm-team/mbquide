export const actionIcon = '💥'
export const flowIcon = '🌊'
export const focusIcon = '👁️'
export const undoIcon = '↩️'
export const redoIcon = '↪️'
export const resetIcon = '🔁'

export const ZXIcon = ({ size = 25 }) => (
  <svg width={ size } height={ size } viewBox="0 0 18 18" fill="none">
    <circle cx="5" cy="9" r="2.5" stroke="black" strokeWidth="0.5" fill="green" />
    <circle cx="13" cy="9" r="2.5" stroke="black" strokeWidth="0.5"  fill="red" />
    <path d="M7.5 9h3" stroke="black" strokeWidth=".6" />
    <path d="M2 9H2.5" stroke="black" strokeWidth=".6" strokeLinecap="round" />
    <path d="M15.5 9H16" stroke="black" strokeWidth=".6" strokeLinecap="round" />
  </svg>
);

export const MBQCIcon = ({ size = 25 }) => (
  <svg width={ size } height={ size } viewBox="0 0 18 18" fill="none">
    <circle cx="9" cy="5" r="2" stroke="black" strokeWidth="0.5" fill="blue" />
    <circle cx="4" cy="13" r="2" stroke="black" strokeWidth="0.5" fill="red" />
    <circle cx="14" cy="13" r="2" stroke="black" strokeWidth="0.5" fill="yellow" />
    <path d="M7.5 6.5L5.5 11.5" stroke="black" strokeWidth="0.8" strokeLinecap="round" />
    <path d="M10.5 6.5L12.5 11.5" stroke="black" strokeWidth="0.8" strokeLinecap="round" />
    <path d="M6 13h6" stroke="black" strokeWidth="0.8" strokeLinecap="round" />
  </svg>
);

export const CodeIcon = ({ size = 25 }) => (
  <svg width={size} height={size} viewBox="0 0 18 18" fill="none">
    <path
      d="M6 4.5L3.5 9L6 13.5"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
      strokeLinejoin="round"
    />
    <path
      d="M12 4.5L14.5 9L12 13.5"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
      strokeLinejoin="round"
    />
    <path
      d="M10.5 4.5L7.5 13.5"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
    />
  </svg>
);

export const RunAllIcon = ({ size = 25 }) => (
  <svg width={size} height={size} viewBox="0 0 18 18" fill="none">
    {/* First triangle */}
    <path
      d="M4.5 4.5L9 9L4.5 13.5Z"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
      strokeLinejoin="round"
    />
    {/* Second triangle */}
    <path
      d="M9 4.5L13.5 9L9 13.5Z"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
      strokeLinejoin="round"
    />
  </svg>
);

export const MeasurementIcon = ({ size = 25 }) => (
  <svg width={ size } height={ size } viewBox="0 0 18 18" fill="none">
    <path
      d="M5 11a5 5 0 0 1 10 0"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
    />
    <path
      d="M9 11l3-3"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
    />
  </svg>
);
