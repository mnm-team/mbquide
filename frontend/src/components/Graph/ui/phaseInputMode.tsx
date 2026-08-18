import React, { useState, useEffect, useRef } from 'react';
import { NodeType } from '../types';
import { NumberButton } from "../../Buttons";

interface TargetOption {
  value: string;
  label: string;
}

interface PhaseInputModalProps {
  node?: NodeType | null;
  isOpen: boolean;
  onClose: () => void;
  onSubmit: (angle: number) => void;
  title?: string;
  // Optional target switcher, e.g. for picking which of a pair of nodes the angle applies to.
  targetOptions?: TargetOption[];
  selectedTargetValue?: string;
  onTargetChange?: (value: string) => void;
}

const parsePiExpression = (input: string): number | null => {
  const s = input.trim().toLowerCase().replace(/\s+/g, '').replace(/π/g, 'pi');
  if (s === '') return null;

  // Accepts: "1.5", "1.5pi", "1.5/2", "1.5pi/2", "pi/1.5", "pi", "0", etc.
  const match = s.match(/^(\d+\.?\d*)(pi)?(?:\/(\d+\.?\d*))?$|^(pi)(?:\/(\d+\.?\d*))?$/);
  if (!match) return null;

  let num: number;
  let den: number;

  if (match[4] === 'pi') {
    // Matched bare "pi" or "pi/N" (second branch of alternation)
    num = 1;
    den = match[5] ? parseFloat(match[5]) : 1;
  } else {
    // Matched "N", "Npi", "N/D", "Npi/D" (first branch)
    num = parseFloat(match[1]);
    const hasPi = !!match[2];
    den = match[3] ? parseFloat(match[3]) : 1;
    if (!hasPi && !match[3]) {
      // Plain decimal like "1.5" means 1.5π
    }
  }

  if (den === 0) return null;

  return (num * Math.PI) / den;
};


export const PhaseInputModal: React.FC<PhaseInputModalProps> = ({
  node,
  isOpen,
  onClose,
  onSubmit,
  title,
  targetOptions,
  selectedTargetValue,
  onTargetChange,
}) => {
  const [value, setValue] = useState<string>('');
  const [parseError, setParseError] = useState<boolean>(false);
  const inputRef = useRef<HTMLInputElement>(null);
  const backdropRef = useRef<HTMLDivElement>(null);
  
  const isXYZ = node?.basis === 'X' || node?.basis === 'Y' || node?.basis === 'Z';
  
  useEffect(() => {
    if (isOpen) {
      setValue('');
      setParseError(false);
      setTimeout(() => {
        if (isXYZ) {
          backdropRef.current?.focus();
        } else {
          inputRef.current?.focus();
        }
      }, 0);
    }
  }, [isOpen]);


  const handleValueChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setValue(e.target.value);
    setParseError(false);
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();

    let numValue: number;

    if (isXYZ) {
      numValue = value === 'pi' ? Math.PI : 0;
    } else {
      const parsed = parsePiExpression(value);
      if (parsed === null) {
        setParseError(true);
        return;
      }
      numValue = ((parsed % (2 * Math.PI)) + 2 * Math.PI) % (2 * Math.PI);
    }

    onSubmit(numValue);
    onClose();
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Escape') onClose();
  };

  if (!isOpen || node?.basis === 'OUTPUT') return null;

  return (
    <div
      ref={backdropRef}
      className="fixed inset-0 bg-opacity-50 backdrop-blur-sm flex items-center justify-center z-50"
      onClick={onClose}
      onKeyDown={handleKeyDown}
      tabIndex={-1}
    >
      <div
        className="bg-white rounded-lg shadow-xl p-6 w-96"
        onClick={(e) => e.stopPropagation()}
      >
        <h2 className="text-xl font-semibold mb-4">
          {title ?? `Enter a Phase for Node ${node?.id}`}
        </h2>

        {targetOptions && targetOptions.length > 0 && (
          <div className="mb-4 flex rounded-md border border-gray-200 overflow-hidden">
            {targetOptions.map((opt) => (
              <button
                key={opt.value}
                type="button"
                onClick={() => onTargetChange?.(opt.value)}
                className={`flex-1 px-3 py-1.5 text-sm font-medium transition-colors ${
                  selectedTargetValue === opt.value
                    ? 'bg-blue-500 text-white'
                    : 'bg-white text-gray-600 hover:bg-gray-50'
                }`}
              >
                {opt.label}
              </button>
            ))}
          </div>
        )}

        <form onSubmit={handleSubmit}>
          
          {isXYZ && (
            <div className="mb-4">
              <label className="block text-sm font-medium text-gray-700 mb-3">
                Choose a phase
              </label>
              <div className="inline-flex gap-8">
                <NumberButton
                  onClick={() => { onSubmit(0); onClose(); }}
                  label="0"
                  disabled={false}
                />
                <NumberButton
                  onClick={() => { onSubmit(Math.PI); onClose(); }}
                  label="π"
                  disabled={false}
                />
              </div>
            </div>
          )}

          {!isXYZ && (
            <div className="mb-2">
              <label className="block text-sm font-medium text-gray-700 mb-2">
                Angle (multiples of π)
              </label>
              <input
                ref={inputRef}
                type="text"
                value={value}
                onChange={handleValueChange}
                placeholder="e.g. 1, pi/2, 3/2"
                className={`w-full px-3 py-2 border rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 ${
                  parseError ? 'border-red-500' : 'border-gray-300'
                }`}
              />
              <p className={`text-xs mt-1 ${parseError ? 'text-red-500' : 'text-gray-500'}`}>
                {parseError
                  ? 'Invalid format. Try: pi/2, 3pi/4, 1/2, 3/2'
                  : 'All values are multiples of π'}
              </p>
            </div>
          )}

          {!isXYZ && (
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
          )}
          
        </form>
      </div>
    </div>
  );
};