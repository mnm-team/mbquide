import React, { useState, useEffect, useRef } from 'react';
import { NodeType } from '../types';

interface PhaseInputModalProps {
  node?: NodeType | null;
  isOpen: boolean;
  onClose: () => void;
  onSubmit: (angle: number) => void;
}

export const PhaseInputModal: React.FC<PhaseInputModalProps> = ({
  node,
  isOpen,
  onClose,
  onSubmit,
}) => {
  const [value, setValue] = useState<string>('');
  const [unit, setUnit] = useState<'radians' | 'degrees'>('radians');
  const [presetPhase, setPresetPhase] = useState<string>('');
  const inputRef = useRef<HTMLInputElement>(null);

  const presetPhases =
    node?.basis === 'X' || node?.basis === 'Y' || node?.basis === 'Z'
      ? [
          { label: '0', value: 0 },
          { label: 'π', value: Math.PI },
        ]
      : [
          { label: '0', value: 0 },
          { label: 'π/2', value: Math.PI / 2 },
          { label: 'π', value: Math.PI },
          { label: '3π/2', value: (3 * Math.PI) / 2 },
        ];

  useEffect(() => {
    if (isOpen) {
      const defaultAngle = 0;
      setValue(defaultAngle.toFixed(4));
      setUnit('radians');
      setPresetPhase('');
      setTimeout(() => inputRef.current?.focus(), 0);
    }
  }, [isOpen]);

  const handlePresetChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    const selectedValue = e.target.value;
    setPresetPhase(selectedValue);
    
    if (selectedValue) {
      const phaseValue = parseFloat(selectedValue);
      if (unit === 'radians') {
        setValue(phaseValue.toFixed(4));
      } else {
        setValue((phaseValue * 180 / Math.PI).toFixed(2));
      }
    }
  };

  const handleUnitChange = (newUnit: 'radians' | 'degrees') => {
    setUnit(newUnit);
    // Update value if a preset is selected
    if (presetPhase) {
      const phaseValue = parseFloat(presetPhase);
      if (newUnit === 'radians') {
        setValue(phaseValue.toFixed(4));
      } else {
        setValue((phaseValue * 180 / Math.PI).toFixed(2));
      }
    }
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    let numValue = parseFloat(value);
    
    if (isNaN(numValue)) return;

    // Convert degrees to radians if needed
    if (unit === 'degrees') {
      numValue = (numValue * Math.PI) / 180;
    }

    // Normalize to [0, 2π]
    numValue = ((numValue % (2 * Math.PI)) + (2 * Math.PI)) % (2 * Math.PI);

    onSubmit(numValue);
    onClose();
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Escape') {
      onClose();
    }
  };

  const getMax = () => unit === 'radians' ? 2 * Math.PI : 360;
  const getStep = () => unit === 'radians' ? 0.00000001 : 0.00000001;

  if (!isOpen || node?.basis === 'OUTPUT') return null;

  return (
    <div
      className="fixed inset-0 bg-opacity-50 backdrop-blur-sm flex items-center justify-center z-50"
      onClick={onClose}
    >
      <style>{`
        input[type="number"]::-webkit-inner-spin-button,
        input[type="number"]::-webkit-outer-spin-button {
          -webkit-appearance: none;
          margin: 0;
        }
        input[type="number"] {
          -moz-appearance: textfield;
        }
      `}</style>
      <div
        className="bg-white rounded-lg shadow-xl p-6 w-96"
        onClick={(e) => e.stopPropagation()}
      >
        <h2 className="text-xl font-semibold mb-4">Enter a Phase for Node {node?.id}</h2>
        <form onSubmit={handleSubmit}>
          <div className="mb-4">
            <label className="block text-sm font-medium text-gray-700 mb-2">
              Quick Select
            </label>
            <select
              value={presetPhase}
              onChange={handlePresetChange}
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 bg-white"
            >
              <option value="">Select a phase...</option>
              {presetPhases.map((preset) => (
                <option key={preset.label} value={preset.value.toString()}>
                  {preset.label}
                </option>
              ))}
            </select>
          </div>

          {node?.basis !== 'X' && node?.basis !== 'Y' && node?.basis !== 'Z' && (
            <>
          <div className="mb-4">
            <label className="block text-sm font-medium text-gray-700 mb-2">
              Unit
            </label>
            <div className="flex gap-2">
              <button
                type="button"
                onClick={() => handleUnitChange('radians')}
                className={`flex-1 px-4 py-2 rounded-md transition-colors ${
                  unit === 'radians'
                    ? 'bg-blue-500 text-white'
                    : 'bg-gray-200 text-gray-800 hover:bg-gray-300'
                }`}
              >
                Radians
              </button>
              <button
                type="button"
                onClick={() => handleUnitChange('degrees')}
                className={`flex-1 px-4 py-2 rounded-md transition-colors ${
                  unit === 'degrees'
                    ? 'bg-blue-500 text-white'
                    : 'bg-gray-200 text-gray-800 hover:bg-gray-300'
                }`}
              >
                Degrees
              </button>
            </div>
          </div>

          <div className="mb-2">
            <label className="block text-sm font-medium text-gray-700 mb-2">
              Angle {unit === 'radians' ? '(0 to 2π)' : '(0° to 360°)'}
            </label>
            <input
              ref={inputRef}
              type="number"
              value={value}
              onChange={(e) => {
                setValue(e.target.value);
                setPresetPhase('');
              }}
              onKeyDown={handleKeyDown}
              placeholder={unit === 'radians' ? '0' : '0'}
              min={0}
              max={getMax()}
              step={getStep()}
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
            />
            <p className="text-xs text-gray-500 mt-1">
              {unit === 'radians' 
                ? `2π ≈ ${(2 * Math.PI).toFixed(4)}`
                : 'Full circle = 360°'}
            </p>
          </div>

            </>
          )}

          <div className="flex gap-2 mt-4">
            <button
              type="button"
              onClick={onClose}
              className="flex-1 px-4 py-2 bg-gray-200 text-gray-800 rounded-md hover:bg-gray-300 transition-colors"
            >
              Cancel
            </button>
            <button
              type="submit"
              className="flex-1 px-4 py-2 bg-blue-500 text-white rounded-md hover:bg-blue-600 transition-colors"
            >
              Submit
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};