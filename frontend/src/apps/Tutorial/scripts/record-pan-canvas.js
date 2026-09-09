#!/usr/bin/env node
// Records "panning the canvas" - holding Ctrl and dragging on empty space to pan the view.
// See README.md in this folder for setup/run instructions.

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

const ZOOM_SCALE = 2.2;
const TRIM_START_SECONDS = 3.6; // measured from an untrimmed run: settle+recenter+zoom+beat

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
    name: 'pan-canvas',
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

    // A small diamond of 4 nodes so the pan motion is visually obvious.
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
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1500);

    // Ctrl + left-drag on empty space pans the canvas (plain drag would rubber-band select
    // instead - see useSvgPan.ts's isCtlLeftClick check). The graph settles into a vertical
    // line down the middle of the canvas for this seeded diamond, so start the drag from a
    // corner well clear of any node - starting a Ctrl-drag *on* a node hits the node's own
    // drag/select behavior instead (dragBehavior.ts only filters on button, not ctrlKey).
    const start = { x: 90, y: 300 };
    const swing1 = { x: 420, y: 110 };
    const swing2 = { x: 160, y: 260 };

    await moveMouse(page, start.x, start.y);
    await page.waitForTimeout(400);
    await page.keyboard.down('Control');
    await page.mouse.down();
    await sweepMouse(page, start, swing1, 26, 40);
    await page.waitForTimeout(300);
    await sweepMouse(page, swing1, swing2, 30, 40);
    await page.waitForTimeout(300);
    await sweepMouse(page, swing2, start, 20, 40);
    await page.mouse.up();
    await page.keyboard.up('Control');
    await page.waitForTimeout(800);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
