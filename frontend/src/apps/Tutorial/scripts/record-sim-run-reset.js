#!/usr/bin/env node
// Records "Simulator: Run All / Reset" - Run All measures every remaining node in one click
// ({measureAll: true} to /api/sim, Simulator_App.tsx's handleRunAll), Reset restores the
// simulator back to its just-initialized state ({init: true, ...} again, same as the initial
// load - handleResetGraph). Seeds the simplest valid pattern and runs it to completion, then
// resets it. See README.md for setup/run instructions.

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
    name: 'sim-run-reset',
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

    // A 3-node chain so Run All visibly measures more than one node in the same click.
    await seedGraph(page, {
      size: 3,
      inputs: [0],
      outputs: [2],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'OUTPUT', angle: '0' },
      },
      edges: [[0, 1], [1, 2]],
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

    const runAllButton = page.locator('button', { hasText: 'Execute all remaining Steps' });
    await hoverWithCursor(page, runAllButton);
    await page.waitForTimeout(1000);
    await runAllButton.click();

    await page.waitForTimeout(2000);

    const resetButton = page.locator('button', { hasText: 'Reset Simulator' });
    await hoverWithCursor(page, resetButton);
    await page.waitForTimeout(1000);
    await resetButton.click();

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
