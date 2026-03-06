import { useState, useCallback } from 'react';
import { HistoryState } from '../types';

const MAX_HISTORY_LENGTH = 10;

export const useGraphHistory = () => {
  const [history, setHistory] = useState<HistoryState[]>([]);
  const [redoStack, setRedoStack] = useState<HistoryState[]>([]);

  const saveToHistory = useCallback((state: HistoryState) => {
    setHistory(prev => {
      const newHistory = [...prev, { ...state }];
      if (newHistory.length > MAX_HISTORY_LENGTH) {
        newHistory.shift();
      }
      return newHistory;
    });
    setRedoStack([]);
  }, []);

  const undo = useCallback((currentState: HistoryState): HistoryState | null => {
    if (history.length === 0) return null;

    const previousState = history[history.length - 1];
    
    setRedoStack(prevRedo => [...prevRedo, { ...currentState }]);
    setHistory(prev => prev.slice(0, -1));

    return previousState;
  }, [history]);

  const redo = useCallback((currentState: HistoryState): HistoryState | null => {
    if (redoStack.length === 0) return null;

    const nextState = redoStack[redoStack.length - 1];
    
    setHistory(prevHistory => [...prevHistory, { ...currentState }]);
    setRedoStack(prevRedo => prevRedo.slice(0, -1));

    return nextState;
  }, [redoStack]);

  const canUndo = history.length > 0;
  const canRedo = redoStack.length > 0;

  return {
    saveToHistory,
    undo,
    redo,
    canUndo,
    canRedo,
  };
};