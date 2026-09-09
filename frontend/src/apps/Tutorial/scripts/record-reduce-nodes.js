#!/usr/bin/env node
// Records "Reduce Nodes" (Simplify) - click to run the backend's iterative simplification
// loop over the whole graph, no selection needed. Backend: MBQC_Graph::simplify
// (MBQC_Graph.cpp) repeats, until no more apply or a cycle/iteration cap is hit: relabel any
// planar (XY/XZ/YZ) non-output node whose angle is a multiple of pi/2 to a Pauli basis,
// local-complement every Y-basis non-input/non-output node, and pivot every X-basis
// non-input node against a non-input neighbor. Seeds a Y-basis hub with two XY leaves, which
// canSimplify() (useGraphValidation.ts) flags as simplifiable via the Y-node LC rule. See
// README.md for setup/run instructions.

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
    name: 'reduce-nodes',
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

    // Y-basis hub (node 0, LC-able) with two XY leaves - node 1 is the input purely for
    // layout spread.
    await seedGraph(page, {
      size: 3,
      inputs: [1],
      meas: {
        '0': { basis: 'Y', angle: '0' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'XY', angle: 'π/2' },
      },
      edges: [[0, 1], [0, 2]],
    });

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1500);

    const reduceNodesButton = page.locator('button', { hasText: 'Automatically simplify the Graph' });
    await hoverWithCursor(page, reduceNodesButton);
    await page.waitForTimeout(1000);
    await reduceNodesButton.click();

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
