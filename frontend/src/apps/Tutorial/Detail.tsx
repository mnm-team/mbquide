import { useParams, useNavigate } from 'react-router-dom';
import { tutorialSteps } from './data';
import TutorialQuickNav from './QuickNav';
import ShortcutsPanel from './ShortcutsPanel';
import TutorialDetailContent from './DetailContent';

export default function TutorialDetailApp() {
  const { id } = useParams();
  const navigate = useNavigate();
  const step = tutorialSteps.find((s) => s.id === id);

  if (!step) {
    return (
      <div className="h-screen w-full overflow-y-auto bg-slate-50 flex items-center justify-center">
        <TutorialQuickNav />
        <ShortcutsPanel />
        <div className="text-center">
          <p className="text-m text-gray-500 mb-3">No tutorial step found for "{id}".</p>
          <button
            onClick={() => navigate('/TUTORIAL')}
            className="text-m text-blue-600 hover:text-blue-800 transition-colors"
          >
            ← Back to tutorial
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="h-screen w-full overflow-y-auto bg-slate-50">
      <TutorialQuickNav />
      <ShortcutsPanel />
      <div className="max-w-3xl mx-auto px-6 py-10">
        <button
          onClick={() => navigate('/TUTORIAL')}
          className="text-sm text-gray-500 hover:text-gray-800 transition-colors mb-6"
        >
          ← Overview
        </button>

        <TutorialDetailContent step={step} />
      </div>
    </div>
  );
}
