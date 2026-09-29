#!/usr/bin/env node
// Records "Z-Delete" - select nodes in Z/XZ/YZ basis with phase 0 or π and click Z-Delete to
// remove them. Backend: MBQC_Graph::ZDeletion (MBQC_Graph.cpp) removes the node and folds a
// sign into each neighbor's phase - an XY/X/Y neighbor's angle shifts by the deleted node's
// angle (0 or π), an XZ/YZ neighbor's angle negates if the deleted angle was π. Seeds a
// Z-basis node (angle π) connected to an XY node (angle π/2): after deletion the XY node's
// phase visibly shifts from π/2 to 3π/2 (+π). See README.md for setup/run instructions.

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

const ZOOM_SCALE = 3;
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
    name: 'z-delete',
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

    await seedGraph(page, {
      size: 2,
      meas: {
        '0': { basis: 'Z', angle: 'π' },
        '1': { basis: 'XY', angle: 'π/2' },
      },
      edges: [[0, 1]],
    });

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1000);

    const node0 = page.locator('g.node[data-node-id="0"]');
    await moveCursorToLocator(page, node0);
    await page.waitForTimeout(300);
    await node0.click();
    await page.waitForTimeout(800);

    const zDeleteButton = page.locator('button', { hasText: 'Z Measurement Elimination' });
    await hoverWithCursor(page, zDeleteButton);
    await page.waitForTimeout(1000);
    await zDeleteButton.click();

    await page.waitForFunction(() => document.querySelectorAll('g.node').length === 1);
    await page.waitForTimeout(2000);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
