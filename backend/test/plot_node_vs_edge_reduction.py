#!/usr/bin/env python3
"""Plots results from the CSV written by
`Benchmarks --test-case="Benchmark: simplify vs greedyOptimizeEdges"`,
comparing the Reduce Nodes (simplify()) and Reduce Edges
(greedyOptimizeEdges()) pipelines.

Rather than one combined-time-vs-size scatter (which conflates reduction
cost and simulation cost into a single number, and looks like every other
scatter plot in the paper), this produces a grouped bar chart breaking
reduction time and simulation time apart for both methods, pooled into a
few pattern-size bins (small/medium/large) rather than picked from single
grid points. Pooling several adjacent sizes' repetitions together (tens of
samples per bar, instead of just the ~8 repetitions of one grid point)
makes each bar much less sensitive to any single unusually
(dis)entangled random circuit - error bars (standard error of the mean)
show what's left of that run-to-run variance after pooling.

Usage:
    python_venv/bin/python backend/test/plot_node_vs_edge_reduction.py \
        [--input backend/test/results/node_vs_edge_reduction.csv] \
        [--output paper/img/plot_node_vs_edge_reduction.pdf] \
        [--bins 3]
"""
import argparse
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from plot_style import FIGSIZE, COLOR_A, COLOR_B


def plot_breakdown(df, output, n_bins):
    sizes = (df.groupby(["nq", "depth"])["nodes_before"].mean()
             .reset_index().sort_values("nodes_before").reset_index(drop=True))
    # Which bin each distinct (nq, depth) grid point falls into, by rank
    # among sizes ordered small to large.
    sizes["bin"] = np.concatenate([np.full(len(idx), b) for b, idx in
                                    enumerate(np.array_split(np.arange(len(sizes)), n_bins))])
    df = df.merge(sizes[["nq", "depth", "bin"]], on=["nq", "depth"])

    labels, groups = [], []
    means = {"rn_reduce": [], "rn_sim": [], "re_reduce": [], "re_sim": []}
    errs = {"rn_reduce": [], "rn_sim": [], "re_reduce": [], "re_sim": []}

    for b in range(n_bins):
        g = df[df["bin"] == b]
        groups.append(g)

        lo, hi = g["nodes_before"].min(), g["nodes_before"].max()
        labels.append(f"{lo:.0f}-{hi:.0f} vertices" if lo != hi else f"~{lo:.0f} vertices")

        cols = {
            "rn_reduce": g["simp_us"] / 1e6,
            "rn_sim": g["simp_sim_us"] / 1e6,
            "re_reduce": g["edge_us"] / 1e6,
            "re_sim": g["edge_sim_us"] / 1e6,
        }
        for key, series in cols.items():
            means[key].append(series.mean())
            errs[key].append(series.std(ddof=1) / np.sqrt(len(series)) if len(series) > 1 else 0.0)

    x = np.arange(len(labels))
    width = 0.19

    fig, ax = plt.subplots(figsize=FIGSIZE)
    bar_specs = [
        ("rn_reduce", x - 1.5 * width, COLOR_A, 1.0, None, "Reduce Nodes: reduction"),
        ("rn_sim", x - 0.5 * width, COLOR_A, 0.5, "//", "Reduce Nodes: simulation"),
        ("re_reduce", x + 0.5 * width, COLOR_B, 1.0, None, "Reduce Edges: reduction"),
        ("re_sim", x + 1.5 * width, COLOR_B, 0.5, "//", "Reduce Edges: simulation"),
    ]
    for key, pos, color, alpha, hatch, label in bar_specs:
        ax.bar(pos, means[key], width, yerr=errs[key], capsize=3,
               color=color, alpha=alpha, hatch=hatch, label=label,
               error_kw={"alpha": 0.7, "linewidth": 1})

    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_ylabel("Time (s)")
    ax.set_yscale("log")
    ax.grid(True, which="major", axis="y", linestyle="-", linewidth=0.6, alpha=0.5)
    ax.set_axisbelow(True)
    ax.legend(loc="upper left", fontsize=9)

    fig.tight_layout()
    fig.savefig(output)
    print(f"Wrote {output}")

    for label, g in zip(labels, groups):
        n_points = g[["nq", "depth"]].drop_duplicates().shape[0]
        print(f"  {label}: pooled from {n_points} grid point(s), {len(g)} repetitions")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", default="backend/test/results/node_vs_edge_reduction.csv")
    parser.add_argument("--output", default="paper/img/plot_node_vs_edge_reduction.pdf")
    parser.add_argument("--bins", type=int, default=3,
                         help="Number of pattern-size bins to pool into (default 3: small/medium/large).")
    args = parser.parse_args()

    df = pd.read_csv(args.input)
    if df.empty:
        sys.exit(f"{args.input} is empty, nothing to plot")

    plot_breakdown(df, args.output, args.bins)


if __name__ == "__main__":
    main()
