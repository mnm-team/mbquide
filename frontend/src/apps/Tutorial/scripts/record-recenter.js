#!/usr/bin/env node
// Records "press C to recenter" - pans the graph off-screen (which fades in the app's own
// "Press C to recenter" hint, RecenterHint.tsx), then presses C to snap the view straight
// back to centered on the graph's bounding box. See README.md for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChrome } from './lib/chrome.js';
import { zoomGraph } from './lib/svg.js';
import { showCursor, moveMouse, sweepMouse } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const ZOOM_SCALE = 2.0;
const TRIM_START_SECONDS = 3;

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
    name: 'recenter',
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

    // Same 4-node diamond as record-pan-canvas.js.
    await seedGraph(page, {
      size: 4,
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'XY', angle: 'π/2' },
        '3': { basis: 'XY', angle: 'π/2' },
      },
      edges: [[0, 1], [1, 2], [2, 3], [3, 0]],
    });

    await page.reload();
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    // The graph auto-centers on mount (the "Reset Pan to Center" effect in Graph/index.tsx
    // runs on first render too), so no need to press "c" before the first zoom here.
    await page.keyboard.press('c');
    await page.waitForTimeout(1200);
    await zoomGraph(page, ZOOM_SCALE);
    await page.waitForTimeout(1000);

    // Ctrl+drag far enough that every node leaves the viewport - starting from empty space,
    // same gotcha as record-pan-canvas.js (starting on a node hits its own drag instead).
    const start = { x: 90, y: 280 };
    const target = { x: 900, y: 280 };

    await moveMouse(page, start.x, start.y);
    await page.waitForTimeout(400);
    await page.keyboard.down('Control');
    await page.mouse.down();
    await sweepMouse(page, start, target, 30, 35);
    await page.mouse.up();
    await page.keyboard.up('Control');

    // RecenterHint fades in over 700ms and auto-hides after 2000ms if untouched - press C
    // comfortably inside that window so the hint is visibly on screen right before it acts.
    await page.waitForTimeout(700);
    await page.keyboard.press('c');
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
