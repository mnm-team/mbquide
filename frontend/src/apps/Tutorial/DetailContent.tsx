import { TutorialStep } from './types';

/**
 * A single tutorial step's media + description + optional deeper sections - shared between
 * the routed /TUTORIAL/:id page (Detail.tsx) and TutorialOverlay. The caller is responsible
 * for whatever "back" affordance makes sense for it (navigate vs. switch overlay state).
 */
export default function TutorialDetailContent({ step }: { step: TutorialStep }) {
  return (
    <>
      <p className="text-xs font-semibold tracking-widest uppercase text-gray-400 mb-2">
        {step.page}
      </p>
      <h1 className="text-2xl font-semibold text-gray-800 mb-6">{step.title}</h1>

      <div className="border border-gray-200 rounded-xl bg-slate-900 overflow-hidden mb-6 shadow-sm">
        {step.media.type === 'video' ? (
          <video
            src={step.media.src}
            autoPlay
            loop
            muted
            playsInline
            className="w-full max-h-[70vh] object-contain"
          />
        ) : (
          <img src={step.media.src} alt={step.title} className="w-full max-h-[70vh] object-contain" />
        )}
      </div>

      <p className="text-base text-gray-700 leading-relaxed mb-10">{step.description}</p>

      {step.sections?.map((section) => (
        <section key={section.heading} className="mb-10">
          <h2 className="text-lg font-semibold text-gray-800 mb-3">{section.heading}</h2>
          <div className="space-y-3">
            {section.paragraphs.map((paragraph, i) => (
              <p key={i} className="text-sm text-gray-600 leading-relaxed">
                {paragraph}
              </p>
            ))}
          </div>
        </section>
      ))}
    </>
  );
}
