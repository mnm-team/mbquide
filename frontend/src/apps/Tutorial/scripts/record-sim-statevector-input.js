#!/usr/bin/env node
// Records "Simulator: entering a custom input statevector" - typing amplitude expressions
// into the |psi> State grid at the top of the Simulator page. StateInput.tsx parses each
// cell with a small hand-rolled expression evaluator (supports sqrt(...), pi, i, +/-), and
// only enables Submit once the total probability (sum of |amplitude|^2 across filled cells)
// is within 1e-6 of 1. Seeds the simplest valid pattern (one input wired straight to one
// output) via the same Flow -> Simulate sequence the real "Simulate" button runs, then
// navigates straight to /SIM (same backend session, via the shared cookie). See README.md
// for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph, postToBackend } from './lib/graphApi.js';
import { disableTextSelection } from './lib/chrome.js';
import { showCursor, hoverWithCursor, moveCursorToLocator } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const TRIM_START_SECONDS = 2.2;

async function assertServersUp() {
  const targets = [
    ['frontend', FRONTEND_URL],
    ['backend', 'http://localhost:18080/api/graph'],
  ];
  for (const [label, url] of targets) {
    try {
      await fetch(url);
    } catch {
      throw new Error(`Could not reach the ${label} at ${url}. Start both servers first.`);
    }
  }
}

async function main() {
  await assertServersUp();

  const { page, finish } = await startRecording({
    name: 'sim-statevector-input',
    width: 1100,
    height: 650,
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await disableTextSelection(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // Simplest valid pattern: one input wired straight to one output - same seed as the
    // Get Flow recording.
    await seedGraph(page, {
      size: 2,
      inputs: [0],
      outputs: [1],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'OUTPUT', angle: '0' },
      },
      edges: [[0, 1]],
    });

    // Mirror what the real "Simulate" button does: compute flow, then initialize the
    // simulator (createGetFlowOperation / createSimulateOperation in api/operations.ts).
    await postToBackend(page, 'graph', { flow: 'pauli' });
    await postToBackend(page, 'graph', { simulate: true, random: true, input: '' });

    await page.goto(`${FRONTEND_URL}/SIM`);
    await disableTextSelection(page);
    await showCursor(page);

    const amplitudeInputs = page.locator('input[placeholder="0 + 0i"]');
    await amplitudeInputs.first().waitFor({ state: 'visible' });
    await page.waitForTimeout(800);
    await page.keyboard.press('c'); // center the (otherwise-offscreen) 2-node graph
    await page.waitForTimeout(600);

    // One input qubit -> two amplitude cells, for |0> and |1>. Type slowly so each
    // character reads clearly.
    await moveCursorToLocator(page, amplitudeInputs.nth(0));
    await amplitudeInputs.nth(0).click();
    await page.keyboard.type('1/sqrt(2)', { delay: 90 });
    await page.waitForTimeout(500);

    await moveCursorToLocator(page, amplitudeInputs.nth(1));
    await amplitudeInputs.nth(1).click();
    await page.keyboard.type('1/sqrt(2)', { delay: 90 });
    await page.waitForTimeout(700);

    // Norm hint reads "‖ψ‖² = 1 ✓" in green once normalized - wait for it rather than a
    // fixed delay, since the check runs on every keystroke.
    await page.locator('text=/‖ψ‖.*1.*✓/').waitFor({ state: 'visible' });
    await page.waitForTimeout(700);

    const submitButton = page.locator('button', { hasText: 'Submit' });
    await hoverWithCursor(page, submitButton);
    await page.waitForTimeout(1000);
    await submitButton.click();

    await page.waitForTimeout(1800);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
