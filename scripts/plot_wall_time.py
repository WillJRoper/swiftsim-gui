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

    # 2) Compute cumulative wallclock
    df["cumWall"] = df["Wallclock"].cumsum()

    # 3) Plot setup
    plt.style.use("dark_background")
    plt.figure(figsize=(12, 12 / 2.34375), dpi=1000)
    plt.xlabel("Age of the Universe (Gyr)", color=HAL_RED, fontsize=16)
    plt.ylabel("Runtime (Hours)", color=HAL_RED, fontsize=16)
    plt.grid(True, color=HAL_RED, linestyle="--", linewidth=0.5)

    # Axis label colors
    plt.gca().tick_params(colors=HAL_RED, which="both", axis="both", labelsize=12)
    for spine in plt.gca().spines.values():
        spine.set_color(HAL_RED)
        spine.set_linewidth(0.5)
        spine.set_linestyle("--")

    # 4) Single series with linestyle[0]
    plt.plot(
        df["Time"],
        df["cumWall"] / 1000 / 60 / 60,  # Convert to hours
        color=HAL_RED,
        linestyle=LINESTYLES[0],
        linewidth=2,
        label="Wallclock",
    )

    plt.tight_layout()
    plt.savefig(outpng, facecolor="black", dpi=1000)
    plt.close()


if __name__ == "__main__":
    main()
