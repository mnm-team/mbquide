import { chromium } from 'playwright';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const execFileAsync = promisify(execFile);

/**
 * Launches a headless Chromium context with video recording enabled.
 * Returns the page to drive plus a `finish()` call that closes the context
 * (which flushes the video) and copies the result into `mediaDir/<name>.webm`.
 *
 * Pass `trimStartSeconds` to cut that many seconds off the front of the output - useful for
 * skipping the page-load / initial-zoom setup at the start of a recording, which is dead time
 * for a viewer but has to happen on the same page (and therefore during the same recording)
 * as the interaction itself, since Playwright can't pause/resume a context's video recording
 * mid-session. Requires `ffmpeg` on PATH. There's no automatic way to know how long your own
 * setup takes - time it (e.g. from the script's console output, or just eyeball the untrimmed
 * clip once) and hardcode a value; a couple hundred ms of slack is fine either way.
 */
export async function startRecording({ name, width = 1000, height = 700, mediaDir, trimStartSeconds = 0 }) {
  const videoDir = fs.mkdtempSync(path.join(os.tmpdir(), 'mbquide-tutorial-'));
  const browser = await chromium.launch();
  const context = await browser.newContext({
    viewport: { width, height },
    recordVideo: { dir: videoDir, size: { width, height } },
  });

  const page = await context.newPage();

  async function finish() {
    await context.close(); // flushes the .webm to videoDir
    const recordedPath = await page.video().path();

    fs.mkdirSync(mediaDir, { recursive: true });
    const destPath = path.join(mediaDir, `${name}.webm`);

    if (trimStartSeconds > 0) {
      await trimVideoStart(recordedPath, destPath, trimStartSeconds);
    } else {
      fs.copyFileSync(recordedPath, destPath);
    }

    fs.rmSync(videoDir, { recursive: true, force: true });
    await browser.close();
    return destPath;
  }

  return { browser, context, page, finish };
}

async function trimVideoStart(inputPath, outputPath, trimStartSeconds) {
  try {
    // -ss after -i (rather than before) decodes from the start instead of seeking to the
    // nearest keyframe, so the cut lands at exactly trimStartSeconds - worth the extra encode
    // time for a clip this short.
    await execFileAsync('ffmpeg', [
      '-y',
      '-i', inputPath,
      '-ss', String(trimStartSeconds),
      '-c:v', 'libvpx',
      '-crf', '20',
      '-b:v', '0',
      '-an',
      outputPath,
    ]);
  } catch (err) {
    throw new Error(
      `Failed to trim the recording with ffmpeg - is it installed and on PATH? (\`ffmpeg -version\`)\n${err.message}`
    );
  }
}
