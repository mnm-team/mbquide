#!/usr/bin/env node
// Records "QASM Input: navigating to the MBQC diagram" - the "MBQC Diagram" action button
// POSTs the current QASM to /api/qasm (parsing it into a graph on the backend), then
// navigates to /MBQC. "Run Simulation" (not separately recorded - same idea, more steps) is
// the same POST followed by simplify, get-flow, and simulate operations chained before
// navigating to /SIM instead - see toSimulator() in QASMControls.tsx. See README.md for
// setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { hideChrome, disableTextSelection } from './lib/chrome.js';
import { zoomGraph } from './lib/svg.js';
import { showCursor, hoverWithCursor } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const TRIM_START_SECONDS = 1.4;

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
    name: 'qasm-to-mbqc',
    // 16:9 - this recording ends up on /MBQC, whose svg needs that ratio to avoid the "SVG
    // letterboxing" gap described in README.md (the QASM page itself has no such
    // requirement, but the video is one continuous recording across both pages).
    width: 1200,
    height: 675,
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/QASM`);
    await disableTextSelection(page);
    await showCursor(page);

    const exampleButton = page.locator('button', { hasText: 'Single Qubit Unitary' });
    await exampleButton.waitFor({ state: 'visible' });
    await hoverWithCursor(page, exampleButton);
    await page.waitForTimeout(400);
    await exampleButton.click();
    await page.waitForTimeout(600);

    const mbqcButton = page.locator('button', { hasText: 'Transform the Circuit to MBQC' });
    await hoverWithCursor(page, mbqcButton);
    await page.waitForTimeout(1000);
    await mbqcButton.click();

    await page.waitForURL('**/MBQC');
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await page.waitForTimeout(1200);
    await page.keyboard.press('c');
    await page.waitForTimeout(400);
    // This particular circuit expands into several MBQC nodes in a row - keep zoom modest
    // so the whole resulting chain stays in frame instead of overflowing the edges.
    await zoomGraph(page, 1.2);
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
