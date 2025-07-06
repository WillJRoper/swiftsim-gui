import sys

import matplotlib.pyplot as plt
import pandas as pd

plt.rcParams["font.family"] = "Eurostile Extended"

HAL_RED = (213 / 255, 19 / 255, 23 / 255)
LINESTYLES = ["-", "--", "-.", ":"]


def main():
    if len(sys.argv) != 3:
        print("Usage: plot_particles.py <input.txt> <output.png>", file=sys.stderr)
        sys.exit(1)
    infile, outpng = sys.argv[1], sys.argv[2]

    # 1) Load
    df = pd.read_csv(infile, sep=r"\s+")

    # 3) Plot setup
    plt.style.use("dark_background")
    plt.figure(figsize=(12, 12 / 2.34375), dpi=300)
    plt.xlabel("Age of the Universe (Gyr)", color=HAL_RED, fontsize=16)
    plt.ylabel("Particle Count", color=HAL_RED, fontsize=16)
    plt.grid(True, color=HAL_RED, linestyle="--", linewidth=0.5)

    ax = plt.gca()
    ax.set_yscale("log")
    ax.tick_params(
        colors=HAL_RED,
        which="both",
        axis="both",
        labelcolor=HAL_RED,
        labelsize=12,
    )
    for spine in ax.spines.values():
        spine.set_color(HAL_RED)
        spine.set_linewidth(0.5)
        spine.set_linestyle("--")

    # 4) Plot each particle type
    plt.plot(
        df["Time"],
        df["Nparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[1],
        linewidth=2,
        label="Gas",
    )
    plt.plot(
        df["Time"],
        df["Ngparts"] - df["Nparts"] - df["Nsparts"] - df["Nbparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[0],
        linewidth=2,
        label="Dark Matter",
    )
    plt.plot(
        df["Time"],
        df["Nsparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[2],
        linewidth=2,
        label="Stars",
    )
    plt.plot(
        df["Time"],
        df["Nbparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[3],
        linewidth=2,
        label="Black Holes",
    )

    plt.legend(
        facecolor="black",
        edgecolor=HAL_RED,
        labelcolor=HAL_RED,
        fontsize=16,
    )

    plt.tight_layout()
    plt.savefig(outpng, facecolor="black", dpi=300)
    plt.close()


if __name__ == "__main__":
    main()
