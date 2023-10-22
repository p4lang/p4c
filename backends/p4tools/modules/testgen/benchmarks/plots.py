#!/usr/bin/env python3

import argparse
import os
import re
import sys
from operator import attrgetter
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
TOOLS_PATH = FILE_DIR.joinpath("../../../../../tools")
sys.path.append(str(TOOLS_PATH))
import testutils

OUTPUT_DIR = Path("plots")

PARSER = argparse.ArgumentParser()

PARSER.add_argument(
    "-i",
    "--input-dir",
    dest="input_dir",
    help="The folder containing measurement data.",
)

PARSER.add_argument(
    "-o",
    "--out-dir",
    dest="out_dir",
    default=OUTPUT_DIR,
    help="The output folder where all plots are dumped.",
)


def plot_preconditions(args, extra_args):
    preconditions = [
        "None",
        "1500B pkt",
        "P4Constraints",
        "P4Constraints + 1500B pkt",
        "P4Constraints + 1500B IPv4 pkt",
        "P4Constraints + 1500B IPv4-TCP pkt",
    ]
    data = [146784, 83784, 74472, 42486, 28216, 7054]
    target_frame = pd.DataFrame({"Precondition": preconditions, "Number of tests": data})
    _, ax = plt.subplots(figsize=(4, 1.2))
    ax = sns.barplot(
        x="Number of tests",
        y="Precondition",
        data=target_frame,
        width=1,
        linewidth=0.5,
        palette=sns.color_palette("RdYlGn"),
        # facecolor=(0,0,0,0),
        edgecolor="0",
    )
    ax.set_xticks(np.arange(0, max(data) + 30000, 30000))
    hatches = ["/", ".", "//", "o", "x", "\\", "-", "+", "*", "O"]
    for idx, plot_bar in enumerate(ax.patches):
        # plot_bar.set_hatch(hatches[idx])
        ax.text(
            plot_bar.get_width() + 3000,
            plot_bar.get_y() + 0.22,
            preconditions[idx],
            ha="left",
            va="top",
        )
    ax.set(yticklabels=[], yticks=[])
    outdir = Path(args.out_dir).joinpath("precondition_reduction")
    plt.savefig(outdir.with_suffix(".png"), bbox_inches="tight")
    plt.savefig(outdir.with_suffix(".pdf"), bbox_inches="tight")
    plt.gcf().clear()


def get_strategy_data(input_dir):
    input_dir = Path(testutils.check_if_dir(input_dir))

    folders = input_dir.glob("*/")
    program_data = {}
    for folder in folders:
        if not os.path.isdir(folder):
            continue
        folder = Path(folder)
        program_name = folder.stem
        # Make sure we escape hyphens
        program_name = program_name.replace("-", "\-")
        # exit(0)
        strategy_files = folder.glob("*_coverage_over_time.csv")
        strategy_data = {}
        for strategy_file in strategy_files:
            m = re.search(f"{program_name}_(.+?)_coverage_over_time", strategy_file.stem)
            if m:
                strategy_name = m.group(1)
            else:
                print(f"Invalid strategy data file {strategy_file}.")
                sys.exit(1)
            df = pd.read_csv(strategy_file)
            df["Time"] = pd.to_timedelta(df["Time"], unit="milliseconds")
            strategy_data[strategy_name] = df
        program_data[program_name] = strategy_data
    return program_data


def plot_strategies(args, extra_args):
    program_datas = get_strategy_data(args.input_dir)
    for program_name, program_data in program_datas.items():
        pruned_data = []
        name_map = {
            "depth_first": "DFS",
            "random_backtrack": "Random",
            "greedy_statement_search": "Coverage-Optimized",
        }
        for strategy, candidate_data in program_data.items():
            candidate_data = candidate_data.drop("Seed", axis=1)
            candidate_data["Strategy"] = name_map[strategy]
            candidate_data.Time = pd.to_numeric(candidate_data.Time)
            candidate_data = candidate_data.sort_values(by=["Time"])
            bins = np.arange(
                0,
                candidate_data.Time.max() + candidate_data.Time.max() / 1000,
                candidate_data.Time.max() / 1000,
            )
            candidate_data["Minutes"] = pd.cut(
                candidate_data.Time, bins.astype(np.int64), include_lowest=True
            ).map(attrgetter("right"))
            candidate_data.Minutes = pd.to_timedelta(candidate_data.Minutes, unit="nanoseconds")
            candidate_data.Minutes = candidate_data.Minutes / pd.Timedelta(minutes=1)
            candidate_data = candidate_data[candidate_data.Minutes <= 60]
            pruned_data.append(candidate_data)
        concat_data = pd.concat(pruned_data)
        ax = sns.lineplot(
            x="Minutes", y="Coverage", hue="Strategy", data=concat_data, errorbar=None
        )
        sns.move_legend(ax, "lower center", bbox_to_anchor=(0.5, 0.98), ncol=3, title=None)
        outdir = Path(args.out_dir).joinpath(f"{program_name}_strategy_coverage")
        plt.savefig(outdir.with_suffix(".png"), bbox_inches="tight")
        plt.savefig(outdir.with_suffix(".pdf"), bbox_inches="tight")
        plt.gcf().clear()


def main(args, extra_args):
    sns.set_theme(
        context="paper",
        style="ticks",
        rc={
            "lines.linewidth": 1,
            "hatch.linewidth": 0.3,
            "axes.spines.right": False,
            "axes.spines.top": False,
            "lines.markeredgewidth": 0.1,
            "axes.labelsize": 7,
            "font.size": 7,
            "xtick.labelsize": 7,
            "ytick.labelsize": 7,
            "legend.fontsize": 7,
            "font.family": "serif",
            "font.serif": ["Computer Modern Roman"] + plt.rcParams['font.serif'],
        },
    )
    plt.rcParams['pdf.fonttype'] = 42
    plt.rcParams['ps.fonttype'] = 42

    testutils.check_and_create_dir(args.out_dir)
    plot_preconditions(args, extra_args)
    if args.input_dir:
        plot_strategies(args, extra_args)
    else:
        print("No input directory provided. Not plotting coverage data.")


if __name__ == "__main__":
    # Parse options and process argv
    arguments, argv = PARSER.parse_known_args()
    main(arguments, argv)
