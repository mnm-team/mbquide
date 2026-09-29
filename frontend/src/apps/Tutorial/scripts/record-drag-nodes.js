#!/usr/bin/env node
// Records "dragging selected node(s) to reposition" - rubber-band selects a pair of nodes,
// then drags one of them, moving the whole selection together preserving their relative
// offsets (dragBehavior.ts: if the dragged node is already in the current selection, the
// whole selection moves as a group). See README.md in this folder for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChrome } from './lib/chrome.js';
import { svgPointToScreen, zoomGraph } from './lib/svg.js';
import { showCursor, moveMouse, sweepMouse } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const ZOOM_SCALE = 2;
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
    name: 'drag-nodes',
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

    // Same two-branch tree as the rubber-band-select recording, for the same 2D spread.
    await seedGraph(page, {
      size: 5,
      inputs: [0],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'XY', angle: 'π/2' },
        '3': { basis: 'XY', angle: 'π/2' },
        '4': { basis: 'XY', angle: 'π/2' },
      },
      edges: [[0, 1], [1, 2], [0, 3], [3, 4]],
    });

    await page.reload();
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1500);

    // Select nodes 1 and 3 (same layer, stacked vertically) with a rubber-band drag first.
    const node1Screen = await svgPointToScreen(page, 'g.node[data-node-id="1"]', 0, 0);
    const node3Screen = await svgPointToScreen(page, 'g.node[data-node-id="3"]', 0, 0);

    const margin = 55;
    const boxX = node1Screen.x;
    const boxTop = Math.min(node1Screen.y, node3Screen.y) - margin;
    const boxBottom = Math.max(node1Screen.y, node3Screen.y) + margin;
    const selectStart = { x: boxX - margin, y: boxTop };
    const selectEnd = { x: boxX + margin, y: boxBottom };

    await moveMouse(page, selectStart.x, selectStart.y);
    await page.waitForTimeout(500);
    await page.mouse.down();
    await sweepMouse(page, selectStart, selectEnd, 20, 35);
    await page.mouse.up();
    await page.waitForTimeout(700); // let the glow on both selected nodes register

    // Now drag from directly on node 1 - since it's already part of the selection, the whole
    // pair (1 and 3) moves together, preserving their relative offset (dragBehavior.ts).
    const dragTarget = { x: node1Screen.x + 130, y: node1Screen.y - 90 };

    await moveMouse(page, node1Screen.x, node1Screen.y);
    await page.waitForTimeout(500);
    await page.mouse.down();
    await sweepMouse(page, node1Screen, dragTarget, 26, 40);
    await page.waitForTimeout(500);
    await page.mouse.up();
    await page.waitForTimeout(1200);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
