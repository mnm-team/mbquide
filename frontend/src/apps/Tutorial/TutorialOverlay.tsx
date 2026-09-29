import { useEffect } from 'react';
import { tutorialSteps } from './data';
import { useTutorialOverlay } from './TutorialOverlayContext';
import TutorialQuickNav from './QuickNav';
import ShortcutsPanel from './ShortcutsPanel';
import TutorialOverviewContent from './OverviewContent';
import TutorialDetailContent from './DetailContent';

/**
 * The tutorial/help content as a full-screen overlay on top of whatever page is currently
 * open, instead of a route - so pressing "?" while in the MBQC editor (say) shows help
 * without navigating away and losing the editor's in-memory graph/pan/zoom state. Closing
 * just unmounts this; the page underneath was never touched.
 *
 * Reuses the exact same QuickNav/ShortcutsPanel/OverviewContent/DetailContent as the real
 * /TUTORIAL and /TUTORIAL/:id routes (index.tsx, Detail.tsx) - same look, same "Read more"
 * links, same shortcuts reference - just driven by TutorialOverlayContext's state instead of
 * the URL. QuickNav's QASM/MBQC jump buttons are hidden here though (showNavButtons=false):
 * in overlay mode you're already sitting on top of a real app page, so navigating away is
 * instead an explicit "open as full page" button next to close.
 */
export default function TutorialOverlay() {
  const { isOpen, activeStepId, close, showStep, showOverview } = useTutorialOverlay();

  useEffect(() => {
    if (!isOpen) return;

    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') close();
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, close]);

  if (!isOpen) return null;

  const step = activeStepId ? tutorialSteps.find((s) => s.id === activeStepId) : null;

  return (
    <>
      {/* Full-bleed dim backdrop behind the inset card, so the sliver of underlying page
          peeking out around the edges reads as "dimmed page behind an overlay" rather than
          raw unrelated content. */}
      <div className="fixed inset-0 z-[2999] bg-black/20" />
      <div className="fixed inset-[15px] z-[3000] overflow-y-auto rounded-xl bg-slate-50 shadow-2xl">
        <TutorialQuickNav showNavButtons={false} />
        <ShortcutsPanel />

        <button
          type="button"
          onClick={() => {
            const path = activeStepId ? `/TUTORIAL/${activeStepId}` : '/TUTORIAL';
            window.open(path, '_blank', 'noopener,noreferrer');
          }}
          title="Open as full page (new tab)"
          className="fixed top-6 right-20 z-40 flex h-11 w-11 items-center justify-center rounded-full border border-gray-200 bg-white text-gray-500 shadow-sm transition-all duration-200 hover:border-blue-300 hover:text-blue-600 hover:shadow-md"
        >
          <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
            <path d="M6 3H3a1 1 0 00-1 1v9a1 1 0 001 1h9a1 1 0 001-1v-3" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" strokeLinejoin="round" />
            <path d="M9 2h5v5M14 2L7 9" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" strokeLinejoin="round" />
          </svg>
        </button>

        <button
          type="button"
          onClick={close}
          title="Close (Esc)"
          className="fixed top-6 right-6 z-40 flex h-11 w-11 items-center justify-center rounded-full border border-gray-200 bg-white text-gray-500 shadow-sm transition-all duration-200 hover:border-red-300 hover:text-red-600 hover:shadow-md"
        >
          <svg width="18" height="18" viewBox="0 0 18 18" fill="none">
            <path d="M4 4l10 10M14 4L4 14" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" />
          </svg>
        </button>

        <div className={step ? 'max-w-3xl mx-auto px-6 py-10' : 'max-w-5xl mx-auto px-6 py-10'}>
          {step ? (
            <>
              <button
                onClick={showOverview}
                className="text-sm text-gray-500 hover:text-gray-800 transition-colors mb-6"
              >
                ← Overview
              </button>
              <TutorialDetailContent step={step} />
            </>
          ) : (
            <>
              <div className="mb-8">
                <h1 className="text-2xl font-semibold text-gray-800">Help &amp; Tutorials</h1>
                <p className="text-sm text-gray-500 mt-0.5">
                  Short demonstrations of what you can do in MBQuIDE.
                </p>
              </div>
              <TutorialOverviewContent onSelectStep={showStep} />
            </>
          )}
        </div>
      </div>
    </>
  );
}
