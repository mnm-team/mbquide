#!/usr/bin/env node
// Records "Simulator: double-click a ready node to measure it" - useSimulatorRendering.ts's
// dblclick handler only calls measureOperation(id) when readyToMeasure (from the backend's
// /api/sim response) includes that node's id; a single click just selects it. There's no CSS
// class or attribute marking "ready" nodes in the app - the only way to know which id is
// ready is to read it straight out of the API response, same as this script does. Seeds the
// simplest valid pattern (one input wired to one output), where the input node is ready
// immediately. See README.md for setup/run instructions.

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
    name: 'sim-measure-node',
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

    await seedGraph(page, {
      size: 2,
      inputs: [0],
      outputs: [1],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'OUTPUT', angle: '0' },
      },
      edges: [[0, 1]],
    });

    await postToBackend(page, 'graph', { flow: 'pauli' });
    await postToBackend(page, 'graph', { simulate: true, random: true, input: '' });

    await page.goto(`${FRONTEND_URL}/SIM`);
    await disableTextSelection(page);
    await showCursor(page);
    // Unlike the editor's graph (class="node"), the Simulator's node <g> elements carry no
    // class at all - only the data-node-id attribute (added alongside this script).
    await page.waitForSelector('g[data-node-id]');
    await page.waitForTimeout(800);
    await page.keyboard.press('c');
    await page.waitForTimeout(600);

    // The /api/graph {simulate:true} response doesn't itself carry readyToMeasure (that
    // field only exists in /api/sim's own response shape, SimData in Simulator_App.tsx) -
    // GET /api/sim directly, same as the Simulator page's own mount effect does.
    const simData = await page.evaluate(async () => {
      const res = await fetch('http://localhost:18080/api/sim', { credentials: 'include' });
      return res.json();
    });
    const readyNodeId = simData.readyToMeasure?.[0];
    if (readyNodeId === undefined) {
      throw new Error(`No node is ready to measure - readyToMeasure was ${JSON.stringify(simData.readyToMeasure)}`);
    }

    const readyNode = page.locator(`g[data-node-id="${readyNodeId}"]`);
    await hoverWithCursor(page, readyNode);
    await page.waitForTimeout(1000);
    await readyNode.dblclick();

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
