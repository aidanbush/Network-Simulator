import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

net_size = 8#16

def dist(c1, c2):
    return abs(c1[0] - c2[0]) + abs(c1[1] - c2[1])

def calc_plot_stats(net_size):
    paths = sorted([dist((x1,y1), (x2,y2))
                    for x1 in range(net_size) for y1 in range(net_size)
                    for x2 in range(net_size) for y2 in range(net_size)
                    if (x1,y1) != (x2,y2)])

    mean = np.mean(paths)
    median = np.median(paths)

    #plot value for each path len plot the count
    counts_df = pd.DataFrame(pd.value_counts(paths)).T
    columns = sorted(counts_df.columns)
    counts_df = counts_df[columns]

    # normalize
    counts_sum = counts_df.values.sum()
    counts_df = counts_df / counts_sum * 100

    print("Mean:    ", mean)
    print("Median:  ", median)
    print("Columns: ", columns)
    print("Counts:  ", counts_df)

    plt.plot(columns, counts_df.values[0])
    plt.show()

calc_plot_stats(net_size)
