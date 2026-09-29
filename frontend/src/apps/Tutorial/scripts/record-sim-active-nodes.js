#!/usr/bin/env node
// Records "Simulator: which qubits are live" - the Simulator doesn't initialize the whole
// resource state at once: only nodes/edges within reach of the next measurement are actually
// live. A live node renders in its full basis color and a live edge renders black; a node
// not yet brought online renders faded (translucent) and its edges render grey, until an
// earlier measurement brings them into reach. Verified directly against a running instance's
// /api/sim response (not just eyeballed): on a 4-node chain 0->1->2->3, after init only nodes
// [0,1] and edge [0,1] are live; after measuring 0, [1,2] and edges [[0,1],[1,2]] are live;
// after measuring 1, [2,3] and every edge are live. A measured node instead renders flat grey,
// regardless of the live/not-yet-live distinction. See README.md for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph, postToBackend } from './lib/graphApi.js';
import { disableTextSelection } from './lib/chrome.js';
import { showCursor, hoverWithCursor } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const TRIM_START_SECONDS = 2.2;

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
    name: 'sim-active-nodes',
    width: 1100,
    height: 650,
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await disableTextSelection(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // 4-node chain, input 0 -> 1 -> 2 -> output 3, so there's a clearly-not-yet-live tail
    // (nodes 2, 3 and edges [1,2], [2,3]) visible right after init, and two separate
    // measurements to watch the live window advance one node at a time.
    await seedGraph(page, {
      size: 4,
      inputs: [0],
      outputs: [3],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'XY', angle: 'π/2' },
        '3': { basis: 'OUTPUT', angle: '0' },
      },
      edges: [[0, 1], [1, 2], [2, 3]],
    });

    await postToBackend(page, 'graph', { flow: 'pauli' });
    await postToBackend(page, 'graph', { simulate: true, random: true, input: '' });

    await page.goto(`${FRONTEND_URL}/SIM`);
    await disableTextSelection(page);
    await showCursor(page);
    await page.waitForSelector('g[data-node-id]');
    await page.waitForTimeout(800);
    await page.keyboard.press('c');
    await page.waitForTimeout(800);

    // Right after init: nodes 0, 1 and edge 0-1 are live (full color, black edge); nodes 2, 3
    // and edges 1-2, 2-3 are not yet live (faded, grey edges). Let this sit on screen a beat.
    await page.waitForTimeout(2200);

    // Double-click the ready node (0) to measure it - node 2 and edge 1-2 come online.
    const node0 = page.locator('g[data-node-id="0"]');
    await hoverWithCursor(page, node0);
    await page.waitForTimeout(500);
    await node0.dblclick();
    await page.waitForTimeout(2200);

    // Measure node 1 too - node 3 and edge 2-3 come online, so the whole chain is now live.
    const node1 = page.locator('g[data-node-id="1"]');
    await hoverWithCursor(page, node1);
    await page.waitForTimeout(500);
    await node1.dblclick();
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
