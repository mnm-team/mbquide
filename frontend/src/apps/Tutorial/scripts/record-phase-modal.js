#!/usr/bin/env node
// Records "double-click a node in building mode to open the phase-input modal, type an
// angle, and submit" - nodeInteractions.ts runs its own hand-rolled 400ms double-click timer
// on top of the "click" event (only engaged when buildingMode is on); Playwright's
// `.dblclick()` fires two native clicks well within that window. See README.md for
// setup/run instructions.

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
    name: 'phase-modal',
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

    await seedGraph(page, {
      size: 1,
      meas: { '0': { basis: 'XY', angle: 'π/2' } },
    });

    await page.reload();
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);

    await page.keyboard.press('b'); // enable building mode - double-click-to-open only works then
    await page.waitForTimeout(600);

    const node = page.locator('g.node[data-node-id="0"]');
    await moveCursorToLocator(page, node);
    await page.waitForTimeout(300);
    await node.dblclick();

    const input = page.getByPlaceholder('e.g. 1, pi/2, 3/2');
    await input.waitFor({ state: 'visible' });
    await page.waitForTimeout(600);

    // Type slowly so each character reads clearly instead of appearing all at once.
    await moveCursorToLocator(page, input);
    await input.click();
    await page.keyboard.type('3pi/4', { delay: 180 });
    await page.waitForTimeout(600);

    const submitButton = page.locator('button', { hasText: 'Submit' });
    await hoverWithCursor(page, submitButton);
    await page.waitForTimeout(1000);
    await submitButton.click();

    await page.waitForTimeout(400);
    await page.waitForSelector('text=3π/4');
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
