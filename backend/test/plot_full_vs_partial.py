#!/usr/bin/env python3
"""Plots results from the CSV written by
`Benchmarks --test-case="Benchmark: Random Clifford - Conveyor Belt Comparison"`,
reproducing Figure 8 of the paper: simulation time vs. pattern size for the
dense backend's full ("standard") vs. partial ("conveyor belt") qubit
initialization strategy.

Usage:
    python_venv/bin/python backend/test/plot_full_vs_partial.py \
        [--input backend/test/results/full_vs_partial.csv] \
        [--output paper/img/plot_simulation_time.pdf]
"""
import argparse
import os
import sys

import matplotlib
matplotlib.use("Agg")
import pandas as pd

from plot_style import new_figure, style_axes, scatter_pair


def summarize(df):
    rows = []
    for depth, g in df.groupby("depth"):
        full = g.loc[g["full_ok"] == 1]
        partial = g.loc[g["partial_ok"] == 1]
        rows.append({
            "depth": depth,
            "nodes_after": g["nodes_after"].mean(),
            "full_s": full["full_us"].mean() / 1e6 if len(full) else None,
            "partial_s": partial["partial_us"].mean() / 1e6 if len(partial) else None,
            "full_fail_rate": 1 - len(full) / len(g),
        })
    return pd.DataFrame(rows).sort_values("nodes_after")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", default="backend/test/results/full_vs_partial.csv")
    parser.add_argument("--output", default="paper/img/plot_simulation_time.pdf")
    args = parser.parse_args()

    df = pd.read_csv(args.input)
    if df.empty:
        sys.exit(f"{args.input} is empty, nothing to plot")

    summary = summarize(df)

    if (summary["full_fail_rate"] > 0).any():
        print("Note: some 'standard' (full init) repetitions failed and were excluded from the average:")
        failed = summary[summary["full_fail_rate"] > 0]
        print(failed[["depth", "nodes_after", "full_fail_rate"]].to_string(index=False))

    fig, ax = new_figure()
    full_plot = summary.dropna(subset=["full_s"])
    partial_plot = summary.dropna(subset=["partial_s"])
    scatter_pair(ax, full_plot["nodes_after"], full_plot["full_s"], "Full Initialization",
                 partial_plot["nodes_after"], partial_plot["partial_s"], "Dynamic Initialization")
    ax.set_xlabel("# Vertices")
    ax.set_ylabel("Simulation Time (s)")
    style_axes(ax)
    fig.tight_layout()

    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    fig.savefig(args.output)
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
