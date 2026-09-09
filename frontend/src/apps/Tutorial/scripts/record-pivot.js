#!/usr/bin/env node
// Records "Pivot" - select exactly two adjacent nodes and click Pivot in the control panel.
// Backend: MBQC_Graph::pivot(u, v) is literally LC(u); LC(v); LC(u) (MBQC_Graph.cpp) - the
// standard graph-state pivot-as-triple-local-complementation identity.
//
// Seeds two inputs (2, 3) both feeding into hub node 0, which also connects to leaf node 1.
// Pivoting about edge (0,1) works out (traced by hand against localComplementation()'s
// actual toggle rule) to move the hub role from node 0 to node 1: final edges are
// {0-1, 1-2, 1-3} instead of the original {2-0, 3-0, 0-1} - both inputs' connections hop
// from node 0 over to node 1. Node 2's diagonal edge moving from node 0 to node 1 is the
// clearest visible tell. (An earlier single-line 3-node version of this recording pivoted
// correctly too, but the newly-toggled edge was exactly collinear with the existing path,
// so it rendered on top of it and the "change" was invisible - this layout avoids that.)
// See README.md for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChromeExceptPanel } from './lib/chrome.js';
import { svgPointToScreen, zoomGraph } from './lib/svg.js';
import { showCursor, moveMouse, sweepMouse, hoverWithCursor } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const ZOOM_SCALE = 1.8;
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
    name: 'pivot',
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

    // Inputs 2 and 3 both feed into hub 0, which also connects to leaf 1. Pivot pair (0, 1)
    // is non-input, as required by isPivotable() (useGraphValidation.ts).
    await seedGraph(page, {
      size: 4,
      inputs: [2, 3],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'XY', angle: 'π/2' },
        '3': { basis: 'XY', angle: 'π/2' },
      },
      edges: [[2, 0], [3, 0], [0, 1]],
    });

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1000);

    // Rubber-band select nodes 0 and 1 (both on the same y, in the two rightmost layers) -
    // the two inputs sit far to the left (x=100), well clear of this box.
    const node0Screen = await svgPointToScreen(page, 'g.node[data-node-id="0"]', 0, 0);
    const node1Screen = await svgPointToScreen(page, 'g.node[data-node-id="1"]', 0, 0);

    const margin = 45;
    const selectStart = { x: node0Screen.x - margin, y: node0Screen.y - margin };
    const selectEnd = { x: node1Screen.x + margin, y: node1Screen.y + margin };

    await moveMouse(page, selectStart.x, selectStart.y);
    await page.waitForTimeout(500);
    await page.mouse.down();
    await sweepMouse(page, selectStart, selectEnd, 20, 35);
    await page.mouse.up();
    await page.waitForTimeout(800);

    const pivotButton = page.locator('button', { hasText: 'Pivot about an edge' });
    await hoverWithCursor(page, pivotButton);
    await page.waitForTimeout(1000);
    await pivotButton.click();

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
