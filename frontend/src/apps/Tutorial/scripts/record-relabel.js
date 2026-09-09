#!/usr/bin/env node
// Records "right-click a node (outside building mode) for the context menu, then Relabel to
// a different basis" - ContextMenu.tsx offers different "Relabel to <basis>" buttons
// depending on the right-clicked node's current basis (X -> XZ/XY, Y -> YZ/XY, Z -> XZ/YZ);
// this records the X -> XZ case as the representative example. See README.md for setup/run
// instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChrome } from './lib/chrome.js';
import { zoomGraph } from './lib/svg.js';
import { showCursor, hoverWithCursor, moveCursorToLocator } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const ZOOM_SCALE = 4;
const TRIM_START_SECONDS = 3.6; // same setup shape as record-yz-unfusion.js

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
    name: 'relabel',
    width: 600,
    height: 338, // matches the app SVG's own 16:9 aspect ratio - see README's "SVG letterboxing" note
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // A single X-basis node, phase 0.
    await seedGraph(page, {
      size: 1,
      meas: { '0': { basis: 'X', angle: '0' } },
    });

    await page.reload();
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1500);

    // Right-click opens the context menu (non-building-mode contextmenu handler in
    // nodeInteractions.ts) - same real native contextmenu event as record-yz-unfusion.js.
    const node0 = page.locator('g.node[data-node-id="0"]');
    await moveCursorToLocator(page, node0);
    await page.waitForTimeout(300);
    await node0.click({ button: 'right' });
    const relabelButton = page.locator('button', { hasText: 'Relabel to XZ' });
    await relabelButton.waitFor({ state: 'visible' });
    await page.waitForTimeout(500);

    await hoverWithCursor(page, relabelButton);
    await page.waitForTimeout(1200);
    await relabelButton.click();

    await page.waitForTimeout(400);
    // The basis label text is appended straight to the root <svg> (not nested under
    // g.node), and its join-key class ("label-t" in renderLabels.ts) is never actually set
    // on the element - so it can't be targeted by class. The node's own circle fill is a
    // reliable proxy for "relabel finished": it recolors from X's blue to XZ's purple.
    await page.waitForFunction(
      () => document.querySelector('g.node[data-node-id="0"] circle.node-shape')?.getAttribute('fill') === '#AA64B4'
    );
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
