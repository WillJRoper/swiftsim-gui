import sys

import matplotlib.pyplot as plt
import pandas as pd

plt.rcParams["font.family"] = "Eurostile Extended"

# HAL-red RGB normalized
HAL_RED = (229 / 255, 0, 0)

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
    plt.figure(figsize=(6.4, 2.7306666667), dpi=100)
    plt.xlabel("Scale Factor (a)", color=HAL_RED)
    plt.ylabel("Runtime (Hours)", color=HAL_RED)
    plt.grid(True, which="both", color=HAL_RED, linestyle="--", linewidth=0.5)

    # Axis label colors
    plt.gca().tick_params(colors=HAL_RED)
    for spine in plt.gca().spines.values():
        spine.set_color(HAL_RED)
        spine.set_linewidth(0.5)
        spine.set_linestyle("--")

    # 4) Single series with linestyle[0]
    plt.plot(
        df["a"],
        df["cumWall"] / 1000 / 60 / 60,  # Convert to hours
        color=HAL_RED,
        linestyle=LINESTYLES[0],
        linewidth=2,
        label="Wallclock",
    )

    plt.tight_layout()
    plt.savefig(outpng, facecolor="black")
    plt.close()


if __name__ == "__main__":
    main()
