/**
 * Playwright drives the mouse via raw CDP input events (Input.dispatchMouseEvent) - there is
 * no actual OS- or browser-rendered cursor, so recordings show no visible pointer at all,
 * even during a drag. This fakes one: a small dot injected into the page that a script moves
 * itself, in lockstep with every real `page.mouse.move()`/`page.mouse.down()` call.
 *
 * Usage: call `showCursor(page)` once after every navigation (goto/reload), same as
 * hideChrome()/disableTextSelection() - it doesn't persist across navigations on its own.
 * Then either call `moveCursor(page, x, y)` yourself right after each `page.mouse.move(...)`,
 * or just replace direct `page.mouse.move`/`mouseSweep` calls with `moveMouse`/`sweepMouse`
 * below, which do both in one call.
 */
export async function showCursor(page) {
  await page.evaluate(() => {
    if (document.getElementById('__tutorial_cursor')) return;
    const cursor = document.createElement('div');
    cursor.id = '__tutorial_cursor';
    cursor.style.cssText = [
      'position:fixed',
      'top:0',
      'left:0',
      'width:24px',
      'height:24px',
      // The arrow's tip (the actual pointer hotspot) sits at local (4,4) in the 24x24 SVG
      // below - offset by that so translate(x,y) lands the tip exactly on the coordinate,
      // not the icon's bounding-box corner.
      'margin:-4px 0 0 -4px',
      'pointer-events:none',
      'z-index:2147483647',
      'transform:translate(-9999px,-9999px)', // off-screen until first moveCursor() call
      'transition:transform 20ms linear', // smooths out mouseSweep()'s discrete steps
    ].join(';');
    // A standard arrow-pointer silhouette (tip at top-left), white fill with a dark outline
    // so it stays visible against both light and dark parts of the app. The inline style
    // attribute on the <svg> itself (not just the wrapping div) is load-bearing: Chromium
    // headless renders an unstyled inline <svg> with a default ~1px #ccc border and #f9f9f9
    // background (visible as a faint box around the icon once zoomed in) - overriding them
    // directly on the element is the only way found to suppress it.
    cursor.innerHTML =
      '<svg width="24" height="24" viewBox="0 0 24 24" ' +
      'style="border:0;background:none;outline:none;box-shadow:none;display:block;">' +
      '<path d="M4 4 L4 20 L8.5 16.3 L11.2 21.8 L13.8 20.5 L11.1 15 L16.5 15 Z" ' +
      'fill="white" stroke="black" stroke-width="1.3" stroke-linejoin="round"/>' +
      '</svg>';
    document.body.appendChild(cursor);
  });
}

/** Repositions the fake cursor injected by showCursor(). A no-op if showCursor() wasn't called. */
export async function moveCursor(page, x, y) {
  await page.evaluate(
    ({ x, y }) => {
      const cursor = document.getElementById('__tutorial_cursor');
      if (cursor) cursor.style.transform = `translate(${Math.round(x)}px, ${Math.round(y)}px)`;
    },
    { x, y }
  );
}

/** page.mouse.move(x, y), plus moving the fake cursor to match. */
export async function moveMouse(page, x, y) {
  await page.mouse.move(x, y);
  await moveCursor(page, x, y);
}

/**
 * Drop-in replacement for lib/svg.js's mouseSweep() that also drags the fake cursor along
 * for every intermediate step, not just the endpoints.
 */
export async function sweepMouse(page, from, to, steps = 20, stepDelayMs = 25) {
  for (let i = 1; i <= steps; i++) {
    const t = i / steps;
    const x = from.x + (to.x - from.x) * t;
    const y = from.y + (to.y - from.y) * t;
    await page.mouse.move(x, y);
    await moveCursor(page, x, y);
    await page.waitForTimeout(stepDelayMs);
  }
}

async function centerOf(locator) {
  const box = await locator.boundingBox();
  if (!box) throw new Error('cursor.js: locator has no bounding box (not visible?)');
  return { x: box.x + box.width / 2, y: box.y + box.height / 2 };
}

/**
 * Moves the fake cursor to a Playwright locator's center - for scripts that click/hover a
 * button or node via `locator.click()`/`.hover()`/`.dblclick()` rather than raw
 * `page.mouse` coordinates, where there's no (x, y) already at hand to pass to moveCursor().
 * Doesn't touch Playwright's own hover state or move the real synthetic pointer - call it
 * right before the actual `.hover()`/`.click()`/`.dblclick()`, or use `hoverWithCursor()`
 * below to do both in one call.
 */
export async function moveCursorToLocator(page, locator) {
  const { x, y } = await centerOf(locator);
  await moveCursor(page, x, y);
}

/** locator.hover(), plus moving the fake cursor to the same spot so the "hover before you
 * click" pacing several scripts already use is actually visible. */
export async function hoverWithCursor(page, locator) {
  const { x, y } = await centerOf(locator);
  await moveCursor(page, x, y);
  await locator.hover();
}
