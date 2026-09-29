import { tutorialSteps } from './data';
import { TutorialStep } from './types';

function groupByPage(steps: TutorialStep[]): [string, TutorialStep[]][] {
  const order: string[] = [];
  const groups = new Map<string, TutorialStep[]>();

  for (const step of steps) {
    if (!groups.has(step.page)) {
      groups.set(step.page, []);
      order.push(step.page);
    }
    groups.get(step.page)!.push(step);
  }

  return order.map((page) => [page, groups.get(page)!]);
}

function TutorialCard({ step, onSelect }: { step: TutorialStep; onSelect: (id: string) => void }) {
  return (
    <button
      type="button"
      onClick={() => onSelect(step.id)}
      className="group block w-full text-left border border-gray-200 rounded-xl bg-white overflow-hidden shadow-sm transition-all duration-150 hover:border-blue-300 hover:shadow-md"
    >
      <div className="bg-slate-900 flex items-center justify-center">
        {step.media.type === 'video' ? (
          <video
            src={step.media.src}
            autoPlay
            loop
            muted
            playsInline
            className="w-full max-h-[420px] object-contain"
          />
        ) : (
          <img
            src={step.media.src}
            alt={step.title}
            className="w-full max-h-[420px] object-contain"
          />
        )}
      </div>
      <div className="p-4">
        <h3 className="text-base font-semibold text-gray-800 mb-1.5 group-hover:text-blue-700 transition-colors">
          {step.title}
        </h3>
        <p className="text-sm text-gray-500 leading-relaxed">{step.description}</p>
        {step.sections && step.sections.length > 0 && (
          <p className="text-xs text-blue-600 mt-2 font-medium">Read more →</p>
        )}
      </div>
    </button>
  );
}

/**
 * The tutorial overview grid, grouped by page - shared between the routed /TUTORIAL page
 * (index.tsx, where onSelectStep navigates to /TUTORIAL/:id) and TutorialOverlay (where
 * onSelectStep just switches which step the overlay shows, without touching the URL or
 * whatever page is open underneath).
 */
export default function TutorialOverviewContent({
  onSelectStep,
}: {
  onSelectStep: (id: string) => void;
}) {
  const groups = groupByPage(tutorialSteps);

  if (groups.length === 0) {
    return <p className="text-sm text-gray-400">No tutorial steps yet.</p>;
  }

  return (
    <>
      {groups.map(([page, steps]) => (
        <section key={page} className="mb-10">
          <h2 className="text-m font-semibold tracking-widest uppercase text-gray-400 mb-3">
            {page}
          </h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-5">
            {steps.map((step) => (
              <TutorialCard key={step.id} step={step} onSelect={onSelectStep} />
            ))}
          </div>
        </section>
      ))}
    </>
  );
}
