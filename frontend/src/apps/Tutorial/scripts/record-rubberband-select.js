#!/usr/bin/env node
// Records "rubber-band drag to multi-select nodes" - a plain (no-Ctrl) click-drag on empty
// canvas space draws a selection box; nodes whose center falls inside it get selected.
// See README.md in this folder for setup/run instructions.

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
    name: 'rubberband-select',
    width: 600,
    // Matches the app SVG's own 1920x1080 (16:9) aspect ratio - see README's "SVG
    // letterboxing" note; a 4:3 recording box leaves a gap showing the app's near-black
    // page background unless zoom is large enough to visually cover it.
    height: 338,
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // A small two-branch tree from input node 0 so getNodePosition() (positioning.ts) lays
    // it out in 3 distinct x-layers: {0} at layer 0, {1,3} at layer 1, {2,4} at layer 2 -
    // giving real 2D spread to draw a selection box around, instead of the single vertical
    // column a plain path graph produces.
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

    // Nodes 1 and 3 share layer 1 (same x, different y) - box around just that pair.
    const node1Screen = await svgPointToScreen(page, 'g.node[data-node-id="1"]', 0, 0);
    const node3Screen = await svgPointToScreen(page, 'g.node[data-node-id="3"]', 0, 0);

    const margin = 55;
    const boxX = node1Screen.x; // same as node3Screen.x (shared layer)
    const boxTop = Math.min(node1Screen.y, node3Screen.y) - margin;
    const boxBottom = Math.max(node1Screen.y, node3Screen.y) + margin;
    const boxLeft = boxX - margin;
    const boxRight = boxX + margin;

    // Start the drag from empty space above-left of the pair (nodes render above the brush
    // overlay in z-order, so starting exactly on a node would trigger its own drag/select
    // instead of the rubber-band brush - same gotcha as the Ctrl-pan script).
    const start = { x: boxLeft, y: boxTop };
    const end = { x: boxRight, y: boxBottom };

    await moveMouse(page, start.x, start.y);
    await page.waitForTimeout(500);
    await page.mouse.down();
    await sweepMouse(page, start, end, 24, 40);
    await page.waitForTimeout(600); // let the selection box sit visibly before releasing
    await page.mouse.up();

    // Selected nodes get a glow filter (see useSimulatorRendering/useGraphSimulation) -
    // hold on the result for a beat so it's clear the pair stayed selected after release.
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
