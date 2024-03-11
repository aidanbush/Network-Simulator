import csv
import statistics
import numpy as np
import matplotlib.pyplot as plt
import os
import math

import sys

FIGSIZE = (16/1.5,9/1.5)
output_dir = "results"
plot_format = "pdf"

# plot averages of multiple runs throughputs
def multiple_run_throughput(test_name, utilization, net_size, run_name_data):
    mean_field = "Throughput mean"
    y_lim = (0, 300000)
    multiple_run_plot(test_name, utilization, mean_field, "Throughputs", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_sent_rate(test_name, utilization, net_size, run_name_data):
    mean_field = "SentRate mean"
    y_lim = (0, 300000)
    multiple_run_plot(test_name, utilization, mean_field, "Sent Rates", y_lim, "/results.csv", net_size, run_name_data)

# plot averages of multiple runs throughputs
def multiple_run_hop_ratio(test_name, utilization, net_size, run_name_data):
    mean_field = "HopRatio mean"
    y_lim = (1, 3)
    multiple_run_plot(test_name, utilization, mean_field, "Hop Ratios", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_drop_rate(test_name, utilization, net_size, run_name_data):
    mean_field = "DropRate mean"
    y_lim = (0, .3)
    multiple_run_plot(test_name, utilization, mean_field, "Drop Rate", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_out_of_order(test_name, utilization, net_size, run_name_data):
    mean_field = "OutOfOrderRatio mean"
    y_lim = (0, .5)
    multiple_run_plot(test_name, utilization, mean_field, "Out of Order Ratio", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_link_usage(test_name, utilization, net_size, run_name_data):
    fig_type = "links usage"
    file_suffix = "/links.csv"
    ylim = (0, .4)

    mean_field = "mean"
    mean_fig_name = f"{test_name} {net_size} Bursty {utilization} Utilization mean {fig_type}"
    stdev_fig_name = f"{test_name} {net_size} Bursty {utilization} Utilization stdev {fig_type}"
    mean_output_name = mean_fig_name.replace(' ', '_')
    stdev_output_name = stdev_fig_name.replace(' ', '_')

    directory = "results/"
    run_infixes = run_name_data[0]
    run_suffix = run_name_data[1]

    run_infixes = sorted(run_infixes)

    data = {}

    for run_infix in run_infixes:
        run_name = get_run_name(net_size, utilization, run_infix, run_suffix)
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
        plt.close()
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
        plt.close()
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", stdev_output_name, "exception", str(e))

def get_runs_data(filename, field_name, exact_match=True):
    data = []
    with open(filename) as f:
        reader = csv.DictReader(f)

        fieldnames = []

        for name in reader.fieldnames:
            if (exact_match and field_name == name) or (not exact_match and field_name in name):
                fieldnames.append(name)

        for row in reader:
            row_data = []
            # all rows that have a mean
            for field in fieldnames:
                val = float(row[field])
                row_data.append(val)
            data.append(row_data)
    return np.array(data)

def multiple_run_plot(test_name, utilization, mean_field, fig_type, ylim, file_suffix, net_size, run_name_data):
    fig_name = f"{test_name} {net_size} Bursty {utilization} Utilization {fig_type}"
    output_name = fig_name.replace(' ', '_')

    directory = "results/"

    #run_prefix = run_name_data[0]
    run_infixes = run_name_data[0]
    run_suffix = run_name_data[1]

    run_infixes = sorted(run_infixes)

    data = {}

    # for each run calculate averages
    for run_infix in run_infixes:
        run_name = get_run_name(net_size, utilization, run_infix, run_suffix)
        filename = directory + run_name + file_suffix

        means = []

        run_data = get_runs_data(filename, mean_field)
        data[run_name] = np.nanmean(run_data,1)

        #data[run_name] = np.array(means)

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
        plt.close()
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

def plot_across_utils(test_name, utilizations, field, fig_type, ylim, file_suffix, net_size, run_name_data, exact_match=True, include_stdev=True, include_min_max=False, stdev_not_mean=False):
    fig_name = f"{test_name} {net_size} Bursty {fig_type} fc 200 long"
    output_name = fig_name.replace(' ', '_')

    utilizations = sorted(utilizations)

    directory = "results/"

    run_infixes = run_name_data[0]
    run_suffix = run_name_data[1]

    run_infixes = sorted(run_infixes)

    means = {}
    stdevs = {}
    mins = {}
    maxs = {}

    # for agent / run
    for run_infix in run_infixes:
        # for util
        run_name = get_no_util_run_name(net_size, run_infix, run_suffix)
        means[run_name] = []
        stdevs[run_name] = []
        mins[run_name] = []
        maxs[run_name] = []

        for util in utilizations:
            file_run_name = get_run_name(net_size, util, run_infix, run_suffix)
            filename = directory + file_run_name + file_suffix

            run_data = get_runs_data(filename, field, exact_match=exact_match)
            # only look at second half
            run_data = run_data[run_data.shape[0]//2:]
            means[run_name].append(np.nanmean(run_data))
            stdevs[run_name].append(np.nanstd(run_data))

            #TODO these min and max are incorrect
            if len(run_data.shape) > 1:
                mins[run_name].append(np.nanmin(run_data))
                maxs[run_name].append(np.nanmax(run_data))
            else:
                mins[run_name].append(run_data.min())
                maxs[run_name].append(run_data.max())

        means[run_name] = np.array(means[run_name])
        stdevs[run_name] = np.array(stdevs[run_name])
        mins[run_name] = np.array(mins[run_name])
        maxs[run_name] = np.array(maxs[run_name])

    plt.figure(figsize=FIGSIZE)
    plt.title(fig_name)

    # TODO here
    loc = np.arange(len(utilizations))
    width = 1 / (len(means.keys())+1)

    #for run in sorted(means.keys()):
    for i, run in enumerate(sorted(means.keys())):
        #plt.plot(utilizations, means[run], label=run, alpha=.9)
        yerr = None
        if include_stdev:
            yerr = stdevs[run]
        if include_min_max:
            yerr = np.stack([means[run] - mins[run], maxs[run] - means[run]])
        if include_stdev and include_min_max:
            print("ERROR CANNOT HAVE STDEV AND MIN MAX")
            return None

        # TODO plot min and max if enabled
        x = means[run]
        if stdev_not_mean:
            x = stdevs[run]

        # line plot
        #plt.errorbar(utilizations, x, yerr=yerr, capsize=5, label=run, alpha=.9)
        # bar plot
        offset = width * i
        plt.bar(loc + offset, x, width, yerr=yerr, label=run)

    plt.legend()

    plt.ylim(bottom=ylim[0], top=ylim[1])
    plt.grid()

    plt.xlabel("Network Utilization")

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        plt.close()
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

def get_no_util_run_name(net_size, infix, suffix):
    prefix = f"{net_size}_"
    return prefix + infix + suffix

def get_run_name(net_size, utilization, infix, suffix):
    prefix = f"{net_size}_"
    return prefix + infix + suffix + "_u_" + utilization

utilizations = ["0.05","0.1","0.15","0.2"]
net_size = "8x8"
run_infixes = [#"rand_forward_fc_200_long",
               #"rand_deflect_fc_200_long",
               #"mbd_[1_hop_shortest]",

               # state test

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


experiment_name ="algorithm test"
run_infixes = [
               "rand_forward_prop_0.1",
               "mbd_original_[1_hop_shortest,3x3_section]_prop_0.1",
               "mbd_slide_[1_hop_shortest,3x3_section]_prop_0.1",
               "mbd_D-LinUCB_0.999_[1_hop_shortest,3x3_section]_prop_0.1",
               "mbd_D-LinUCB_0.9999_[1_hop_shortest,3x3_section]_prop_0.1",
            ]


experiment_name = "state test"
run_infixes = [
               #"rand_forward_prop_0.1",
               #"rand_deflect_prop_0.1",
               "mbd_slide_[1_hop_shortest]_prop_0.1",
               "mbd_slide_[1_hop_shortest,3x3_section]_prop_0.1",
               "mbd_slide_[2_hop_shortest,1_hop_shortest]_prop_0.1",
               "mbd_slide_[2_hop_shortest,1_hop_shortest,3x3_section]_prop_0.1",
               "mbd_slide_[1-2_hop_shortest]_prop_0.1",
               "mbd_slide_[1-2_hop_shortest,3x3_section]_prop_0.1",
            ]

experiment_name = "propagation test"
run_infixes = [
               "rand_forward_prop_0.1",
               "rand_forward_prop_0.01",
               "rand_forward_prop_0.5",
               "mbd_slide_[1_hop_shortest,3x3_section]_prop_0.1",
               "mbd_slide_[1_hop_shortest,3x3_section]_prop_0.01",
               "mbd_slide_[1_hop_shortest,3x3_section]_prop_0.5",
            ]

run_suffix = ""

'''
experiment_name = "changing flows test"
run_infixes = [
                "mbd_slide_[1_hop_shortest,3x3_section]_prop_0.1",
                "rand_forward_prop_0.1",
                "rand_deflect_prop_0.1",
            ]
run_suffix = ""
'''

utilizations = ["0.05"]
experiment_name = "changes test"
run_infixes = [
                "rand_forward_prop_0.1",
                "delete_prop_0.1",
            ]
run_suffix = ""

utilizations = ["0.05"]
experiment_name = "test NDD"
run_infixes = [
        #"mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_1.0_mice-elephant_p_0.1",
        #"rand_forward_mice-elephant_p_0.1",
        #"NDD_rand_mice-elephant_p_0.01",
        #"NDD_Q-learning_a_0.05_e_0.05_g_0.99_mice-elephant_p_0.1",
        #"NDD_drop_mice-elephant_p_0.1",
        #"NDD_Q-learning_a_0.05_e_0.05_g_0.99_mice-elephant_p_0.01",
        "NDD_Q-learning_a_0.05_e_0.05_g_0.99_mice-elephant_p_0.01",
        "NDD_rand_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_1.0_sd_-1_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_1.0_sd_2_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_1.0_sd_4_mice-elephant_p_0.01",
        ]
run_suffix = ""

'''
utilizations = ["0.2"]
experiment_name = "test NDD"
run_infixes = [
        "NDD_Q-learning_a_0.05_e_0.05_g_0.99_mice-elephant_p_0.01",
        "NDD_rand_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_1.0_sd_-1_mice-elephant_p_0.01",
        "rand_forward_mice-elephant_p_0.01",
        ]
run_suffix = ""
'''

'''
experiment_name = "parameter sweep"
run_infixes = [
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_0.9_d_0.9_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_0.9_d_1.0_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_0.9_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_1.0_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.1_d_0.9_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.1_d_1.0_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_0.9_d_0.5_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.0_d_0.5_mice-elephant_p_0.1",
        "mbd_slide_s_[1_hop_shortest,3x3_section]_r_1.1_d_0.5_mice-elephant_p_0.1",
        ]
run_suffix = ""
'''

run_name_data = (run_infixes, run_suffix)

for utilization in utilizations:
    multiple_run_throughput(experiment_name, utilization, net_size, run_name_data)
    multiple_run_hop_ratio(experiment_name, utilization, net_size, run_name_data)
    multiple_run_drop_rate(experiment_name, utilization, net_size, run_name_data)
    multiple_run_link_usage(experiment_name, utilization, net_size, run_name_data)
    #multiple_run_out_of_order(experiment_name, utilization, net_size, run_name_data)
    multiple_run_sent_rate(experiment_name, utilization, net_size, run_name_data)

#plot_across_utils(experiment_name, utilizations, "DropRate mean", "Drop Rate", (0,.3), "/results.csv", net_size, run_name_data)
#plot_across_utils(experiment_name, utilizations, "HopRatio mean", "Hop Ratios", (0,3), "/results.csv", net_size, run_name_data)
#plot_across_utils(experiment_name, utilizations, "OutOfOrderRatio mean", "Out of Order Ratio", (0,.5), "/results.csv", net_size, run_name_data)
# link mean
#plot_across_utils(experiment_name, utilizations, "mean", "Link Usage", (0,.5), "/links.csv", net_size, run_name_data, exact_match=False, include_stdev=False, include_min_max=True)
#plot_across_utils(experiment_name, utilizations, "mean", "Link Usage stdev", (0,.2), "/links.csv", net_size, run_name_data, exact_match=False, include_stdev=False, include_min_max=False, stdev_not_mean=True)
