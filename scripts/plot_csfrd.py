import sys

import matplotlib.pyplot as plt
import pandas as pd
from astropy.cosmology import Planck18 as cosmo
from astropy.cosmology import z_at_value
from astropy.units import Gyr

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

    # 2) Create figure & main axis
    plt.style.use("dark_background")
    fig, ax = plt.subplots(figsize=(12, 12 / 2.34375), dpi=300)

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

    # 5) Create top axis for redshift
    ax2 = ax.twiny()

    # Sync x-limits
    ax2.set_xlim(ax.get_xlim())

    # Bottom tick positions
    bottom_ticks = ax.get_xticks()

    # Clean up the bottom ticks
    bottom_ticks = [t for t in bottom_ticks if t > 0 and t <= 14]

    # Convert bottom ticks to redshift
    z_ticks = [
        float(z_at_value(cosmo.age, t * Gyr, zmax=140).value) for t in bottom_ticks
    ]

    bottom_ticks.insert(0, cosmo.age(127).value)
    z_ticks.insert(0, 127.0)

    print(z_ticks)

    # Apply to top axis
    ax2.set_xticks(bottom_ticks)
    ax2.set_xticklabels([f"{z:.1f}" for z in z_ticks], color=HAL_RED, fontsize=12)

    # Label & style top axis
    ax2.set_xlabel("Redshift", color=HAL_RED, fontsize=16)
    ax2.xaxis.set_label_position("top")
    ax2.xaxis.tick_top()
    ax2.tick_params(colors=HAL_RED, which="both")
    for spine in ax2.spines.values():
        spine.set_color(HAL_RED)
        spine.set_linewidth(0.5)
        spine.set_linestyle("--")

    # 6) Finalize and save
    plt.tight_layout()
    fig.savefig(outpng, facecolor="black", dpi=300)
    plt.close(fig)


if __name__ == "__main__":
    main()
