#!/usr/bin/env node
// Records "Local Complementation (LC)" - select exactly one non-input node and click LC in
// the control panel. Backend: MBQC_Graph::localComplementation (MBQC_Graph.cpp) toggles
// edges among the selected node's neighborhood and updates that node's + each neighbor's
// measurement basis/angle per the standard graph-state LC rule. Seeds a "fan" - one hub node
// with three otherwise-disconnected leaves - so LC on the hub visibly adds the missing edges
// between the leaves, turning the fan into a hub-plus-triangle. See README.md for setup/run
// instructions.

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

const ZOOM_SCALE = 2.0;
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
    name: 'local-complementation',
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

    // Hub (node 0) with three leaves (1, 2, 3) - no edges between the leaves yet. LC excludes
    // input nodes (isLCable() in useGraphValidation.ts), so node 1 - not the hub - is the
    // input, purely to give getNodePosition() (positioning.ts) something to BFS-layer from
    // for a spread-out layout; node 0 stays a plain, LC-able node.
    await seedGraph(page, {
      size: 4,
      inputs: [1],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'XY', angle: 'π/2' },
        '3': { basis: 'XY', angle: 'π/2' },
      },
      edges: [[0, 1], [0, 2], [0, 3]],
    });

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1000);

    const hubNode = page.locator('g.node[data-node-id="0"]');
    await moveCursorToLocator(page, hubNode);
    await page.waitForTimeout(300);
    await hubNode.click();
    await page.waitForTimeout(800);

    // Plain "LC" as a hasText filter also matches the "Reduce Edges" button, whose sublabel
    // is "Greedily reduce edges via LC & Pivot" - match on the LC button's own sublabel text
    // instead, which is unique.
    const lcButton = page.locator('button', { hasText: 'Local Complementation' });
    await hoverWithCursor(page, lcButton);
    await page.waitForTimeout(1000);
    await lcButton.click();

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
