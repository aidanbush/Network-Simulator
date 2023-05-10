import csv
import statistics
import numpy as np
import matplotlib.pyplot as plt
import os
import math

FIGSIZE = (16,9)
output_dir = "results"
plot_format = "pdf"

# plot averages of multiple runs throughputs
def multiple_run_throughput(utilization):
    mean_field = "Throughput mean"
    stdev_field = "Throughput stdev"
    multiple_run_plot(utilization, mean_field, stdev_field, "Throughputs", (0, 300000))

# plot averages of multiple runs throughputs
def multiple_run_hop_ratio(utilization):
    mean_field = "HopRatio mean"
    stdev_field = "HopRatio stdev"
    multiple_run_plot(utilization, mean_field, stdev_field, "Hop Ratios", (1, 3))

def multiple_run_drop_rate(utilization):
    mean_field = "DropRate mean"
    stdev_field = "DropRate stdev"
    multiple_run_plot(utilization, mean_field, stdev_field, "Drop Rate", (0, .3))

def multiple_run_plot(utilization, mean_field, stdev_field, fig_type, ylim):
    net_size = "8x8"
    fig_name = f"{net_size} Bursty {utilization} Utilization {fig_type}"
    output_name = fig_name.replace(' ', '_')

    directory = "results/"
    run_prefix = f"{net_size}_bursty_{utilization}_"
    run_infixes = ["rand_forward",
                   "rand_deflect",
                   "mbd_[1_hop_shortest]",
                   "mbd_[1_hop_shortest,3x3_section]",
                   #"mbd_[1_hop_shortest,3x3_section,deflect_probability]",
                   #"mbd_[1_hop_shortest,3x3_section,drop_probability]",
                   "mbd_[2_hop_shortest,1_hop_shortest]",
                   #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section]",
                   #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]",
                   #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]",
                   "mbd_[dest_id]",
                   #"mbd_[dest_id,deflect_probability]",
                   #"mbd_[dest_id,drop_probability]"
                   ]
    run_suffix = ""
    file_suffix = "/results.csv"

    run_infixes = sorted(run_infixes)

    data = {}

    # for each run calculate averages
    for run_infix in run_infixes:
        run_name = run_prefix + run_infix + run_suffix
        filename = directory + run_name + file_suffix

        means = []
        stdevs = []

        with open(filename) as f:
            reader = csv.DictReader(f)

            mean_fieldnames = []
            stdev_fieldnames = []

            for name in reader.fieldnames:
                if mean_field in name:
                    mean_fieldnames.append(name)
                if stdev_field in name:
                    stdev_fieldnames.append(name)

            for row in reader:
                row_means = []
                row_stdevs = []
                # all rows that have a throughput mean
                for field in mean_fieldnames:
                    val = float(row[field])
                    row_means.append(val)
                # all rows that have a throughput stdev
                for field in stdev_fieldnames:
                    val = float(row[field])
                    row_stdevs.append(val)

                means.append(np.nanmean(np.array(row_means)))
                stdevs.append(np.nanmean(np.array(row_stdevs)))

        data[run_name] = (np.array(means), np.array(stdevs))

    # plot data
    plt.figure(figsize=FIGSIZE)
    plt.title(fig_name)

    for run in sorted(data.keys()):
        run_data = data[run]
        plt.plot(run_data[0], label=run, alpha=0.9)
        #plt.fill_between(range(len(run_data[0])), run_data[0] - run_data[1], run_data[0] + run_data[1], alpha=1/3)
    plt.legend()

    plt.ylim(bottom=ylim[0], top=ylim[1])
    plt.grid()

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

utilizations = ["0.05","0.1","0.15","0.2"]
for utilization in utilizations:
    multiple_run_throughput(utilization)
    multiple_run_hop_ratio(utilization)
    multiple_run_drop_rate(utilization)
