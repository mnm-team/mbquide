#!/usr/bin/env node
// Records "building mode: drag an example node from the palette onto the canvas" - the
// palette is always rendered at the SVG's fixed right edge (see renderExamples.ts /
// EXAMPLE_CONFIG), but only draggable once building mode is on (exampleDrag.ts is only
// attached `if (buildingMode)` in useGraphSimulation.ts). See README.md for setup/run info.
//
// Framing: the palette is tiny and far to one side at the app's natural (unzoomed) scale, so
// this zooms in anchored on the palette itself (not the usual centered zoom, since the
// palette isn't part of the pannable/recenterable graph content) and only drags the node a
// short distance to its immediate left, rather than all the way across the frame.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { svgPointToScreen, zoomGraph } from './lib/svg.js';
import { disableTextSelection } from './lib/chrome.js';
import { showCursor, moveMouse, sweepMouse } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const TRIM_START_SECONDS = 2.7; // measured from an untrimmed run: page load + setup + hover

// EXAMPLE_CONFIG (constants.ts): X_OFFSET=1850, Y_OFFSET=200, Y_DISTANCE=70, 8 icons -
// palette spans graph y=200 (X) to y=690 (OUTPUT), all at graph x=1850.
const PALETTE_X = 1850;
const PALETTE_CENTER_Y = 445;
const ZOOM_SCALE = 1.8; // tuned so the palette's ~500-unit vertical span nearly fills the frame

// A CSS transform-origin anchor point stays fixed on screen as scale increases; everything
// else moves away from it in proportion to its distance from the anchor. Anchoring directly
// on the palette's own center (445) would keep the palette at its natural unzoomed screen
// position, which isn't the frame's vertical middle - that first attempt clipped the topmost
// icon. Solving screen(445) = viewportCenter for the anchor's graph-space Y instead (with
// screen(P) = unzoomedScale * (origin*(1-scale) + P*scale), where the video's 16:9 ratio
// makes unzoomedScale cancel out of "screen(P) = viewport center") gives this value for
// ZOOM_SCALE=1.8 - it puts the palette's center at the frame's vertical middle with
// symmetric margins top and bottom, regardless of the video's actual pixel size.
const ZOOM_ORIGIN_Y = 326.25;
// Anchor X just past the viewBox's own right edge (x=1920, beyond the palette itself) so
// scaling up pulls the palette away from the frame's right edge rather than off of it (see
// zoomGraph()'s own note on anchoring near an edge).
const ZOOM_ORIGIN = `${(1920 / 1920) * 100}% ${(ZOOM_ORIGIN_Y / 1080) * 100}%`;

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

// Unlike hideChrome() (which hides everything data-tutorial-hide, including the palette),
// this recording needs the palette visible as the drag source - only hide the control panel
// and the building-mode toggle switch.
async function hidePanelAndToggle(page) {
  await page.addStyleTag({
    content:
      '[data-tutorial-hide="control-panel"], [data-tutorial-hide="building-mode-toggle"] { display: none !important; }',
  });
  await disableTextSelection(page);
  await showCursor(page);
}

async function main() {
  await assertServersUp();

  const { page, finish } = await startRecording({
    name: 'building-drag-node',
    width: 800,
    height: 450, // 16:9 - see README's "SVG letterboxing" note
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hidePanelAndToggle(page);
    await page.waitForSelector('svg');

    await seedGraph(page, { size: 0 });

    await page.reload();
    await hidePanelAndToggle(page);
    await page.waitForSelector('g.example-10004'); // the XY palette icon
    await page.waitForTimeout(800);

    // Enable building mode via its keyboard shortcut - this re-runs the d3 setup effect in
    // useGraphSimulation.ts and attaches the drag behavior to the palette nodes.
    await page.keyboard.press('b');
    await page.waitForTimeout(600);

    await zoomGraph(page, ZOOM_SCALE, 'svg', ZOOM_ORIGIN);
    await page.waitForTimeout(600);

    const xyIconScreen = await svgPointToScreen(page, 'g.example-10004', 0, 0);
    // A short drag to the icon's immediate left, not across the whole canvas - empty canvas,
    // no pan offset yet, so a point in the root <svg>'s own coordinate space (pre-viewBox-
    // scale user units, i.e. graph space) maps straight to screen space here.
    const dropTarget = await svgPointToScreen(page, 'svg', PALETTE_X - 340, 410);

    await moveMouse(page, xyIconScreen.x, xyIconScreen.y);
    await page.waitForTimeout(1000); // hover so it's clear which palette icon is about to move
    await page.mouse.down();
    await sweepMouse(page, xyIconScreen, dropTarget, 22, 40);
    await page.waitForTimeout(400);
    await page.mouse.up();

    await page.waitForSelector('g.node[data-node-id="0"]');
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
