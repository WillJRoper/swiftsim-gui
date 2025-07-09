import sys

import matplotlib.pyplot as plt
import pandas as pd

plt.rcParams["font.family"] = "Eurostile Extended"

HAL_RED = (213 / 255, 19 / 255, 23 / 255)

LINESTYLES = ["-", "--", "-.", ":"]


def main():
    if len(sys.argv) != 3:
        print("Usage: plot_wall_time.py <input.txt> <output.png>", file=sys.stderr)
        sys.exit(1)
    infile, outpng = sys.argv[1], sys.argv[2]

    # 1) Load whitespace-delimited data
    df = pd.read_csv(infile, sep=r"\s+")

    # 2) Create figure & main axis
    plt.style.use("dark_background")
    fig, ax = plt.subplots(figsize=(12, 12 / 2.34375), dpi=500)

    # 3) Plot the CSFRD
    ax.plot(
        df["Time"],
        df["CSFRD"],
        color=HAL_RED,
        linestyle=LINESTYLES[0],
        linewidth=2,
    )

    # 4) Styling bottom axis
    ax.set_xlabel("Age of the Universe (Gyr)", color=HAL_RED, fontsize=16)
    ax.set_ylabel(r"CSFRD / [$M_\odot$ / Gyr / cMpc$^{3}$]", color=HAL_RED, fontsize=16)
    ax.set_yscale("log")
    ax.grid(True, color=HAL_RED, linestyle="--", linewidth=0.5)
    ax.tick_params(colors=HAL_RED, which="both", axis="both", labelsize=12)
    for spine in ax.spines.values():
        spine.set_color(HAL_RED)
        spine.set_linewidth(0.5)
        spine.set_linestyle("--")

    # 6) Finalize and save
    plt.tight_layout()
    fig.savefig(outpng, facecolor="black", dpi=500)
    plt.close(fig)


if __name__ == "__main__":
    main()
