import React from 'react';
import { useNavigate } from "react-router-dom";

import { ActionButton } from './Buttons';
import { CodeIcon, MBQCIcon, MeasurementIcon, ZXIcon, actionIcon, undoIcon, redoIcon, flowIcon, focusIcon, resetIcon, RunAllIcon } from './Icons';

type ControlPanelProps = {
  selectedCount?: number;
  canUndo?: boolean;
  canRedo?: boolean;
  onUndo?: () => void;
  onRedo?: () => void;
  onResetGraph?: () => void;
  onResetSim?: () => void;
  resetGraphDisabled?: boolean;
  flowFocusable?: boolean;
  simulatable?: boolean;
  isLCable?: boolean;
  isPivotable?: boolean;
  isZDeletable?: boolean;
  fitForRelabeling?: boolean;
  areNonPlanar?: boolean;
  canResetSim?: boolean;
  canRunAll?: boolean;
  onPrintNodes?: () => void;
  onLocalComplementation?: () => void;
  onPivot?: () => void;
  onZInsertion?: () => void;
  onZDeletion?: () => void;
  onRelabeling?: () => void;
  onRelabelingPlanar?: () => void;
  onTransformToZX?: () => void;
  onTransformToMBQC?: () => void;
  onGetFlow?: () => void;
  onFocusFlow?: () => void;
  onSimulate?: () => void;
  onRunAll?: () => void;
};

export const ControlPanel: React.FC<ControlPanelProps> = (props) => {
  const navigate = useNavigate();
  const {
    selectedCount = 0,
    canUndo,
    canRedo,
    flowFocusable,
    simulatable,
    isLCable,
    isPivotable,
    isZDeletable,
    fitForRelabeling,
    areNonPlanar,
    canResetSim,
    canRunAll,
    onPrintNodes,
    onLocalComplementation,
    onPivot,
    onZInsertion,
    onZDeletion,
    onRelabeling,
    onRelabelingPlanar,
    onUndo,
    onRedo,
    onResetGraph,
    onResetSim,
    resetGraphDisabled,
    onTransformToZX,
    onTransformToMBQC,
    onGetFlow,
    onFocusFlow,
    onSimulate,
    onRunAll,
  } = props;

  const navigationButtons = [
    { onClick: () => navigate("/QASM"), disabled: false, label: 'New QASM', sublabel: 'Input new Qasm', icon: <CodeIcon />, show: true, },
    { onClick: onTransformToZX, disabled: false, label: 'To ZX', sublabel: 'Transform to ZX', icon: <ZXIcon />, show: !!onTransformToZX, },
    { onClick: onTransformToMBQC, disabled: false, label: 'To MBQC', sublabel: 'Transform to MBQC', icon: <MBQCIcon />, show: !!onTransformToMBQC, },
  ];

  const operationButtons = [
    { onClick: onPrintNodes, disabled: selectedCount === 0, label: 'Print', show: !!onPrintNodes },
    { onClick: onResetGraph, disabled: resetGraphDisabled, label: resetIcon + ' Reset', sublabel: 'Reset to empty Graph', show: !!onResetGraph },
    { onClick: onResetSim, disabled: !canResetSim, label: resetIcon + ' Reset', sublabel: 'Reset Simulator', show: !!onResetSim },
    { onClick: onLocalComplementation, disabled: !isLCable, label: actionIcon + ' LC', sublabel: 'Local Complementation', show: !!onLocalComplementation },
    { onClick: onPivot, disabled: !isPivotable, label: actionIcon + ' Pivot', sublabel: 'Pivot about an edge', show: !!onPivot },
    { onClick: onZInsertion, disabled: selectedCount === 0, label: actionIcon + ' Z-Insert', sublabel: 'Insert a Z Node', show: !!onZInsertion },
    { onClick: onZDeletion, disabled: !isZDeletable, label: actionIcon + ' Z-Delete', sublabel: 'Z Measurement Elimination', show: !!onZDeletion },
    // { onClick: onRelabeling, disabled: !fitForRelabeling || selectedCount !== 1, label: actionIcon + ' Relabel', sublabel: 'Relabel to axis basis', show: !!onRelabeling },
    // { onClick: onRelabelingPlanar, disabled: !areNonPlanar || selectedCount !== 1, label: actionIcon + ' Relabel Planar', sublabel: 'Relabel to planar basis', show: !!onRelabelingPlanar },
  ];

  const undoButtons = [
    { onClick: onUndo, disabled: !canUndo, label: undoIcon + ' Undo', show: !!onUndo },
    { onClick: onRedo, disabled: !canRedo, label: redoIcon + ' Redo', show: !!onRedo },
  ];

  const simButtons = [
    { onClick: onGetFlow, disabled: false, label: flowIcon + ' Flow', sublabel: 'Get the Pauli Flow', show: !!onGetFlow },
    // { onClick: onFocusFlow, disabled: !flowFocusable, label: focusIcon + ' Focus', sublabel: 'Focus the Pauli Flow', show: !!onFocusFlow },
    { onClick: onSimulate, disabled: !simulatable, label: 'Simulate', sublabel: 'Simulate this graph flow', icon: <MeasurementIcon />, show: !!onSimulate },
    { onClick: onRunAll, disabled: !canRunAll, label: ' Run All', sublabel: 'Execute all remaining Steps', icon: <RunAllIcon />, show: !!onRunAll },
  ];

  const renderButtonGroup = (buttons: typeof navigationButtons) => {
    const visibleButtons = buttons.filter(b => b.show);

    if (visibleButtons.length === 0) return null; // don't render empty groups

    return (
      <div
        className={`flex justify-around gap-4 pr-4 items-center ${
          // add border only if not last group
          'border-r last:border-r-0'
        }`}
      >
        {visibleButtons.map((b, i) => (
          <div className='w-auto'>
            <ActionButton
              key={i}
              onClick={b.onClick}
              disabled={b.disabled}
              label={b.label}
              sublabel={b.sublabel}
              icon={b.icon}
            />
          </div>
        ))}
      </div>
    );
  };

  return (
    <div className="flex items-center justify-center gap-6 px-4 py-2">
      {renderButtonGroup(navigationButtons)}
      {renderButtonGroup(operationButtons)}
      {renderButtonGroup(undoButtons)}
      {renderButtonGroup(simButtons)}
    </div>
  );

};
