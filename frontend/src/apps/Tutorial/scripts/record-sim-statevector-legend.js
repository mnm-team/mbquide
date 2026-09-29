#!/usr/bin/env node
// Records "reading the Statevector panel" - each row's bar width is |amplitude|^2 (P) and
// its color comes from the amplitude's phase (atan2(imag, real), mapped through
// hsl(phase, 70%, 50%) - see Statevector.tsx), with a legend gradient at the bottom of the
// panel. Submits a state with two different phases (0 degrees vs 90 degrees) so the two
// rows actually render in different colors, instead of the same-phase example used in the
// "Custom input statevector" recording. See README.md for setup/run instructions.

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
    name: 'sim-statevector-legend',
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

    await postToBackend(page, 'graph', { flow: 'pauli' });
    await postToBackend(page, 'graph', { simulate: true, random: true, input: '' });

    await page.goto(`${FRONTEND_URL}/SIM`);
    await disableTextSelection(page);
    await showCursor(page);

    const amplitudeInputs = page.locator('input[placeholder="0 + 0i"]');
    await amplitudeInputs.first().waitFor({ state: 'visible' });
    await page.waitForTimeout(800);
    await page.keyboard.press('c');
    await page.waitForTimeout(600);

    // |0> gets phase 0 deg (real, red on the legend), |1> gets phase 90 deg (imaginary,
    // yellow-green) - two clearly different colors instead of the same-phase example used
    // in the "Custom input statevector" recording.
    await moveCursorToLocator(page, amplitudeInputs.nth(0));
    await amplitudeInputs.nth(0).click();
    await page.keyboard.type('1/sqrt(2)', { delay: 80 });
    await page.waitForTimeout(400);

    await moveCursorToLocator(page, amplitudeInputs.nth(1));
    await amplitudeInputs.nth(1).click();
    await page.keyboard.type('1/sqrt(2)i', { delay: 80 });
    await page.waitForTimeout(600);

    await page.locator('text=/‖ψ‖.*1.*✓/').waitFor({ state: 'visible' });
    await page.waitForTimeout(500);

    const submitButton = page.locator('button', { hasText: 'Submit' });
    await hoverWithCursor(page, submitButton);
    await page.waitForTimeout(800);
    await submitButton.click();

    // Hold on the result so the panel's two differently-colored rows and the legend
    // gradient at the bottom both read clearly.
    await page.waitForTimeout(3000);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
