import csv
import statistics
import numpy as np
import matplotlib.pyplot as plt
import os

FIGSIZE = (16,9)
output_dir = "results"
plot_format = "pdf"

# plot averages of multiple runs throughputs
def multiple_run_throughput():
    mean_field = "Throughput mean"
    stdev_field = "Throughput stdev"
    multiple_run_plot(mean_field, stdev_field, "Throughputs", (0, 500000))

# plot averages of multiple runs throughputs
def multiple_run_hop_ratio():
    mean_field = "HopRatio mean"
    stdev_field = "HopRatio stdev"
    multiple_run_plot(mean_field, stdev_field, "Hop Ratios", (0, 5))

def multiple_run_drop_rate():
    mean_field = "DroppedPackets mean"
    stdev_field = "DroppedPackets stdev"
    multiple_run_plot(mean_field, stdev_field, "Drop Rate", (0, 1))

def multiple_run_plot(mean_field, stdev_field, fig_type, ylim):
    utilization = "0.4"
    #fig_name = f"8x8 Bursty {utilization} Utilization {fig_type}"
    fig_name = f"5x5 Bursty {utilization} Utilization {fig_type}"
    #fig_name = f"5x5 Bursty {utilization} fs 4 Utilization {fig_type}"
    #fig_name = f"5x5 Bursty {utilization} fc 20 Utilization {fig_type}"
    output_name = fig_name.replace(' ', '_')

    directory = "results/"
    #run_prefix = f"8x8_bursty_{utilization}_"
    run_prefix = f"5x5_bursty_{utilization}_"
    #run_infixes = ["mbd_[1-2_hop_shortest]", "mbd_[dest_id]", "rand_deflect", "mbd_[1_hop_shortest]", "mbd_[2_hop_shortest]", "mbd_[2_hop_shortest,1_hop_shortest]", "mbd_[flow_id]"]
    run_infixes = ["mbd_[2_hop_shortest,1_hop_shortest]", "mbd_[1_hop_shortest]", "mbd_[dest_id]", "rand_deflect", "rand_forward"]
    #run_infixes = ["mbd_[2_hop_shortest,1_hop_shortest]", "mbd_[1_hop_shortest]", "mbd_[dest_id]", "mbd_[1_hop_shortest,3x3_section]", "rand_deflect", "rand_forward"]
    #run_infixes = ["mbd_[1_hop_shortest]", "rand_deflect", "rand_forward"]
    run_suffix = ""
    #run_suffix = "_fs_4"
    #run_suffix = "_fc_20"
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
                    if val != 0.0: # ignore 0s as they are from flows that are not currently on
                        row_means.append(val)
                # all rows that have a throughput stdev
                for field in stdev_fieldnames:
                    val = float(row[field])
                    if val != 0.0: # ignore 0s as they are from flows that are not currently on
                        row_stdevs.append(val)

                means.append(row_means)
                stdevs.append(row_stdevs)

                # uncomment 1 and 3 for only flow reporting results
                #means.append(statistics.mean(row_means))
                #stdevs.append((sum([stdev**2 for stdev in row_stdevs]))**.5)
                #stdevs.append(statistics.stdev(row_means))

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

multiple_run_throughput()
multiple_run_hop_ratio()
multiple_run_drop_rate()
