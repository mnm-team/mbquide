#!/usr/bin/env node
// Records "QASM Input: loading an example circuit" - clicking an example button just calls
// setQasmInput(...) with a hardcoded string (QASMControls.tsx), instantly filling the
// textarea and (per the live-render recording) re-rendering the diagram - no backend call.
// See README.md for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { disableTextSelection } from './lib/chrome.js';
import { showCursor, hoverWithCursor } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const TRIM_START_SECONDS = 1.4;

async function main() {
  try {
    await fetch(FRONTEND_URL);
  } catch {
    throw new Error(`Could not reach the frontend at ${FRONTEND_URL}. Start it first.`);
  }

  const { page, finish } = await startRecording({
    name: 'qasm-example',
    width: 1200,
    height: 950,
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/QASM`);
    await disableTextSelection(page);
    await showCursor(page);

    const exampleButton = page.locator('button', { hasText: 'Single Qubit Unitary' });
    await exampleButton.waitFor({ state: 'visible' });
    await page.waitForTimeout(600);

    await hoverWithCursor(page, exampleButton);
    await page.waitForTimeout(1200);
    await exampleButton.click();

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
