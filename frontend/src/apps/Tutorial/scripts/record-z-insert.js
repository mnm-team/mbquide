#!/usr/bin/env node
// Records "Z-Insert" - select one or more nodes and click Z-Insert to add a fresh Z-basis
// node connected to all of them. Backend: MBQC_Graph::ZInsertion (MBQC_Graph.cpp) creates a
// new Z-basis (angle 0) node wired to the given vertex set. Seeds a single XY node, selects
// it, and inserts a Z node connected to it. See README.md for setup/run instructions.

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

const ZOOM_SCALE = 2.2;
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
    name: 'z-insert',
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
      size: 1,
      meas: { '0': { basis: 'XY', angle: 'π/2' } },
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

    const zInsertButton = page.locator('button', { hasText: 'Insert a Z Node' });
    await hoverWithCursor(page, zInsertButton);
    await page.waitForTimeout(1000);
    await zInsertButton.click();

    await page.waitForSelector('g.node[data-node-id="1"]');
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
