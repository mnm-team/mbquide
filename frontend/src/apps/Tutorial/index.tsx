import { useNavigate } from 'react-router-dom';
import TutorialQuickNav from './QuickNav';
import ShortcutsPanel from './ShortcutsPanel';
import TutorialOverviewContent from './OverviewContent';

export default function TutorialApp() {
  const navigate = useNavigate();

  return (
    <div className="h-screen w-full overflow-y-auto bg-slate-50">
      <TutorialQuickNav />
      <ShortcutsPanel />
      <div className="max-w-5xl mx-auto px-6 py-10">
        <div className="mb-8">
          <h1 className="text-2xl font-semibold text-gray-800">Help &amp; Tutorials</h1>
          <p className="text-sm text-gray-500 mt-0.5">
            Short demonstrations of what you can do in MBQuIDE.
          </p>
        </div>

        <TutorialOverviewContent onSelectStep={(id) => navigate(`/TUTORIAL/${id}`)} />
      </div>
    </div>
  );
}
