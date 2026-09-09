#!/usr/bin/env node
// Records "Get Flow" - click Flow to compute the graph's Pauli flow. Backend: findPauliFlow
// (Flow.cpp), an O(n^3) algorithm per Theorem 4.4 in Mitosek & Backens (arXiv:2410.23439).
// A successful result reorders nodes into flow-ordered layers (see orderNodesByFlow in
// useGraphApi.ts) and enables the Simulate button. Seeds the simplest possible pattern - one
// input directly wired to one output - which trivially has flow. See README.md for
// setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChromeExceptPanel } from './lib/chrome.js';
import { zoomGraph } from './lib/svg.js';
import { showCursor, hoverWithCursor } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const ZOOM_SCALE = 2.2;
const TRIM_START_SECONDS = 3.6;

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

async function settleRecenterAndZoom(page) {
  await page.waitForTimeout(1200);
  await page.keyboard.press('c');
  await page.waitForTimeout(400);
  await zoomGraph(page, ZOOM_SCALE);
}

async function main() {
  await assertServersUp();

  const { page, finish } = await startRecording({
    name: 'get-flow',
    width: 1100,
    height: 619, // 16:9, roomy enough that the control panel doesn't cover the graph
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  // A failed flow computation alert()s in the app - auto-dismiss defensively so a bad seed
  // can't hang the recording.
  page.on('dialog', async (dialog) => {
    console.error(`Unexpected dialog: ${dialog.message()}`);
    await dialog.dismiss();
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // Simplest possible pattern: one input wired straight to one output.
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

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1200);

    const flowButton = page.locator('button', { hasText: 'Get the Pauli Flow' });
    await hoverWithCursor(page, flowButton);
    await page.waitForTimeout(1000);
    await flowButton.click();

    await page.waitForTimeout(2500);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
