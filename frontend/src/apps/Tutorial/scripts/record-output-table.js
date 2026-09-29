#!/usr/bin/env node
// Records "the In/Out/Sign table next to an OUTPUT node" - renderOutputTables.ts draws it
// from outputAdjustments[nodeId], a 2x2 table (rows X and Z) tracking how that output
// qubit's ideal Pauli observables have been conjugated by graph edits so far; a fresh output
// starts at the identity (Out X / Sign +, Out Z / Sign +). Seeds a Z-basis node (angle π)
// directly wired to an OUTPUT node, then Z-Deletes it - ZDeletion's own effect on an OUTPUT
// neighbor (MBQC_Graph.cpp: outputAdjustments[v].adjustOutput("Z") when the deleted angle is
// pi) flips the output's X-row sign from + to -, while the Z row stays unchanged, since
// conjugating X by Z anticommutes but conjugating Z by Z doesn't. See README.md for
// setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChromeExceptPanel } from './lib/chrome.js';
import { zoomGraph } from './lib/svg.js';
import { showCursor, hoverWithCursor, moveCursorToLocator } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const ZOOM_SCALE = 2.6;
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
    name: 'output-table',
    width: 1100,
    height: 619, // 16:9, roomy enough that the control panel doesn't cover the graph
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // A Z-basis node (angle pi, Z-deletable) directly wired to an OUTPUT node.
    await seedGraph(page, {
      size: 2,
      outputs: [1],
      meas: {
        '0': { basis: 'Z', angle: 'π' },
        '1': { basis: 'OUTPUT', angle: '0' },
      },
      edges: [[0, 1]],
    });

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1500); // let the starting "Out X / Sign +, Out Z / Sign +" table sit on screen

    const node0 = page.locator('g.node[data-node-id="0"]');
    await moveCursorToLocator(page, node0);
    await page.waitForTimeout(300);
    await node0.click();
    await page.waitForTimeout(800);

    const zDeleteButton = page.locator('button', { hasText: 'Z Measurement Elimination' });
    await hoverWithCursor(page, zDeleteButton);
    await page.waitForTimeout(1000);
    await zDeleteButton.click();

    // After deletion only the OUTPUT node remains - wait for its table's X-row sign to flip.
    await page.waitForFunction(() => {
      const signs = Array.from(document.querySelectorAll('text.sign-X'));
      return signs.some((el) => el.textContent === '−');
    });
    await page.waitForTimeout(2200);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
