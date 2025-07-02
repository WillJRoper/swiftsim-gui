import sys

import matplotlib.pyplot as plt
import pandas as pd

plt.rcParams["font.family"] = "Eurostile Extended"

HAL_RED = (229 / 255, 0, 0)
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
    plt.figure(figsize=(6.4, 2.7306666667), dpi=100)
    plt.xlabel("Scale Factor (a)", color=HAL_RED)
    plt.ylabel("Particle Count", color=HAL_RED)
    plt.grid(True, which="both", color=HAL_RED, linestyle="--", linewidth=0.5)

    ax = plt.gca()
    ax.set_yscale("log")
    ax.tick_params(colors=HAL_RED, which="both", axis="both", labelcolor=HAL_RED)
    for spine in ax.spines.values():
        spine.set_color(HAL_RED)
        spine.set_linewidth(0.5)
        spine.set_linestyle("--")

    # 4) Plot each particle type
    plt.plot(
        df["a"],
        df["Nparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[1],
        linewidth=2,
        label="Gas",
    )
    plt.plot(
        df["a"],
        df["Ngparts"] - df["Nparts"] - df["Nsparts"] - df["Nbparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[0],
        linewidth=2,
        label="Dark Matter",
    )
    plt.plot(
        df["a"],
        df["Nsparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[2],
        linewidth=2,
        label="Stars",
    )
    plt.plot(
        df["a"],
        df["Nbparts"],
        color=HAL_RED,
        linestyle=LINESTYLES[3],
        linewidth=2,
        label="Baryons",
    )

    plt.legend(
        facecolor="black",
        edgecolor=HAL_RED,
        labelcolor=HAL_RED,
    )

    plt.tight_layout()
    plt.savefig(outpng, facecolor="black")
    plt.close()


if __name__ == "__main__":
    main()
