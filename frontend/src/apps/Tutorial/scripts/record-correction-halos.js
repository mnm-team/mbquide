#!/usr/bin/env node
// Records "MBQC Editor: click a node to reveal its correction set" - Get Flow first (a plain
// click, not the double-click that opens the phase modal, sets selectedNodes), then a single
// click on the input node. useGraphSimulation.ts reads the clicked node's own
// `correctionSet`/`oddCorrectionSet` (populated from the computed flow's corrf/oddNcorrf, per
// node id - see getDepthOrderedNodes in apps/MBQC/utils/positioning.ts) to show an orange ring
// around every node that would need an X-correction because of this node's outcome, and a
// green ring for the odd-neighborhood Z-correction - gated on flowLayerLines being non-empty
// (flowValid), which Get Flow just populated. Same 3-node chain seed as the old Simulator
// version of this recording (record-sim-correction-halos.js) - the input node's correction
// set includes the middle node, instead of the trivial single-edge case. Same control-panel
// visibility/zoom pattern as record-get-flow.js. See README.md for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChromeExceptPanel } from './lib/chrome.js';
import { zoomGraph } from './lib/svg.js';
import { showCursor, hoverWithCursor } from './lib/cursor.js';

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
    name: 'correction-halos',
    width: 1100,
    height: 619, // 16:9, roomy enough that the control panel doesn't cover the graph
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  page.on('dialog', async (dialog) => {
    console.error(`Unexpected dialog: ${dialog.message()}`);
    await dialog.dismiss();
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // Same 3-node chain as the old Simulator recording: input 0 -> XY 1 -> output 2, so the
    // input's correction set includes the middle node rather than nothing (a direct
    // input-output edge would give the trivial, uninteresting case).
    await seedGraph(page, {
      size: 3,
      inputs: [0],
      outputs: [2],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'OUTPUT', angle: '0' },
      },
      edges: [[0, 1], [1, 2]],
    });

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1000);

    const flowButton = page.locator('button', { hasText: 'Get the Pauli Flow' });
    await hoverWithCursor(page, flowButton);
    await page.waitForTimeout(1000);
    await flowButton.click();

    // Let the flow computation land, the layer lines draw, and the auto-recenter settle.
    await page.waitForTimeout(2200);

    // Click (not double-click, which would open the phase modal) the input node to select it
    // and reveal its correction set.
    const node0 = page.locator('g.node[data-node-id="0"]');
    await hoverWithCursor(page, node0);
    await page.waitForTimeout(600);
    await node0.click();

    await page.waitForTimeout(2500);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
