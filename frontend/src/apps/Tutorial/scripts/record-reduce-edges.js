#!/usr/bin/env node
// Records "Reduce Edges" (optimize) - click to greedily reduce the graph's edge count via
// alternating LC/Pivot rewrites. Backend: MBQC_Graph::greedyOptimizeEdges (MBQC_Graph.cpp,
// explicitly commented as mirroring pyzx's pattern_optimize.py) searches for LC/Pivot
// sequences that reduce total edge count. The button is enabled only once an async check
// (useCanOptimizeEdges.ts, polling checkCanOptimizeEdges) confirms a reducing step exists.
// Seeds a 4-cycle. See README.md for setup/run instructions.

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

const ZOOM_SCALE = 1.6;
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
    name: 'reduce-edges',
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

    // A 4-cycle, spread into a diamond via a single input.
    await seedGraph(page, {
      size: 4,
      inputs: [0],
      meas: {
        '0': { basis: 'XY', angle: 'π/2' },
        '1': { basis: 'XY', angle: 'π/2' },
        '2': { basis: 'XY', angle: 'π/2' },
        '3': { basis: 'XY', angle: 'π/2' },
      },
      edges: [[0, 1], [1, 2], [2, 3], [3, 0]],
    });

    await page.reload();
    await hideChromeExceptPanel(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(1000);

    const reduceEdgesButton = page.locator('button', { hasText: 'Greedily reduce edges' });
    // useCanOptimizeEdges.ts checks asynchronously against the backend - wait for it to
    // actually enable rather than assuming it's ready immediately.
    await page.waitForFunction(
      () => {
        const buttons = Array.from(document.querySelectorAll('button'));
        const btn = buttons.find(b => b.textContent?.includes('Greedily reduce edges'));
        return btn && !btn.disabled;
      },
      { timeout: 15000 }
    );

    await hoverWithCursor(page, reduceEdgesButton);
    await page.waitForTimeout(1000);
    await reduceEdgesButton.click();

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
