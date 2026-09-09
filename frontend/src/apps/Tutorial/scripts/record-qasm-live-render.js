#!/usr/bin/env node
// Records "QASM Input: typing QASM and watching the circuit diagram render live" -
// QuantumCircuitViewer.tsx parses and lays out the circuit entirely client-side inside a
// useMemo keyed on qasmInput, with no debounce and no backend call, so every keystroke
// re-renders the diagram instantly. See README.md for setup/run instructions.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startRecording } from './lib/recorder.js';
import { disableTextSelection } from './lib/chrome.js';
import { showCursor, moveCursorToLocator } from './lib/cursor.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MEDIA_DIR = path.join(__dirname, '..', 'media');
const FRONTEND_URL = process.env.MBQUIDE_FRONTEND_URL ?? 'http://localhost:5173';

const TRIM_START_SECONDS = 1.4;

const QASM = `OPENQASM 2.0;
include "qelib1.inc";
qreg q[2];
h q[0];
cx q[0],q[1];`;

async function main() {
  try {
    await fetch(FRONTEND_URL);
  } catch {
    throw new Error(`Could not reach the frontend at ${FRONTEND_URL}. Start it first.`);
  }

  const { page, finish } = await startRecording({
    name: 'qasm-live-render',
    width: 1200,
    height: 950,
    mediaDir: MEDIA_DIR,
    trimStartSeconds: TRIM_START_SECONDS,
  });

  try {
    await page.goto(`${FRONTEND_URL}/QASM`);
    await disableTextSelection(page);
    await showCursor(page);

    const textarea = page.getByPlaceholder('paste your QASM here...');
    await textarea.waitFor({ state: 'visible' });
    await page.waitForTimeout(600);

    await moveCursorToLocator(page, textarea);
    await textarea.click();
    await page.keyboard.type(QASM, { delay: 35 });

    await page.waitForTimeout(2000);
  } finally {
    const outPath = await finish();
    console.log(`Saved recording to ${outPath}`);
  }
}

main().catch((err) => {
  console.error(err.message ?? err);
  process.exit(1);
});
