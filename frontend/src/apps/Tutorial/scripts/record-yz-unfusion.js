#!/usr/bin/env node
// Records the "YZ-unfuse a node, then drag the angle handle" tutorial clip by driving the
// real app with Playwright, instead of screen-capturing it by hand. See README.md in this
// folder for setup and run instructions, and for how to add another recording like this one.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { seedGraph } from './lib/graphApi.js';
import { hideChrome } from './lib/chrome.js';
import { svgPointToScreen, zoomGraph, lerp, waitForDisplayed } from './lib/svg.js';
import { showCursor, moveMouse, sweepMouse, moveCursorToLocator } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

// Tuned for this particular graph's node spacing (a bare 2-node pair, ~100 units apart -
// see getNodePosition() in positioning.ts). Adjust per-recording if the seeded graph differs.
const ZOOM_SCALE = 4;

// How long the page-load + first recenter/zoom setup takes before there's anything worth
// watching (see startRecording()'s own comment on trimStartSeconds) - measured by eyeballing
// an untrimmed run; a couple hundred ms of slack either way is fine.
const TRIM_START_SECONDS = 4;

async function assertServersUp() {
  const targets = [
    ['frontend', FRONTEND_URL],
    ['backend', 'http://localhost:18080/api/graph'],
  ];
  for (const [label, url] of targets) {
    try {
      await fetch(url);
    } catch {
      throw new Error(
        `Could not reach the ${label} at ${url}. Start both servers first - see README.md in this folder.`
      );
    }
  }
}

async function readLineAttrs(locator) {
  return locator.evaluate((el) => ({
    x1: +el.getAttribute('x1'),
    y1: +el.getAttribute('y1'),
    x2: +el.getAttribute('x2'),
    y2: +el.getAttribute('y2'),
  }));
}

/** Waits for the force simulation to settle, then recenters ("c") and zooms on it. */
async function settleRecenterAndZoom(page) {
  await page.waitForTimeout(1200);
  await page.keyboard.press('c');
  await page.waitForTimeout(400);
  await zoomGraph(page, ZOOM_SCALE);
}

async function main() {
  await assertServersUp();

  const { page, finish } = await startRecording({
    name: 'yz-unfusion',
    width: 600,
    height: 400,
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/MBQC`);
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('svg');

    // Start with just a single XY node - the unfused pendant gets created on screen via the
    // "YZ-Unfuse" context-menu action below, same as a real user would.
    await seedGraph(page, {
      size: 1,
      meas: { '0': { basis: 'XY', angle: 'π/2' } },
    });

    await page.reload();
    await hideChrome(page);
    await showCursor(page);
    await page.waitForSelector('g.node[data-node-id="0"]');
    await settleRecenterAndZoom(page);
    await page.waitForTimeout(2000); // let the lone node sit on screen for a beat first

    // Right-click opens the context menu (non-building-mode contextmenu handler in
    // nodeInteractions.ts); Playwright's button:'right' click dispatches a real right mouse
    // press/release, which Chromium turns into a native 'contextmenu' event same as a user.
    const node0Locator = page.locator('g.node[data-node-id="0"]');
    await moveCursorToLocator(page, node0Locator);
    await page.waitForTimeout(300);
    await node0Locator.click({ button: 'right' });
    const yzUnfuseButton = page.locator('button', { hasText: 'YZ-Unfuse' });
    await yzUnfuseButton.waitFor({ state: 'visible' });
    await page.waitForTimeout(500); // let the menu register before anything else happens

    // Hover before clicking so it's visually clear which button is about to be pressed,
    // rather than the click just happening.
    await moveCursorToLocator(page, yzUnfuseButton);
    await yzUnfuseButton.hover();
    await page.waitForTimeout(1200);

    await yzUnfuseButton.click();
    await page.waitForSelector('g.node[data-node-id="1"]');
    await settleRecenterAndZoom(page);

    // The unfuse operation clears selection (see runGraphOperation() in useGraphApi.ts), so
    // the handle isn't shown yet - reselect the XY node to reveal it (handles are hidden
    // unless one of the pair's two nodes is selected, see updateUnfusionHandleVisibility()).
    // Note: this waits on computed `display`, not Playwright's built-in `state: 'visible'` -
    // the knob is a thin SVG <line> that ends up perfectly horizontal for this seeded graph
    // (both nodes share an x-coordinate), and `state: 'visible'` treats its zero-height
    // bounding box as not visible even though it's plainly on screen. See waitForDisplayed().
    await moveCursorToLocator(page, node0Locator);
    await node0Locator.click();
    await waitForDisplayed(page, 'line.unfusion-knob');
    await page.waitForTimeout(500);

    // The "wire" is the full travel path between the two nodes; the "knob" is the short
    // draggable bar currently sitting somewhere along it. Read both directly off the SVG.
    const knobAttrs = await readLineAttrs(page.locator('line.unfusion-knob'));
    const wireAttrs = await readLineAttrs(page.locator('line.unfusion-wire'));

    const knobCenterLocal = {
      x: (knobAttrs.x1 + knobAttrs.x2) / 2,
      y: (knobAttrs.y1 + knobAttrs.y2) / 2,
    };

    const toScreen = (t) =>
      svgPointToScreen(
        page,
        'line.unfusion-wire',
        lerp(wireAttrs.x1, wireAttrs.x2, t),
        lerp(wireAttrs.y1, wireAttrs.y2, t)
      );

    const knobScreen = await svgPointToScreen(page, 'line.unfusion-knob', knobCenterLocal.x, knobCenterLocal.y);
    // Stay a little inside each end so the knob doesn't visually overlap the node shapes.
    const nearStart = await toScreen(0.08);
    const nearEnd = await toScreen(0.92);
    const middle = await toScreen(0.5);

    // Slower than a natural drag (40ms/step instead of a snappier 25ms, with longer pauses
    // between legs) so the angle change reads clearly instead of blurring past.
    await moveMouse(page, knobScreen.x, knobScreen.y);
    await page.waitForTimeout(400);
    await page.mouse.down();
    await sweepMouse(page, knobScreen, nearEnd, 20, 40);
    await page.waitForTimeout(400);
    await sweepMouse(page, nearEnd, nearStart, 30, 40);
    await page.waitForTimeout(400);
    await sweepMouse(page, nearStart, middle, 16, 40);
    await page.mouse.up();
    await page.waitForTimeout(800);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
