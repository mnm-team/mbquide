import { useNavigate } from "react-router-dom";
import { ActionButton } from "../../components/Buttons";
import { CodeIcon, MBQCIcon } from "../../components/Icons";
import { useTutorialOverlay } from "./TutorialOverlayContext";
import mbquideLogo from "../../assets/mbquide.png";

type TutorialQuickNavProps = {
  // In overlay mode the underlying page is a real app page you're already on, so the
  // QASM/MBQC jump buttons would just be a redundant (and disorienting, since they close
  // the overlay first) way to navigate away - only the logo makes sense there.
  showNavButtons?: boolean;
};

/**
 * Fixed-to-the-viewport logo + QASM/MBQC shortcut, pinned in the page's left margin -
 * outside (to the left of) the centered, independently-scrolling content column both
 * Tutorial pages use, so it stays put as that column scrolls. Shared between index.tsx (the
 * overview grid), Detail.tsx (a single step), and TutorialOverlay.tsx (logo only) rather than
 * duplicated, so all three stay in sync.
 */
export default function TutorialQuickNav({ showNavButtons = true }: TutorialQuickNavProps) {
  const navigate = useNavigate();
  // Closing the overlay (a no-op on the routed /TUTORIAL pages, where it's already closed)
  // before navigating means clicking QASM/MBQC from inside the overlay actually shows you
  // that page, instead of navigating underneath a still-open full-screen overlay.
  const { close } = useTutorialOverlay();

  const goTo = (path: string) => {
    close();
    navigate(path);
  };

  return (
    <div className="fixed top-6 left-6 z-40 flex flex-col gap-3">
      <img
        src={mbquideLogo}
        alt="MBQuIDE logo"
        className="w-70 object-contain rounded-lg"
      />
      {showNavButtons && (
        <div className="flex items-center gap-2">
          <div className="w-32">
            <ActionButton
              onClick={() => goTo("/QASM")}
              label="QASM"
              sublabel="Input a QASM circuit"
              sublabelAsTooltip
              icon={<CodeIcon size={18} />}
            />
          </div>
          <div className="w-32">
            <ActionButton
              onClick={() => goTo("/MBQC")}
              label="MBQC"
              sublabel="Enter the MBQC Editor"
              sublabelAsTooltip
              icon={<MBQCIcon size={18} />}
            />
          </div>
        </div>
      )}
    </div>
  );
}
