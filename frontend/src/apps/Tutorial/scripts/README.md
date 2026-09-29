# Tutorial recording scripts

These generate the short looping clips shown on the `/TUTORIAL` page by driving the real app with [Playwright](https://playwright.dev) - seeding a graph via the backend API, then performing the actual mouse actions - and recording video as a side effect. Nothing here is hand screen-captured, so a clip can be regenerated any time the UI changes.

## One-time setup

From `frontend/`:

```bash
npm install -D playwright
npx playwright install chromium
```

## Running a recording

You need both servers running first, in two separate terminals, from the repo root:

```bash
# Terminal 1
./backend/build/Server

# Terminal 2
cd frontend && npm run dev
```

Then, from `frontend/`, run the script:

```bash
node src/apps/Tutorial/scripts/record-yz-unfusion.js
```

It launches a headless Chromium, drives the app, and overwrites `src/apps/Tutorial/media/yz-unfusion.webm`. Refresh the `/TUTORIAL` page in your browser (hard refresh if it was already open) to see the new clip.

If a server isn't reachable the script fails fast with a message telling you which one.

## Adding another recording

1. Copy `record-yz-unfusion.js` as a starting point (`record-<your-step>.js`).
2. Change the `seedGraph(...)` call to set up whatever starting graph the interaction needs
   (see `lib/graphApi.js` - it writes straight to the backend, matching the shape used by
   `src/apps/MBQC/api/graphApi.ts`).
3. Drive the interaction with `page.mouse` / `page.keyboard` / `page.locator(...).click()`,
   the same way `record-yz-unfusion.js` does. Call `hideChrome(page)` (`lib/chrome.js`) after
   every navigation. `lib/svg.js` has `svgPointToScreen()` for converting a point in the SVG's
   own coordinate space to screen coordinates (accounting for panning/zoom), `zoomGraph()` for
   zooming in, `mouseSweep()` for a smooth multi-step drag, and `waitForDisplayed()` for
   waiting on a thin SVG shape to appear (see the Notes below for why not
   `page.waitForSelector(..., { state: 'visible' })`).
4. Call `startRecording({ name, mediaDir: MEDIA_DIR })` / `finish()` from `lib/recorder.js`
   exactly as the example does - `name` controls the output filename.
5. Add an entry to `../data.ts` pointing at the new file under `../media/`, e.g.:

   ```ts
   import myStepVideo from './media/my-step.webm';
   // ...
   { id: 'my-step', page: 'MBQC Editor', title: '...', description: '...',
     media: { type: 'video', src: myStepVideo } },
   ```

For a step that's just a button click (no drag to demonstrate), a static screenshot is clearer use `page.screenshot({ path: ... })` after seeding and reaching the right state, and reference it with `media: { type: 'image', src: ... }` instead.

## Notes

- Each run gets a fresh backend session (new cookie), so runs don't interfere with each other or with whatever graph you have open in your own browser tab.
- Node <g> elements carry a `data-node-id` attribute (added in
  `Graph/hooks/useGraphSimulation.ts`) specifically so these scripts can target a specific node reliably instead of relying on screen position.
- `hideChrome(page)` from `lib/chrome.js` hides everything marked `data-tutorial-hide="..."`
  in the app - currently the control panel, the building-mode toggle, and the example-node
  palette. Call it after **every** navigation (`page.goto(...)` and again after any
  `page.reload()`) - it doesn't persist across navigations on its own. It's implemented with
  `page.addStyleTag()`, not `context.addInitScript()`: addInitScript looks like the better
  fit (runs before the app's own scripts, so in theory no flash of the chrome first) but in
  this app its injected DOM nodes don't survive through to the fully-loaded page - Vite/HMR
  ends up discarding them. addStyleTag runs after the page starts loading instead, so there
  can be a brief flash, but it actually works, and the flash is invisible in practice since it
  happens well before the `page.waitForTimeout` calls that wait for the graph to settle. To
  hide something else too, just add `data-tutorial-hide="whatever"` to it in the app.
- `zoomGraph(page, scale)` from `lib/svg.js` CSS-zooms the graph canvas in around its own
  center - call it after pressing "c" to recenter, so the recentered point is what gets
  zoomed into. It's purely visual (doesn't touch the app's pan state) and waits out its own
  transition before returning, so coordinates read afterwards (e.g. via `svgPointToScreen()`)
  are already correct for the zoomed view.
- Prefer `waitForDisplayed(page, selector)` (`lib/svg.js`) over Playwright's built-in
  `page.waitForSelector(selector, { state: 'visible' })` when waiting on a thin SVG shape
  (a line, a thin rect). `state: 'visible'` additionally requires a non-empty bounding box,
  and `getBoundingClientRect()` on an SVG `<line>` is geometry-only - it ignores stroke-width -
  so a perfectly horizontal or vertical line has a zero-height or zero-width box and gets
  reported "hidden" even though it's clearly on screen. This is exactly what happened with the
  YZ-unfusion knob: with the two seeded nodes sharing an x-coordinate, its bar renders exactly
  horizontal. `waitForDisplayed()` only checks computed `display` up the ancestor chain, so it
  doesn't have this problem.
- **No visible cursor by default:** Playwright drives the mouse via raw CDP input events
  (`Input.dispatchMouseEvent`), not an actual OS/browser-rendered pointer, so a recording
  shows no cursor at all even mid-drag - a hover or drag can be hard to read without one.
  `lib/cursor.js` fakes one: `showCursor(page)` injects a small dot into the page (call it
  after every navigation, same as `hideChrome`/`disableTextSelection`), and `moveMouse(page,
  x, y)` / `sweepMouse(page, from, to, steps, stepDelayMs)` are drop-in replacements for
  `page.mouse.move(x, y)` / `lib/svg.js`'s `mouseSweep()` that also reposition it. Opt-in per
  script (not every recording needs it) - see `record-recenter.js` for a worked example.
- **Stray text-selection highlight:** a mouse drag whose path crosses a node label (id,
  basis, or phase text) can trigger the browser's native text-selection highlight on that
  label - same as dragging over any other selectable text on a page, nothing to do with the
  app's own node selection. It's easy to miss because it doesn't show up as an obviously
  "wrong" interaction, only as a stray blue box sitting on a label when you sample frames
  (first seen on a Ctrl-drag pan whose path happened to cross a column of stacked node
  labels). `hideChrome(page)` (`lib/chrome.js`) now also disables text selection on the svg,
  so scripts that call it are covered automatically; a script that skips it in favor of its
  own partial-hide style tag (e.g. one that needs the example palette visible, so it can't
  use hideChrome's blanket `data-tutorial-hide` rule) should call the exported
  `disableTextSelection(page)` itself, right after its own style tag.
- **SVG letterboxing:** the MBQC/ZX graph `<svg>` has `viewBox="0 0 1920 1080"` (16:9) with
  `width="100%" height="100%"`, but Chromium sizes the element itself to fit that aspect
  ratio within its container rather than stretching to fill it - at a 4:3 recording size
  (e.g. 600x400) this leaves a real gap (confirmed via `getBoundingClientRect()`: a 600-wide
  box comes out ~338px tall, not 400) below/around the svg's own box, showing the app's
  near-black page background (`bg-[#111]` in `apps/MBQC/index.tsx`) through the gap. A large
  enough `zoomGraph()` scale visually covers it (the transform grows outward from screen
  center past the viewport edges, which is what hid this in `record-yz-unfusion.js`'s 4x
  zoom), but that's incidental - don't rely on it. Instead size the recording to the same
  16:9 ratio, e.g. `{ width: 600, height: 338 }` in `startRecording(...)`, so there's no gap
  regardless of zoom level.
- Output is `.webm` (Playwright's native format). If Safari support ever matters, transcode to `.mp4` (e.g. `ffmpeg -i in.webm -c:v libx264 -pix_fmt yuv420p out.mp4`) and reference that file instead - the `<video>` tag in `TutorialCard` just plays whatever `src` it's given.
