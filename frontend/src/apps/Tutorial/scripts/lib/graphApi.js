const API_BASE = process.env.MBQUIDE_API_URL ?? 'http://localhost:18080/api';

/**
 * Writes a graph to the backend for the *current page's* session, by running the fetch
 * inside the page itself (so it shares the app's session cookie) rather than from Node -
 * a Node-side fetch would get its own, separate session and the app would never see it.
 *
 * Shape mirrors writeGraphToBackend() in src/apps/MBQC/api/graphApi.ts:
 *   size, inputs, outputs: as in the editor
 *   edges: [[sourceId, targetId], ...]
 *   meas: { "<nodeId>": { basis: "XY" | "YZ" | ..., angle: "<pretty or numeric angle string>" } }
 *   outAdj: output Pauli-adjustment map (usually {} unless seeding OUTPUT nodes)
 */
export async function seedGraph(page, { size, inputs = [], outputs = [], edges = [], meas = {}, outAdj = {} }) {
  const result = await page.evaluate(
    async ({ apiBase, payload }) => {
      try {
        const res = await fetch(`${apiBase}/graph`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload),
          credentials: 'include',
        });
        return { ok: res.ok, status: res.status };
      } catch (err) {
        return { ok: false, status: 0, error: String(err) };
      }
    },
    { apiBase: API_BASE, payload: { size, inputs, outputs, edges, meas, outAdj } }
  );

  if (!result.ok) {
    throw new Error(
      `Seeding the graph failed (HTTP ${result.status}${result.error ? `, ${result.error}` : ''}).\n` +
      `Is the backend running on ${API_BASE}?`
    );
  }
}

/**
 * POSTs an arbitrary operation to /api/graph or /api/sim from inside the page (so it shares
 * the page's session cookie - see seedGraph()'s own note on why this can't be a Node-side
 * fetch), and returns the parsed JSON response. Useful for driving the Simulator the same
 * way the real app does (Get Flow, then Simulate, then individual measure/reset/runAll
 * calls) and for reading the response directly - e.g. `readyToMeasure` to know which node id
 * is currently measurable, since there's no DOM attribute for that (see Simulator/index.tsx
 * in the app - no CSS class marks a "ready" node either).
 */
export async function postToBackend(page, endpoint, body) {
  const result = await page.evaluate(
    async ({ apiBase, endpoint, body }) => {
      try {
        const res = await fetch(`${apiBase}/${endpoint}`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(body),
          credentials: 'include',
        });
        const json = await res.json().catch(() => null);
        return { ok: res.ok, status: res.status, json };
      } catch (err) {
        return { ok: false, status: 0, error: String(err) };
      }
    },
    { apiBase: API_BASE, endpoint, body }
  );

  if (!result.ok) {
    throw new Error(
      `POST /api/${endpoint} failed (HTTP ${result.status}${result.error ? `, ${result.error}` : ''}).\n` +
      `Is the backend running on ${API_BASE}?`
    );
  }

  return result.json;
}
