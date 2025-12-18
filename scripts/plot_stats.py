import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

def plot_duration_distribution(csv_file_path, output_dir="."):
    try:
        df = pd.read_csv(csv_file_path, header=None)
    except FileNotFoundError:
        print(f"Error: CSV file not found at {csv_file_path}")
        return
    except Exception as e:
        print(f"Error loading CSV file: {e}")
        return

    # Prepare data for plotting
    plot_data = []
    for index, row in df.iterrows():
        implementation_name = row[0]
        durations = row[2:-1].tolist()
        for duration in durations:
            plot_data.append({"Implementation": implementation_name, "DurationNs": duration})

    if not plot_data:
        print("No duration data found to plot.")
        return

    plot_df = pd.DataFrame(plot_data)

    # Plotting - Combined
    plt.figure(figsize=(30, 7))
    ax = plt.gca() # Get current axes
    sns.histplot(data=plot_df,
                 x="DurationNs",
                 hue="Implementation", # Use hue to color bars by implementation
                 stat="count",
                 linewidth=2.0, edgecolor=None,
                 binwidth=30,
                 ax=ax) # Pass ax to histplot

    plt.title("Distribution of Duration Measurements per OrderBook Implementation")
    plt.xlabel("Duration (Nanoseconds)")
    plt.ylabel("Count (Log Scale)")
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.yscale('log') # Apply logarithmic scale to the y-axis
    # plt.legend(title="Implementation") # Seaborn's histplot with hue automatically adds a legend

    os.makedirs(output_dir, exist_ok=True)
    png_path = os.path.join(output_dir, "duration_distribution_combined.png")
    svg_path = os.path.join(output_dir, "duration_distribution_combined.svg")

    plt.savefig(png_path)
    plt.savefig(svg_path)
    plt.close() # Close the combined plot figure

    print(f"Combined plots saved to {png_path} and {svg_path}")

    # Generate separate plots for each implementation
    for implementation_name in plot_df["Implementation"].unique():
        plt.figure(figsize=(30, 7))
        ax = plt.gca() # Get current axes
        sns.histplot(data=plot_df[plot_df["Implementation"] == implementation_name],
                     x="DurationNs",
                     stat="count",
                     linewidth=2.0, edgecolor=None,
                     binwidth=30,
                     ax=ax) # Pass ax to histplot

        plt.title(f"Distribution of Duration Measurements for {implementation_name}")
        plt.xlabel("Duration (Nanoseconds)")
        plt.ylabel("Count (Log Scale)")
        plt.grid(True, linestyle='--', alpha=0.6)
        plt.yscale('log')

        png_path_individual = os.path.join(output_dir, f"{implementation_name}_distribution.png")
        svg_path_individual = os.path.join(output_dir, f"{implementation_name}_distribution.svg")

        plt.savefig(png_path_individual)
        plt.savefig(svg_path_individual)
        plt.close() # Close the individual plot figure

        print(f"Individual plots for {implementation_name} saved to {png_path_individual} and {svg_path_individual}")

if __name__ == "__main__":
    csv_path = "artifacts/benchmark_results.csv"
    plot_duration_distribution(csv_path, "artifacts/vis/latency")
