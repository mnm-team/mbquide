"""Shared matplotlib styling for the benchmark plots, so every figure in the
paper's evaluation section looks like it belongs to the same family as
Figure 8 (plot_simulation_time.pdf): same canvas size, same major-only grid
on both axes, same marker/color convention for a two-series comparison.
"""
import matplotlib.pyplot as plt

# matplotlib's own default (6.4in x 4.8in). Figure 8 was produced at this
# size; matching it keeps font/marker sizes visually consistent once every
# figure is scaled down to \linewidth in the two-column layout.
FIGSIZE = (6.4, 4.8)

# First series (e.g. the "baseline"/older approach) vs. second series (the
# new one being compared against it) - keep this assignment consistent
# across plots so readers don't have to re-learn the color coding.
COLOR_A = "tab:blue"
MARKER_A = "o"
COLOR_B = "tab:orange"
MARKER_B = "x"


def new_figure():
    return plt.subplots(figsize=FIGSIZE)


# Two panels stacked vertically in one figure, sharing the x-axis - used
# when two related metrics (e.g. time and memory) for the same comparison
# would otherwise need two separate, near-identical-looking figures.
STACKED_FIGSIZE = (6.4, 6.4)


def new_stacked_figure():
    fig, axes = plt.subplots(2, 1, figsize=STACKED_FIGSIZE, sharex=True)
    fig.subplots_adjust(hspace=0.3)
    return fig, axes


def style_axes(ax, log_y=True):
    """Applies the shared grid/legend/log-scale conventions to an axes."""
    if log_y:
        ax.set_yscale("log")
    # Major gridlines only, both axes - matches Figure 8, which does not
    # use minor gridlines.
    ax.grid(True, which="major", axis="both", linestyle="-", linewidth=0.6, alpha=0.5)
    ax.legend(loc="upper left")


def scatter_pair(ax, x_a, y_a, label_a, x_b, y_b, label_b):
    ax.scatter(x_a, y_a, marker=MARKER_A, color=COLOR_A, label=label_a, zorder=3)
    ax.scatter(x_b, y_b, marker=MARKER_B, color=COLOR_B, label=label_b, zorder=3)
