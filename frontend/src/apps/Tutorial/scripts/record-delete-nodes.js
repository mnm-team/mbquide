#!/usr/bin/env node
// Records "building mode: Delete/Backspace removes the selected node(s)" -
// useGraphSimulation.ts's own keydown handler only acts on Delete/Backspace when
// buildingMode is on and there's a non-empty selection (handleNodeDelete in
// apps/MBQC/index.tsx then remaps remaining node ids to stay contiguous). Rubber-band
// selects a pair of nodes, then deletes both in one keypress. See README.md for
// setup/run instructions.

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

const ZOOM_SCALE = 1.7;
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
    name: 'delete-nodes',
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

    // Same two-branch tree as the rubber-band-select / drag-nodes recordings.
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
    await page.waitForTimeout(1000);

    await page.keyboard.press('b'); // enable building mode - Delete only acts then
    await page.waitForTimeout(600);

    // Rubber-band select nodes 1 and 3 (same pair used elsewhere).
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
    await page.waitForTimeout(800); // let the glow on both selected nodes register

    await page.keyboard.press('Delete');
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
