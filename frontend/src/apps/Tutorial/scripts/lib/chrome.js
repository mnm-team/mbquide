/**
 * Hides every element the app marks with `data-tutorial-hide="..."` (the control panel,
 * the building-mode toggle, the example-node palette, ...), so a recording can focus on
 * just the graph canvas.
 *
 * Call this after EVERY navigation (`page.goto(...)` and again after any `page.reload()`)
 * - it does not persist across navigations on its own.
 *
 * This uses `page.addStyleTag()`, not `context.addInitScript()`. addInitScript looked like
 * the better fit (it runs before the app's own scripts, so in theory there's no flash of
 * the chrome before it's hidden) but in practice, in this app, DOM nodes and listeners it
 * creates don't survive through to the fully-loaded page - Vite/HMR content ends up
 * overwriting them. addStyleTag runs after the page has (at least started to) load, so
 * there can be a brief flash of the chrome before it's hidden, but it's the same brief
 * moment we're waiting out anyway (with `page.waitForTimeout` for the graph layout to
 * settle) before anything worth recording happens.
 */
export async function hideChrome(page) {
  await page.addStyleTag({ content: '[data-tutorial-hide] { display: none !important; }' });
  await disableTextSelection(page);
}

/**
 * Disables text selection on the graph SVG. A mouse drag whose path crosses a node label
 * (id/basis/phase text) triggers the browser's native text-selection highlight, same as
 * dragging over any other selectable text - it has nothing to do with the app's own
 * selection state and doesn't show up from a quick eyeball of the interaction itself, only
 * as a stray blue highlight box sitting on a label in the recorded frames. Scoped to the
 * svg (rather than the whole page) so it doesn't interfere with typing/selecting text in the
 * phase-input modal's own <input>, which lives outside it.
 *
 * hideChrome() already calls this. Call it directly only in a script that skips hideChrome()
 * in favor of its own partial-hide style tag (e.g. one that needs the example palette
 * visible) and still does mouse-drag sweeps - add it after that style tag the same way.
 *
 * Also hides the app's global "?" tutorial-link button (TutorialHelpButton.tsx) specifically
 * - unlike the rest of app chrome, that button isn't scoped to any one page, so it would
 * otherwise show up in every recording regardless of which of hideChrome()/
 * hideChromeExceptPanel()/a script's own partial-hide style tag is in play. Every recording
 * script calls this function, directly or indirectly, which is what makes a single rule here
 * enough to cover all of them without touching each script individually.
 */
export async function disableTextSelection(page) {
  await page.addStyleTag({
    content:
      'svg, svg * { user-select: none !important; -webkit-user-select: none !important; } ' +
      '[data-tutorial-hide="tutorial-help-button"] { display: none !important; }',
  });
}

/**
 * Like hideChrome(), but keeps the control panel visible instead of hiding it - for
 * recordings of control-panel buttons (LC, Pivot, Z-Insert, Z-Delete, Reduce Nodes/Edges,
 * Get Flow, ...) that need it on screen to click. Still hides the building-mode toggle and
 * example palette, neither of which is relevant outside building mode, and still disables
 * text selection (see disableTextSelection()). Call after every navigation, same as
 * hideChrome().
 */
export async function hideChromeExceptPanel(page) {
  await page.addStyleTag({
    content:
      '[data-tutorial-hide="building-mode-toggle"], [data-tutorial-hide="example-palette"] { display: none !important; }',
  });
  await disableTextSelection(page);
}
