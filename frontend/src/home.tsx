import { useNavigate } from "react-router-dom";
import { ActionButton } from "./components/Buttons";
import { CodeIcon, MBQCIcon } from "./components/Icons";
import lmuLogo from "./assets/lmu_logo.png";
import mnmLogo from "./assets/mnm_logo.png";

export default function Home() {
  const navigate = useNavigate();

  return (
    <div className="flex flex-col items-center min-h-screen text-center">

      <div className="flex flex-col items-center justify-center flex-1">
        <h1 className="mb-16">MBQuIDE</h1>

        <div className="flex justify-center gap-10">
          <div className="w-100">
            <ActionButton
              onClick={() => navigate("/QASM")}
              label="QASM"
              sublabel="Initialize an MBQC Pattern from a QASM circuit"
              icon={<CodeIcon />}
            />
          </div>

          <div className="w-100">
            <ActionButton
              onClick={() => navigate("/MBQC")}
              label="MBQC"
              sublabel="Directly start the MBQC Editor"
              icon={<MBQCIcon />}
            />
          </div>
        </div>
      </div>

      {/* <div className="flex w-full items-center justify-evenly mb-6 opacity-70">
        <img src={mnmLogo} alt="MNM Logo" className="h-25 object-contain" />
        <img src={lmuLogo} alt="LMU Logo" className="h-25 object-contain" />
      </div> */}

    </div>
  );
}
