#!/usr/bin/env python3
"""Plots results from the CSV written by
`Benchmarks --test-case="Benchmark: Statevector vs TensorNetwork backend"`.

Produces a single figure with two y-axes sharing one x-axis (# vertices):
simulation time on the left, peak memory footprint (number of complex
amplitudes actually held at once, converted to MB) on the right, for both
backends. The memory numbers are measured directly, not estimated, read
straight off TensorNetworkSimulator::getStoredAmplitudeCount().

Time and memory series use the same per-backend color (blue = statevector,
orange = tensor network) and marker shape (o = statevector, x/+ = tensor
network); memory series are drawn hollow/lighter so the two metrics stay
visually distinct even sharing one plot.

Usage:
    python_venv/bin/python backend/test/plot_sv_vs_tn.py \
        [--input backend/test/results/sv_vs_tn.csv] \
        [--output paper/img/plot_sv_vs_tn.pdf]
"""
import argparse
import os
import sys

import matplotlib
matplotlib.use("Agg")
import pandas as pd

from plot_style import new_figure, style_axes

BYTES_PER_AMPLITUDE = 16  # sizeof(std::complex<double>)

COLOR_SV = "tab:blue"
COLOR_TN = "tab:orange"


def summarize(df):
    grouped = df.groupby(["nq", "depth"])
    rows = []
    for (nq, depth), g in grouped:
        sv = g.loc[g["sv_ok"] == 1]
        tn = g.loc[g["tn_ok"] == 1]
        rows.append({
            "nq": nq,
            "depth": depth,
            "nodes_after": g["nodes_after"].mean(),
            "sv_s": sv["sv_us"].mean() / 1e6 if len(sv) else None,
            "tn_s": tn["tn_us"].mean() / 1e6 if len(tn) else None,
            "sv_peak_mb": sv["sv_peak_amplitudes"].mean() * BYTES_PER_AMPLITUDE / 1e6 if len(sv) else None,
            "tn_peak_mb": tn["tn_peak_amplitudes"].mean() * BYTES_PER_AMPLITUDE / 1e6 if len(tn) else None,
            "sv_fail_rate": 1 - len(sv) / len(g),
            "tn_fail_rate": 1 - len(tn) / len(g),
        })
    return pd.DataFrame(rows).sort_values("nodes_after")


def plot(summary, output):
    fig, ax_time = new_figure()
    ax_mem = ax_time.twinx()

    sv_t = summary.dropna(subset=["sv_s"])
    tn_t = summary.dropna(subset=["tn_s"])
    sv_m = summary.dropna(subset=["sv_peak_mb"])
    tn_m = summary.dropna(subset=["tn_peak_mb"])

    # Time: solid, filled markers on the left axis.
    h1 = ax_time.scatter(sv_t["nodes_after"], sv_t["sv_s"], marker="o",
                          color=COLOR_SV, label="Statevector (time)", zorder=4)
    h2 = ax_time.scatter(tn_t["nodes_after"], tn_t["tn_s"], marker="x",
                          color=COLOR_TN, label="Tensor Network (time)", zorder=4)

    # Memory: hollow/lighter markers on the right axis, same per-backend
    # color so the two metrics for one backend are still easy to pair up.
    h3 = ax_mem.scatter(sv_m["nodes_after"], sv_m["sv_peak_mb"], marker="o",
                         facecolors="none", edgecolors=COLOR_SV, alpha=0.6,
                         label="Statevector (memory)", zorder=3)
    h4 = ax_mem.scatter(tn_m["nodes_after"], tn_m["tn_peak_mb"], marker="+",
                         color=COLOR_TN, alpha=0.6,
                         label="Tensor Network (memory)", zorder=3)

    ax_time.set_xlabel("# Vertices")
    ax_time.set_ylabel("Simulation Time (s)")
    ax_mem.set_ylabel("Peak Memory (MB)")
    ax_mem.set_yscale("log")
    # Grid only from the time (primary) axis - a second grid from the twin
    # axis would just double up gridlines at different, confusing heights.
    style_axes(ax_time)
    ax_mem.grid(False)

    handles = [h1, h2, h3, h4]
    labels = [h.get_label() for h in handles]
    ax_time.legend(handles, labels, loc="upper right",
                   bbox_to_anchor=(0.995, 0.995), fontsize=8,
                   handletextpad=0.4, borderpad=0.5, labelspacing=0.35)

    fig.tight_layout()
    fig.savefig(output)
    print(f"Wrote {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", default="backend/test/results/sv_vs_tn.csv")
    parser.add_argument("--output", default="paper/img/plot_sv_vs_tn.pdf")
    args = parser.parse_args()

    df = pd.read_csv(args.input)
    if df.empty:
        sys.exit(f"{args.input} is empty, nothing to plot")

    summary = summarize(df)

    if (summary["sv_fail_rate"] > 0).any() or (summary["tn_fail_rate"] > 0).any():
        print("Note: some repetitions failed and were excluded from their backend's average:")
        failed = summary[(summary["sv_fail_rate"] > 0) | (summary["tn_fail_rate"] > 0)]
        print(failed[["nq", "depth", "nodes_after", "sv_fail_rate", "tn_fail_rate"]].to_string(index=False))

    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    plot(summary, args.output)


if __name__ == "__main__":
    main()
