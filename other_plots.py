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
def multiple_run_throughput(utilization, net_size, run_name_data):
    mean_field = "Throughput mean"
    y_lim = (0, 300000)
    multiple_run_plot(utilization, mean_field, "Throughputs", y_lim, "/results.csv", net_size, run_name_data)

# plot averages of multiple runs throughputs
def multiple_run_hop_ratio(utilization, net_size, run_name_data):
    mean_field = "HopRatio mean"
    y_lim = (1, 3)
    multiple_run_plot(utilization, mean_field, "Hop Ratios", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_drop_rate(utilization, net_size, run_name_data):
    mean_field = "DropRate mean"
    y_lim = (0, .3)
    multiple_run_plot(utilization, mean_field, "Drop Rate", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_link_usage(utilization, net_size, run_name_data):
    fig_type = "links usage"
    file_suffix = "/links.csv"
    ylim = (0, .4)

    mean_field = "mean"
    mean_fig_name = f"{net_size} Bursty {utilization} Utilization mean {fig_type} update tests fc 200 long"
    stdev_fig_name = f"{net_size} Bursty {utilization} Utilization stdev {fig_type} update tests fc 200 long"
    mean_output_name = mean_fig_name.replace(' ', '_')
    stdev_output_name = stdev_fig_name.replace(' ', '_')

    directory = "results/"
    run_prefix = run_name_data[0]
    run_infixes = run_name_data[1]
    run_suffix = run_name_data[2]

    run_infixes = sorted(run_infixes)

    data = {}

    for run_infix in run_infixes:
        run_name = run_prefix + run_infix + run_suffix
        filename = directory + run_name + file_suffix

        #for mean_field in mean_fields:
        means = []
        mins = []
        maxs = []

        stdevs = []

        with open(filename) as f:
            reader = csv.DictReader(f)

            mean_fieldnames = []

            for name in reader.fieldnames:
                if mean_field in name:
                    mean_fieldnames.append(name)

            for row in reader:
                row_means = []
                # all rows that have a mean
                for field in mean_fieldnames:
                    val = float(row[field])
                    row_means.append(val)

                means.append(np.nanmean(np.array(row_means)))
                mins.append(np.nanmax(np.array(row_means)))
                maxs.append(np.nanmin(np.array(row_means)))
                stdevs.append(np.nanstd(np.array(row_means)))

        data[run_name] = [np.array(means), np.array(maxs), np.array(mins), stdevs]

    # plot mean data
    plt.figure(figsize=FIGSIZE)
    plt.title(mean_fig_name)

    for run in sorted(data.keys()):
        mean_data = data[run][0]
        min_data = data[run][1]
        max_data = data[run][2]
        p = plt.plot(mean_data, label=run, alpha=0.9)
        plt.fill_between(range(len(mean_data)), mean_data - min_data, mean_data + max_data, alpha=1/3, facecolor=(1,1,1,1), edgecolor=p[-1].get_color())
    plt.legend()

    plt.ylim(bottom=ylim[0], top=ylim[1])
    plt.grid()

    plt.xlabel("Time (s)")

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(mean_output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", mean_output_name, "exception", str(e))

    # plot stdev data
    plt.figure(figsize=FIGSIZE)
    plt.title(stdev_fig_name)

    for run in sorted(data.keys()):
        stdev_data = data[run][3]
        p = plt.plot(stdev_data, label=run, alpha=0.9)
    plt.legend()

    plt.ylim(bottom=ylim[0], top=ylim[1])
    plt.grid()

    plt.xlabel("Time (s)")

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(stdev_output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", stdev_output_name, "exception", str(e))

def multiple_run_plot(utilization, mean_field, fig_type, ylim, file_suffix, net_size, run_name_data):
    fig_name = f"{net_size} Bursty {utilization} Utilization {fig_type} update tests fc 200 long"
    output_name = fig_name.replace(' ', '_')

    directory = "results/"

    run_prefix = run_name_data[0]
    run_infixes = run_name_data[1]
    run_suffix = run_name_data[2]

    run_infixes = sorted(run_infixes)

    data = {}

    # for each run calculate averages
    for run_infix in run_infixes:
        run_name = run_prefix + run_infix + run_suffix
        filename = directory + run_name + file_suffix

        means = []

        with open(filename) as f:
            reader = csv.DictReader(f)

            mean_fieldnames = []

            for name in reader.fieldnames:
                if mean_field in name:
                    mean_fieldnames.append(name)

            for row in reader:
                row_means = []
                # all rows that have a mean
                for field in mean_fieldnames:
                    val = float(row[field])
                    row_means.append(val)

                means.append(np.nanmean(np.array(row_means)))

        data[run_name] = np.array(means)

    # plot data
    plt.figure(figsize=FIGSIZE)
    plt.title(fig_name)

    for run in sorted(data.keys()):
        run_data = data[run]
        plt.plot(run_data, label=run, alpha=0.9)
    plt.legend()

    plt.ylim(bottom=ylim[0], top=ylim[1])
    plt.grid()

    plt.xlabel("Time (s)")

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

utilizations = ["0.05","0.1","0.15"]#,"0.2"]
net_size = "8x8"
run_infixes = ["rand_forward_fc_200_long",
               #"rand_deflect_fc_200_long",
               #"rand_deflect_fc_200_long",
               #"mbd_[1_hop_shortest]",
               "mbd_original_[1_hop_shortest,3x3_section]_fc_200",
               "mbd_slide_[1_hop_shortest,3x3_section]_fc_200",
               #"mbd_D-LinUCB_0.9_[1_hop_shortest,3x3_section]_fc_200",
               #"mbd_D-LinUCB_0.99_[1_hop_shortest,3x3_section]_fc_200",
               "mbd_D-LinUCB_0.999_[1_hop_shortest,3x3_section]_fc_200",
               "mbd_D-LinUCB_0.9999_[1_hop_shortest,3x3_section]_fc_200",
               #"mbd_D-LinUCB_0.99999_[1_hop_shortest,3x3_section]_fc_200",
               #"mbd_[1_hop_shortest,3x3_section]_fc_200_long",
               #"mbd_[1_hop_shortest,3x3_section]_fc_200_long",
               #"mbd_[1_hop_shortest,3x3_section]_fc_100_linUCB_update",
               #"mbd_[1_hop_shortest,3x3_section]_fc_100_D-linUCB_update",
               #"mbd_[1_hop_shortest,3x3_section]_fc_100_D-linUCB_update.9999",
               #"mbd_[1_hop_shortest,3x3_section]_fc_100_D-linUCB_update.99999",
               #"mbd_[1_hop_shortest,3x3_section]_fc_100_D-linUCB_update1.0",
               #"mbd_[1_hop_shortest,3x3_section,deflect_probability]",
               #"mbd_[1_hop_shortest,3x3_section,drop_probability]",
               #"mbd_[2_hop_shortest,1_hop_shortest]",
               #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section]",
               #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section]_fc_100_slide_update",
               #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section]_fc_100_linUCB_update",
               #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]",
               #"mbd_[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]",
               #"mbd_[dest_id]",
               #"mbd_[dest_id,deflect_probability]",
               #"mbd_[dest_id,drop_probability]"
               ]
run_suffix = ""


for utilization in utilizations:
    run_prefix = f"{net_size}_bursty_{utilization}_"
    run_name_data = (run_prefix, run_infixes, run_suffix)
    multiple_run_throughput(utilization, net_size, run_name_data)
    multiple_run_hop_ratio(utilization, net_size, run_name_data)
    multiple_run_drop_rate(utilization, net_size, run_name_data)
    multiple_run_link_usage(utilization, net_size, run_name_data)
