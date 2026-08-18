import React from "react";


type NumberButtonProps = {
  onClick?: () => void;
  label: string;
  disabled?: boolean;
};

export function NumberButton({ onClick, label, disabled }: NumberButtonProps) {
  return (
    <button
      onClick={onClick}
      disabled={disabled}
      className="
        group relative
        w-14 h-14
        disabled:opacity-50 disabled:cursor-not-allowed
      "
      style={{ cursor: disabled ? "not-allowed" : "pointer" }}
    >
      <div
        className="
          flex items-center justify-center
          w-full h-full
          border border-gray-200 bg-white
          rounded-lg
          text-gray-800 text-lg font-medium

          transition-all duration-200 ease-out

          group-hover:border-blue-400
          group-hover:bg-blue-50
          group-hover:text-blue-700
          group-hover:shadow-sm

          active:scale-[0.97]
          active:bg-blue-100

          disabled:bg-gray-50 disabled:border-gray-200 disabled:text-gray-400
        "
      >
        {label}
      </div>
    </button>
  );
}


type ActionButtonProps = {
  onClick?: () => void;
  disabled?: boolean;
  label: string;
  sublabel?: string;
  // Renders sublabel as a hover-only tooltip above the button instead of a always-visible
  // line below the label - for compact toolbars (e.g. ControlPanel) where the sublabel is a
  // hint rather than part of the button's permanent content.
  sublabelAsTooltip?: boolean;
  icon?: React.ReactNode;
  arrow?: boolean;
};

export function ActionButton({ onClick, disabled, label, sublabel, sublabelAsTooltip, icon, arrow }: ActionButtonProps) {
  return (
    <button
      onClick={onClick}
      disabled={disabled}
      className="
        group relative w-full min-w-full text-left 
        disabled:opacity-50 disabled:cursor-not-allowed
      "
      style={{ display: "block", cursor: disabled ? "not-allowed" : "pointer" }}
    >
      <div
        className="
          flex items-center gap-4 px-4 py-3
          border border-gray-200 bg-white
          transition-all duration-200 ease-out
          rounded-lg
          group-hover:border-blue-400 group-hover:bg-blue-50 group-hover:shadow-sm
          disabled:bg-gray-50 disabled:border-gray-200
        "
      >
        {icon && (
          <div
            className="
              flex-shrink-0 w-10 h-10 flex items-center justify-center
              border border-gray-200
              text-blue-500 bg-blue-50
              rounded-lg
              group-hover:bg-blue-100 group-hover:text-blue-700
              disabled:text-gray-400 disabled:bg-gray-100
              transition-all duration-200
            "
          >
            {icon}
          </div>
        )}

        {/* Text */}
        <div className="flex-1 min-w-0">
          <div className="text-gray-800 group-hover:text-gray-900 text-sm font-medium transition-colors duration-200">
            {label}
          </div>
          {sublabel && !sublabelAsTooltip && (
            <div className="text-gray-400 text-xs mt-0.5 transition-colors duration-200 group-hover:text-gray-500">
              {sublabel}
            </div>
          )}
          {sublabel && sublabelAsTooltip && (
            <div
              role="tooltip"
              className="
                pointer-events-none absolute bottom-full left-1/2 z-50 mb-2 -translate-x-1/2
                whitespace-nowrap rounded-md bg-gray-900 px-2 py-1 text-xs text-white
                opacity-0 scale-95 transition-all duration-150 ease-out
                group-hover:opacity-100 group-hover:scale-100
              "
            >
              {sublabel}
            </div>
          )}
        </div>

        {/* Arrow */}
        {arrow && (
          <div className="text-gray-300 group-hover:text-blue-500 transition-colors duration-200 ml-2">
            <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
              <path
                d="M3 7h8M7 3l4 4-4 4"
                stroke="currentColor"
                strokeWidth="1.5"
                strokeLinecap="round"
                strokeLinejoin="round"
              />
            </svg>
          </div>
        )}
      </div>
    </button>
  );
}


type ExampleButtonProps = {
  onClick: () => void;
  label: string;
  description: string;
  qubits: string;
  description_link?: string;
};

export function ExampleButton({ onClick, label, description, qubits, description_link }: ExampleButtonProps) {
  return (
    <button
      onClick={onClick}
      className="group w-full text-left"
      style={{ all: "unset", display: "block", cursor: "pointer" }}
    >
      <div
        className="
          px-3 py-2.5
          border border-gray-200 bg-white
          hover:border-blue-300 hover:bg-blue-50
          transition-all duration-150
        "
        style={{ borderRadius: 8 }}
      >
        <div className="flex items-center justify-between gap-2">
          <span className="text-xs font-medium text-gray-700 group-hover:text-gray-900">
            {label}
          </span>

          <div className="flex gap-1.5">
            <span className="text-[10px] px-2 py-0.5 bg-gray-100 border border-gray-200 text-gray-600 rounded">
              {qubits}
            </span>
          </div>
        </div>

        <p className="text-[11px] text-gray-500 mt-0.5">
          {description}

          {description_link && (
            <>
              {" "}
              <a
                href={description_link}
                target="_blank"
                rel="noopener noreferrer"
                className="text-[10px] text-blue-600 hover:text-blue-700 underline"
                onClick={(e) => e.stopPropagation()}
              >
                → Learn more
              </a>
            </>
          )}
        </p>
      </div>
    </button>
  );
}

interface CheckboxProps {
  label: string;
  checked: boolean;
  onChange: (checked: boolean) => void;
  disabled?: boolean;
}

export const Checkbox: React.FC<CheckboxProps> = ({
  label,
  checked,
  onChange,
  disabled = false,
}) => {
  return (
    <label
      className={`group inline-flex items-center gap-3 select-none transition-all duration-200 ${
        disabled ? "opacity-50 cursor-not-allowed" : "cursor-pointer"
      }`}
    >
      <div className="relative flex-shrink-0">
        <input
          type="checkbox"
          checked={checked}
          onChange={(e) => onChange(e.target.checked)}
          disabled={disabled}
          className="sr-only peer"
        />

        {/* Checkbox box */}
        <div
          className={`
            w-5 h-5 rounded-md border flex items-center justify-center
            transition-all duration-200 ease-out
            peer-focus-visible:ring-2 peer-focus-visible:ring-offset-2 peer-focus-visible:ring-indigo-500
            ${
              checked
                ? `
                  bg-indigo-50
                  border-indigo-500
                  shadow-inner
                `
                : `
                  bg-white
                  border-gray-300
                  group-hover:border-indigo-400
                `
            }
          `}
        >
          {/* Checkmark */}
          <svg
            className={`
              w-3.5 h-3.5
              transition-all duration-150 ease-out
              ${
                checked
                  ? "opacity-100 scale-100 text-indigo-600"
                  : "opacity-0 scale-75 text-transparent"
              }
            `}
            viewBox="0 0 12 10"
            fill="none"
            stroke="currentColor"
            strokeWidth="2.2"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <polyline points="1.5 5.5 4.5 8.5 10.5 1.5" />
          </svg>
        </div>
      </div>

      {/* Label */}
      <span
        className={`
          text-sm font-medium transition-colors duration-200
          ${
            checked
              ? "text-indigo-700"
              : "text-gray-600 group-hover:text-gray-900"
          }
        `}
      >
        {label}
      </span>
    </label>
  );
};