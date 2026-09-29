/**
 * Converts a point given in an SVG element's own local coordinate space into page (screen)
 * coordinates, using the element's current screen CTM - so the result is correct regardless
 * of how the graph is currently panned/zoomed. Use this instead of assuming a fixed offset,
 * since MBQuIDE's graph canvas pans (see useSvgPan.ts) and its <svg> scales to its container
 * via viewBox (see Graph/index.tsx).
 */
export async function svgPointToScreen(page, selector, x, y) {
  return page.evaluate(
    ({ selector, x, y }) => {
      const el = document.querySelector(selector);
      if (!el) throw new Error(`svgPointToScreen: no element matches "${selector}"`);
      const svg = el.ownerSVGElement ?? el;
      const pt = svg.createSVGPoint();
      pt.x = x;
      pt.y = y;
      const screenPt = pt.matrixTransform(el.getScreenCTM());
      return { x: screenPt.x, y: screenPt.y };
    },
    { selector, x, y }
  );
}

/**
 * Zooms in on the graph canvas by CSS-scaling the <svg> element around `origin` (default its
 * own center), with a short transition so it reads as a deliberate zoom rather than a jump
 * cut. Purely visual (doesn't touch the app's own pan/viewBox state), so it's safe to call
 * from any recording script - combine with the "C" recenter shortcut first so the right spot
 * ends up under the zoom's center (when using the default center origin). Waits for the
 * transition to finish before returning, since `svgPointToScreen()` and Playwright's own
 * hit-testing both need the final CTM.
 *
 * `origin` lets you anchor the zoom on something other than the pannable graph content -
 * e.g. the always-fixed-position example palette, which isn't affected by recenter/pan at
 * all. Pass it as a CSS transform-origin value ("<x>% <y>%" of the svg's own box); a point
 * near an edge is fine (even slightly beyond 100%) to pull the anchored content further from
 * that edge as scale increases, since content on the far side of the origin from that edge
 * moves further away from the origin (and thus further from the edge) as scale grows.
 */
export async function zoomGraph(page, scale, selector = 'svg', origin = '50% 50%') {
  const TRANSITION_MS = 400;
  await page.evaluate(
    ({ selector, scale, transitionMs, origin }) => {
      const el = document.querySelector(selector);
      if (!el) throw new Error(`zoomGraph: no element matches "${selector}"`);
      el.style.transformOrigin = origin;
      el.style.transition = `transform ${transitionMs}ms ease-out`;
      el.style.transform = `scale(${scale})`;
    },
    { selector, scale, transitionMs: TRANSITION_MS, origin }
  );
  await page.waitForTimeout(TRANSITION_MS + 50);
}

/**
 * Waits until `selector` matches an element whose (and whose ancestors') computed `display`
 * is not `none` - i.e. it's toggled on, without relying on Playwright's built-in `state:
 * 'visible'` wait. That built-in check additionally requires a non-empty *bounding box*,
 * which fails for a perfectly horizontal or vertical thin SVG `<line>`: `getBoundingClientRect()`
 * on an SVG line is geometry-only (it ignores stroke-width), so an axis-aligned line has a
 * zero-width or zero-height box and Playwright reports it "hidden" even though it's plainly
 * on screen (this bit the YZ-unfusion knob, whose bar is exactly horizontal when the two
 * seeded nodes share an x-coordinate). Prefer this over `state: 'visible'` for any thin SVG
 * shape a recording targets.
 */
export async function waitForDisplayed(page, selector, timeoutMs = 30000) {
  await page.waitForFunction(
    (selector) => {
      const el = document.querySelector(selector);
      if (!el) return false;
      for (let cur = el; cur; cur = cur.parentElement) {
        if (getComputedStyle(cur).display === 'none') return false;
      }
      return true;
    },
    selector,
    { timeout: timeoutMs }
  );
}

export const lerp = (a, b, t) => a + (b - a) * t;

/** Smoothly moves the mouse from `from` to `to` (both {x,y} in screen coords) over `steps` ticks. */
export async function mouseSweep(page, from, to, steps = 20, stepDelayMs = 25) {
  for (let i = 1; i <= steps; i++) {
    const t = i / steps;
    await page.mouse.move(lerp(from.x, to.x, t), lerp(from.y, to.y, t));
    await page.waitForTimeout(stepDelayMs);
  }
}
