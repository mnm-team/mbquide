#!/usr/bin/env node
// Records "Simulator: the Random checkbox" - toggling it immediately re-initializes the
// simulator with the new setting (handleRandomCheckBoxClicked -> resetGraph(undefined,
// checked) in Simulator_App.tsx). Unchecking it, then running the same pattern twice with a
// Reset in between, demonstrates that outcomes become repeatable instead of sampled - see
// README.md for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph, postToBackend } from './lib/graphApi.js';
import { disableTextSelection } from './lib/chrome.js';
import { showCursor, hoverWithCursor } from './lib/cursor.js';

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
    name: 'sim-random-toggle',
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

    // Single XY-basis input measured directly, so one click is enough to see an outcome.
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
    await page.waitForSelector('g[data-node-id]');
    await page.waitForTimeout(800);
    await page.keyboard.press('c');
    await page.waitForTimeout(800);

    // The real <input type="checkbox"> is visually hidden (sr-only, a11y pattern) behind a
    // styled sibling <div> - target the enclosing <label> (the actual clickable surface),
    // not the input itself.
    const randomCheckbox = page.locator('label', { hasText: 'Random' });
    await hoverWithCursor(page, randomCheckbox);
    await page.waitForTimeout(800);
    await randomCheckbox.click(); // unchecks it - immediately reinitializes the simulator
    await page.waitForTimeout(1200);

    const runAllButton = page.locator('button', { hasText: 'Execute all remaining Steps' });
    await hoverWithCursor(page, runAllButton);
    await page.waitForTimeout(700);
    await runAllButton.click();
    await page.waitForTimeout(1800); // let the first outcome sit on screen

    const resetButton = page.locator('button', { hasText: 'Reset Simulator' });
    await hoverWithCursor(page, resetButton);
    await page.waitForTimeout(700);
    await resetButton.click();
    await page.waitForTimeout(1000);

    await hoverWithCursor(page, runAllButton);
    await page.waitForTimeout(700);
    await runAllButton.click();
    await page.waitForTimeout(2000); // let the repeated (same) outcome sit on screen
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
