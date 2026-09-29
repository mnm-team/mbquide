import { useEffect } from "react";
import { useTutorialOverlay } from "../apps/Tutorial/TutorialOverlayContext";

/**
 * Persistent "open help" affordance, rendered once at the app root (see main.tsx) so it
 * shows up on every route without each page having to remember to include it. Opens the
 * tutorial as a full-screen overlay (TutorialOverlay.tsx) on top of whatever page is
 * currently active, rather than navigating to /TUTORIAL - so pressing "?" while in the MBQC
 * editor doesn't unmount it and lose the in-memory graph/pan/zoom state.
 *
 * Marked data-tutorial-hide so it disappears from tutorial recordings themselves - see
 * disableTextSelection() in Tutorial/scripts/lib/chrome.js, which every recording script
 * calls (directly or via hideChrome()/hideChromeExceptPanel()) and which hides this
 * specific attribute value regardless of which of those three a given script uses.
 */
export default function TutorialHelpButton() {
  const { open } = useTutorialOverlay();

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      const tag = (e.target as HTMLElement).tagName;
      if (tag === "INPUT" || tag === "TEXTAREA") return;

      if (e.key === "?") {
        e.preventDefault();
        open();
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [open]);

  return (
    <button
      type="button"
      data-tutorial-hide="tutorial-help-button"
      onClick={() => open()}
      title="Help (press ?)"
      className="fixed bottom-4 left-4 z-[2000] flex h-10 w-10 items-center justify-center rounded-full border border-black/10 bg-white/90 text-gray-600 shadow-md backdrop-blur-sm transition-colors hover:bg-white hover:text-gray-900"
    >
      <span className="text-base font-semibold leading-none">?</span>
    </button>
  );
}
