import { createContext, useCallback, useContext, useMemo, useState, ReactNode } from "react";

type TutorialOverlayContextValue = {
  isOpen: boolean;
  /** The step currently shown inside the overlay, or null for the overview grid. */
  activeStepId: string | null;
  /** Opens the overlay - to the overview grid, or straight to a step if given. */
  open: (stepId?: string) => void;
  close: () => void;
  showStep: (stepId: string) => void;
  showOverview: () => void;
};

const TutorialOverlayContext = createContext<TutorialOverlayContextValue | null>(null);

/**
 * Holds whether the tutorial overlay is open and which step it's showing, above the router
 * (see main.tsx) so it survives - and is reachable from - every route. This is what lets
 * "press ? while in the MBQC editor" show the tutorial without navigating away and losing
 * the editor's in-memory graph/pan/zoom state: the overlay mounts on top of whatever page is
 * already there instead of replacing it.
 */
export function TutorialOverlayProvider({ children }: { children: ReactNode }) {
  const [isOpen, setIsOpen] = useState(false);
  const [activeStepId, setActiveStepId] = useState<string | null>(null);

  const open = useCallback((stepId?: string) => {
    setActiveStepId(stepId ?? null);
    setIsOpen(true);
  }, []);

  const close = useCallback(() => {
    setIsOpen(false);
  }, []);

  const showStep = useCallback((stepId: string) => {
    setActiveStepId(stepId);
  }, []);

  const showOverview = useCallback(() => {
    setActiveStepId(null);
  }, []);

  const value = useMemo(
    () => ({ isOpen, activeStepId, open, close, showStep, showOverview }),
    [isOpen, activeStepId, open, close, showStep, showOverview]
  );

  return <TutorialOverlayContext.Provider value={value}>{children}</TutorialOverlayContext.Provider>;
}

export function useTutorialOverlay() {
  const ctx = useContext(TutorialOverlayContext);
  if (!ctx) {
    throw new Error("useTutorialOverlay must be used within a TutorialOverlayProvider");
  }
  return ctx;
}
