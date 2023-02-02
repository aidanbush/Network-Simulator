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
    fig_name = "5x5 Bursty 0.4 Utilization Throughputs"
    output_name = fig_name.replace(' ', '_')

    directory = "results/"
    run_prefix = "5x5_bursty_0.4_"
    run_suffixes = ["mbd_[1-2_hop_shortest]", "mbd_[dest_id]", "rand_deflect", "mbd_[1_hop_shortest]", "mbd_[2_hop_shortest]", "mbd_[2_hop_shortest,1_hop_shortest]", "mbd_[flow_id]"]
    #run_suffixes = ["mbd_[dest_id]", "rand_deflect", "mbd_[1_hop_shortest]", "mbd_[2_hop_shortest,1_hop_shortest]", "mbd_[flow_id]"]
    file_suffix = "/results.csv"

    data = {}

    # for each run calculate averages
    for run_suffix in run_suffixes:
        run_name = run_prefix + run_suffix
        filename = directory + run_name + file_suffix

        means = []
        stdevs = []

        with open(filename) as f:
            reader = csv.DictReader(f)

            mean_fieldnames = []
            stdev_fieldnames = []

            for name in reader.fieldnames:
                if "Throughput mean" in name:
                    mean_fieldnames.append(name)
                if "Throughput stdev" in name:
                    stdev_fieldnames.append(name)

            for row in reader:
                row_means = []
                row_stdevs = []
                # all rows that have a throughput mean
                for mean_field in mean_fieldnames:
                    row_means.append(float(row[mean_field]))
                # all rows that have a throughput stdev
                for stdev_field in stdev_fieldnames:
                    row_stdevs.append(float(row[stdev_field]))

                means.append(statistics.mean(row_means))
                stdevs.append((sum([stdev**2 for stdev in row_stdevs]))**.5)

        data[run_name] = (np.array(means), np.array(stdevs))

    # plot data
    plt.figure(figsize=FIGSIZE)
    plt.title(fig_name)

    for run in sorted(data.keys()):
        run_data = data[run]
        plt.plot(run_data[0], label=run, alpha=0.9)
        #plt.fill_between(range(len(run_data[0])), run_data[0] - run_data[1], run_data[0] + run_data[1], alpha=1/3)
    plt.legend()

    plt.ylim(bottom=0, top=500000)
    plt.grid()

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

multiple_run_throughput()

