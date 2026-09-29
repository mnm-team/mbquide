#!/usr/bin/env node
// Records "building mode: drag the INPUT palette icon onto a basis icon to set its input
// basis, then drop it on canvas" - exampleDrag.ts only lets an INPUT drag pick up a basis by
// hovering directly over the X, Y, or XY palette icon (a 30px radius hit-test against their
// fixed screen positions); dropping without ever having hovered one is a silent no-op. See
// README.md for setup/run instructions.
//
// Framing: see record-building-drag-node.js's own note - zoomed in anchored on the palette
// itself (not the usual centered zoom, since the palette isn't part of the pannable graph
// content), and the node is only dragged a short distance to its immediate left afterward.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { svgPointToScreen, zoomGraph } from './lib/svg.js';
import { disableTextSelection } from './lib/chrome.js';
import { showCursor, moveMouse, sweepMouse } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const TRIM_START_SECONDS = 2.7; // same setup shape as record-building-drag-node.js

// See record-building-drag-node.js for the derivation of these two constants.
const PALETTE_X = 1850;
const ZOOM_SCALE = 1.8;
const ZOOM_ORIGIN_Y = 326.25;
const ZOOM_ORIGIN = `${(1920 / 1920) * 100}% ${(ZOOM_ORIGIN_Y / 1080) * 100}%`;

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

async function hidePanelAndToggle(page) {
  await page.addStyleTag({
    content:
      '[data-tutorial-hide="control-panel"], [data-tutorial-hide="building-mode-toggle"] { display: none !important; }',
  });
  await disableTextSelection(page);
  await showCursor(page);
}

async function main() {
  await assertServersUp();

  const { page, finish } = await startRecording({
    name: 'building-drag-input',
    width: 800,
    height: 450, // 16:9 - see README's "SVG letterboxing" note
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hidePanelAndToggle(page);
    await page.waitForSelector('svg');

    await seedGraph(page, { size: 0 });

    await page.reload();
    await hidePanelAndToggle(page);
    await page.waitForSelector('g.example-10008'); // the INPUT palette icon
    await page.waitForTimeout(800);

    await page.keyboard.press('b'); // enable building mode
    await page.waitForTimeout(600);

    await zoomGraph(page, ZOOM_SCALE, 'svg', ZOOM_ORIGIN);
    await page.waitForTimeout(600);

    const inputIconScreen = await svgPointToScreen(page, 'g.example-10008', 0, 0);
    const xyIconScreen = await svgPointToScreen(page, 'g.example-10004', 0, 0);
    // A short drag to the icon's immediate left, not across the whole canvas.
    const dropTarget = await svgPointToScreen(page, 'svg', PALETTE_X - 340, 440);

    await moveMouse(page, inputIconScreen.x, inputIconScreen.y);
    await page.waitForTimeout(1000); // hover so it's clear which icon is about to move
    await page.mouse.down();

    // Drag onto the XY icon first - the clone (a square, since it's the INPUT icon's own
    // shape) recolors to XY's green once within the 30px hit-test radius, showing the basis
    // pick registering before continuing on to the canvas.
    await sweepMouse(page, inputIconScreen, xyIconScreen, 18, 40);
    await page.waitForTimeout(900); // hold on the XY icon so the recolor reads clearly

    await sweepMouse(page, xyIconScreen, dropTarget, 18, 40);
    await page.waitForTimeout(400);
    await page.mouse.up();

    await page.waitForSelector('g.node[data-node-id="0"]');
    await page.waitForTimeout(1500);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
