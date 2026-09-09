#!/usr/bin/env node
// Records "building mode: right-click + drag between two nodes to create or remove an edge"
// - edgeCreation.ts is a right-button-only d3 drag (`.filter(event => event.button === 2)`)
// that toggles the edge between whatever two nodes it starts and ends on. See README.md for
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

const ZOOM_SCALE = 2.5;
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

async function rightClickDrag(page, from, to) {
  await moveMouse(page, from.x, from.y);
  await page.waitForTimeout(500);
  await page.mouse.down({ button: 'right' });
  await sweepMouse(page, from, to, 20, 40);
  await page.waitForTimeout(300);
  await page.mouse.up({ button: 'right' });
}

async function main() {
  await assertServersUp();

  const { page, finish } = await startRecording({
    name: 'edge-create-remove',
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

    // Two disconnected XY nodes, no edge between them yet.
    await seedGraph(page, {
      size: 2,
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
      },
    });

    await page.reload();
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1000);

    await page.keyboard.press('b'); // enable building mode - edge drag only attaches when on
    await page.waitForTimeout(600);

    const node0Screen = await svgPointToScreen(page, 'g.node[data-node-id="0"]', 0, 0);
    const node1Screen = await svgPointToScreen(page, 'g.node[data-node-id="1"]', 0, 0);

    // First drag creates the edge.
    await rightClickDrag(page, node0Screen, node1Screen);
    await page.waitForTimeout(1500); // let the new edge sit on screen

    // Second drag between the same pair removes it again (toggle behavior).
    await rightClickDrag(page, node1Screen, node0Screen);
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
