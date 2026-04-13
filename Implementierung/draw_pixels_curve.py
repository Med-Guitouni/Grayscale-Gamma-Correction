import pandas as pd
import matplotlib.pyplot as plt

# we are loading the csv file
data = pd.read_csv("performance_pixels.csv")

# unique versions
versions = data["Version"].unique()

# Plot performance curves for each version
plt.figure(figsize=(10, 6))
for version in versions:
    version_data = data[data["Version"] == version]
    plt.plot(version_data["Pixels"], version_data["Time"], label=f"Version {version}")

# labels, legend, and title
plt.xlabel("Number of Pixels(in millions)")
plt.ylabel("Execution Time (seconds)")
plt.title("Performance Curve: Execution Time vs. Number of Pixels")
plt.legend()
plt.grid()

plt.savefig("performance_curve.png")  # Save to a file (optio.)
plt.show()  # Display the plot