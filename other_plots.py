import csv
import statistics
import numpy as np
import matplotlib.pyplot as plt
import os
import math
import pandas as pd
import seaborn as sns

import sys

import code

FIGSIZE = (16,9)#(16/1.5,9/1.5)
output_dir = "plots"
plot_format = "pdf"

# plot averages of multiple runs throughputs
def multiple_run_throughput(test_name, utilization, net_size, run_name_data):
    mean_field = "Throughput mean"
    y_lim = (0, 300000)
    multiple_run_plot(test_name, utilization, mean_field, "Throughputs", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_all_entropy(test_name, utilization, net_size, run_name_data, interval):
    mean_field = f"allActionsEntropy_{interval} mean"
    y_lim = (0, 2)
    multiple_run_plot(test_name, utilization, mean_field, f"entropy {interval}", y_lim, "/switches.csv", net_size, run_name_data)

def multiple_run_deflection_entropy(test_name, utilization, net_size, run_name_data, interval):
    mean_field = f"deflectionEntropy_{interval} mean"
    y_lim = (0, 1)
    multiple_run_plot(test_name, utilization, mean_field, f"action type entropy {interval}", y_lim, "/switches.csv", net_size, run_name_data)

def multiple_run_all_entropy_old(test_name, utilization, net_size, run_name_data):
    mean_field = "allActionsEntropy mean"
    y_lim = (0, 2)
    multiple_run_plot(test_name, utilization, mean_field, "entropy", y_lim, "/switches.csv", net_size, run_name_data)

def multiple_run_deflection_entropy_old(test_name, utilization, net_size, run_name_data):
    mean_field = "deflectionEntropy mean"
    y_lim = (0, 1)
    multiple_run_plot(test_name, utilization, mean_field, "action type entropy", y_lim, "/switches.csv", net_size, run_name_data)

def multiple_run_reward(test_name, utilization, net_size, run_name_data):
    mean_field = "averageReward mean"
    y_lim = (0, 1)
    multiple_run_plot(test_name, utilization, mean_field, "Rewards", y_lim, "/switches.csv", net_size, run_name_data)

def multiple_run_sent_rate(test_name, utilization, net_size, run_name_data):
    mean_field = "SentRate mean"
    y_lim = (0, 300000)
    multiple_run_plot(test_name, utilization, mean_field, "Sent Rates", y_lim, "/results.csv", net_size, run_name_data)

# plot averages of multiple runs throughputs
def multiple_run_elephant_hop_ratio(test_name, utilization, net_size, run_name_data, y_lim=(1, 2)):
    mean_field = "elephant_HopRatio mean"
    multiple_run_plot(test_name, utilization, mean_field, "Elephant Hop Ratios", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_elephant_drop_rate(test_name, utilization, net_size, run_name_data, y_lim=(0,.2)):
    mean_field = "elephant_DropRate mean"
    multiple_run_plot(test_name, utilization, mean_field, "Elephant Packet Loss", y_lim, "/results.csv", net_size, run_name_data, percent_data=True)

def multiple_run_hop_ratio(test_name, utilization, net_size, run_name_data, y_lim=(1, 2)):
    mean_field = "HopRatio mean"
    multiple_run_plot(test_name, utilization, mean_field, "Hop Ratios", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_drop_rate(test_name, utilization, net_size, run_name_data, y_lim=(0, .2)):
    mean_field = "DropRate mean"
    multiple_run_plot(test_name, utilization, mean_field, "Packet Loss", y_lim, "/results.csv", net_size, run_name_data, percent_data=True)

def multiple_run_out_of_order(test_name, utilization, net_size, run_name_data):
    mean_field = "OutOfOrderRatio mean"
    y_lim = (0, .5)
    multiple_run_plot(test_name, utilization, mean_field, "Out of Order Ratio", y_lim, "/results.csv", net_size, run_name_data)

def multiple_run_forwarding(test_name, utilization, net_size, run_name_data, y_lim=(0,4)):
    mean_field = "averageForwardInterfacesRatio mean"
    multiple_run_plot(test_name, utilization, mean_field, "Forwarding Interfaces", y_lim, "/switches.csv", net_size, run_name_data)

def switch_link_usage_heatmap(test_name, utilization, net_size, run_name_data, start_offset, v_range):
    suffix = run_name_data[1]
    for infix in run_name_data[0]:
        name_data = (infix, suffix)
        multiple_run_individual_heatmap(test_name, utilization, net_size, name_data,
                "switches.csv", "averageOutgoingLinkUsage", "Average Outgoing Link Usage", start_offset, v_range)

def switch_link_usage_grid_plots(test_name, utilization, net_size, run_name_data):
    ylim = (0, 1)
    suffix = run_name_data[1]

    for infix in run_name_data[0]:
        name_data = (infix, suffix)
        multiple_run_individual_grid(test_name, utilization, net_size, name_data,
                "switches.csv", "averageOutgoingLinkUsage", "Average Outgoing Link Usage", ylim)

def reward_to_packet_loss(test_name, utilization, net_size, name_data):
    # this should be per run but right now that data is not available
    reward_field = "averageReward mean"
    packet_loss_field = "DropRate mean"
    pass

def correlation_plot(run_name_data, field_1, field_1_file, field_name_1, field_2, field_2_file, field_name_2, fig_type, data_range):
    pass
"""
    run_infixes = run_name_data[0]
    suffix = run_name_data[1]

    data = {}

    for infix in run_infixes:
        name_data = (infix, suffix)
        file_run_name = get_run_name(net_size, util, infix, run_suffix)
        filename = directory + file_run_name + file_suffix
        data[name_data] = [[],[]]
        [0] = get_runs_data(???)
        [1] = get_runs_data(???)

    for run_infix in run_infixes:
        field_1_data = get_runs_data(file_1, field_1)
        field_2_data = get_runs_data(file_2, field_2)

        if data_range != None:
            field_1_data = field_1_data[data_range[0]:data_range[1]]
            field_2_data = field_2_data[data_range[0]:data_range[1]]

    # scatter plot
    #plt.scatter()

    fig_name = f"{test_name} {net_size} Bursty {utilization} Utilization {fig_type}"
    output_name = fig_name.replace(' ', '_')

    plt.figure(figsize=FIGSIZE)
    plt.title(mean_fig_name)
"""

def multiple_run_link_usage(test_name, utilization, net_size, run_name_data):
    fig_type = "links usage"
    file_suffix = "/links.csv"
    ylim = (0, 1)

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
                val_str = row[field]
                val = np.nan
                if val_str != "":
                    val = float(row[field])
                row_data.append(val)
            data.append(row_data)
    return np.array(data)

def multiple_run_plot(test_name, utilization, mean_field, fig_type, ylim, file_suffix, net_size, run_name_data, percent_data=False):
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
        #why does this not always work?
        mask = ~np.isnan(run_data)
        plt.plot(np.arange(len(run_data))[mask], run_data[mask], label=run, alpha=0.8, linewidth=1)
    plt.legend()

    plt.ylim(bottom=ylim[0], top=ylim[1])
    plt.grid()

    if percent_data:
        ax = plt.gca()
        y_ticks = ax.get_yticks()
        ax.set_yticklabels([f'{t * 100}%' for t in y_ticks])

    plt.ylabel(fig_type)
    plt.xlabel("Time (s)")

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        plt.close()
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

def multiple_run_individual_grid(test_name, utilization, net_size, run_name_data, filename, metric, fig_type, ylim):
    run_name = get_run_name(net_size, utilization, run_name_data[0], run_name_data[1])

    fig_name = f"{test_name} {run_name} {fig_type} Grid"
    output_name = fig_name.replace(' ', '_')

    #for run_name in run_name_data
    file_suffix = filename
    file_path = os.path.join("results", run_name, file_suffix)

    df = pd.read_csv(file_path)

    dim_size = int(net_size.split("x")[0])
    fig, axs = plt.subplots(dim_size, dim_size, figsize=(20, 20))
    axs = axs.flatten()

    time_data = df["Time"]
    # for each switch
    for x, y in [(x,y) for x in range(dim_size) for y in range(dim_size)]:
        switch_id = x + y * dim_size + 1
        i = switch_id - 1
        # grab data and plot
        col_name = f"switch_{switch_id} {metric} mean"
        col_data = df[col_name]

        axs[i].plot(time_data, col_data, linewidth=1)
        #axs[i].set_xlabel("Time")
        #axs[i].set_ylabel(fig_type)
        #axs[i].set_title(f"switch {switch_id}")
        axs[i].set_xticklabels([])
        axs[i].set_yticklabels([])
        axs[i].set_ylim(ylim)

    fig.suptitle(fig_name)
    #fig.supxlabel("Time")
    #fig.supylabel(fig_type)
    plt.tight_layout(rect=[0.02, 0.03, 1, 0.95])
    #fig.subplots_adjust(hspace=0.4, wspace=0.4, top=0.92, bottom=0.07)

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

    plt.close()

def multiple_run_individual_heatmap(test_name, utilization, net_size, run_name_data,
        filename, metric, fig_type, start_offset, v_range):
    run_name = get_run_name(net_size, utilization, run_name_data[0], run_name_data[1])
    fig_name = f"{test_name} {run_name} {fig_type} Heatmap"
    output_name = fig_name.replace(' ', '_')

    #for run_name in run_name_data
    file_suffix = filename
    file_path = os.path.join("results", run_name, file_suffix)

    df = pd.read_csv(file_path)

    dim_size = int(net_size.split("x")[0])

    heatmap_data = np.zeros((dim_size, dim_size))

    time_data = df["Time"]
    # for each switch
    for x, y in [(x,y) for x in range(dim_size) for y in range(dim_size)]:
        switch_id = x + y * dim_size + 1
        # grab data and plot
        col_name = f"switch_{switch_id} {metric} mean"
        #import code
        #code.interact(local=locals())
        col_data = df[col_name][start_offset:]

        heatmap_data[x, y] = col_data.mean()

    plt.figure(figsize=(10, 8))
    ax = sns.heatmap(heatmap_data, annot=False, fmt=".2f", cmap="viridis", cbar_kws={'label': 'Average Usage'},
                xticklabels=False, yticklabels=False, vmin=v_range[0], vmax=v_range[1])
    plt.title(f'Heatmap of {metric} for {run_name}')
    #plt.title(f"Switch outgoing link usage at {float(utilization) * 100}% network utilization")

    colorbar = ax.collections[0].colorbar

    # Set the colorbar tick labels to percentages
    colorbar.set_ticks(colorbar.get_ticks())
    colorbar.set_ticklabels([f'{int(t * 100)}%' for t in colorbar.get_ticks()])


    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

    plt.close()

def plot_across_utils(test_name, utilizations, field, fig_type, ylim, file_suffix, net_size, run_name_data, exact_match=True, include_stdev=True, include_min_max=False, stdev_not_mean=False):
    fig_name = f"{test_name} {net_size} Bursty {fig_type}"
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
        yerr_min_max = None
        yerr_stdev = None
        if include_stdev:
            yerr_stdev = stdevs[run]
        if include_min_max:
            yerr_min_max = np.stack([means[run] - mins[run], maxs[run] - means[run]])

        # TODO plot min and max if enabled
        x = means[run]
        if stdev_not_mean:
            x = stdevs[run]

        # line plot
        #plt.errorbar(utilizations, x, yerr=yerr, capsize=5, label=run, alpha=.9)
        # bar plot
        offset = width * i
        plt.bar(loc + offset, x, width, label=run)
        plt.errorbar(loc + offset, x, yerr=yerr_stdev, fmt='none', ecolor='black', capsize=5)
        plt.errorbar(loc + offset, x, yerr=yerr_min_max, fmt='none', ecolor='black', capsize=5)


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

utilizations = ["0.05","0.1","0.15","0.2","0.25"]
experiment_name = "utilization_sweep"
run_infixes = [
        "rand_forward_mice-elephant_p_0.01",
        "rand_deflect_mice-elephant_p_0.01",
        "rand_deflect_sd_2_mice-elephant_p_0.01",
        ]
run_suffix = ""

utilizations = ["0.1","0.2"]
experiment_name = "Algorithm Sweep"
run_infixes = [
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",

        "mbd_original_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",

        "mbd_slide_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",

        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.9999_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",

        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.26_d_0.99_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.9999_r_1.96_d_0.99_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.57_d_0.71_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.76_d_0.78_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.93_d_0.54_sd_-1_ei_5_mice-elephant_p_0.01",
        ]
run_suffix = ""

utilizations = ["0.1","0.2"]
experiment_name = "Algorithm Sweep Original"
run_infixes = [
        "mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",

        "mbd_original_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",
        ]
run_suffix = ""

utilizations = ["0.1","0.2"]
experiment_name = "Algorithm Sweep Slide"
run_infixes = [
        "mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",

        "mbd_slide_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",
        ]
run_suffix = ""

utilizations = ["0.1","0.2"][1:2]
experiment_name = "Algorithm Sweep D-LinUCB"
run_infixes = [
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",

        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.9999_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",

        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.26_d_0.99_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.9999_r_1.96_d_0.99_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.57_d_0.71_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.76_d_0.78_sd_-1_ei_5_mice-elephant_p_0.01",
        #"mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.93_d_0.54_sd_-1_ei_5_mice-elephant_p_0.01",
        ]
run_suffix = ""

utilizations = ["0.1","0.2"]
experiment_name = "Algorithm Sweep Best"
run_infixes = [
        "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
        "mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
        ]
run_suffix = ""

utilizations = ["0.1","0.2"]
experiment_name = "State Sweep"
run_infixes = [
        'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1-2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1-2_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        ]
run_suffix = ""


net_size = "16x16"
utilizations = ["0.1","0.2"][1:2]
experiment_name = "State Sweep"
run_infixes = [
        'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1-2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1-2_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "16x16"
utilizations = ["0.05","0.1"][0:1]
experiment_name = "DLRL poster"
run_infixes = [
        'NDD_Q-learning_a_0.5_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'rand_forward_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"]
experiment_name = "Limited Deflections"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_mice-elephant_p_0.01',

        #'NDD_Q-learning_a_0.05_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        #'rand_forward_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"]
experiment_name = "Limited Deflections Free Reign"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_mice-elephant_p_0.01',

        #'NDD_Q-learning_a_0.05_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        #'rand_forward_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"]
experiment_name = "Limited Deflections Forward First"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_forward_first_mice-elephant_p_0.01',

        #'NDD_Q-learning_a_0.05_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        #'rand_forward_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][0:2]
experiment_name = "Limited Deflections Only Forward"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01',

        #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        'rand_forward_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"]
experiment_name = "Limited Deflections best"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01'

        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_forward_first_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_forward_first_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"]
experiment_name = "Limited Deflections best cut"
run_infixes = [
        'c_mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'c_mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][1:2]
experiment_name = "Propagation delay experiments"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.001',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.1',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_1.0',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][1:2]
experiment_name = "Propagation delay experiments baselines"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.001',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.1',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_1.0',

        'rand_forward_mice-elephant_p_0.001',
        'rand_forward_mice-elephant_p_0.01',
        'rand_forward_mice-elephant_p_0.1',
        'rand_forward_mice-elephant_p_1.0',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][1:2]
experiment_name = "bandwidth experiments"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_0.5',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_2',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"]#[1:2]
experiment_name = "Update Frequency test"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_2_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_4_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_8_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_16_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_32_mice-elephant_p_0.01',

        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_2_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_4_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_8_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_16_mice-elephant_p_0.01',
        #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_32_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][0:1]
experiment_name = "Random Deflect Deflect Count Test"
run_infixes = [
        'rand_deflect_mice-elephant_p_0.01',
        'rand_deflect_sd_2_mice-elephant_p_0.01',
        'rand_deflect_sd_4_mice-elephant_p_0.01',
        'rand_deflect_sd_6_mice-elephant_p_0.01',
        'rand_deflect_sd_8_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][1:2]
experiment_name = "Compare 2 deflect algorithms"
run_infixes = [
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_forward_first_mice-elephant_p_0.01',
        'rand_deflect_sd_2_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"]
experiment_name = "Compare no limit deflect algorithms"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01',
        'rand_deflect_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][1:2]
experiment_name = "NDD Param Sweep"
run_infixes = [
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.006_e_0.023_g_0.685_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.081_e_0.046_g_0.999_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.002_e_0.059_g_0.996_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.013_e_0.066_g_0.958_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.05_e_0.012_g_0.993_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.009_e_0.029_g_0.131_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.021_e_0.058_g_0.997_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.002_e_0.047_g_0.997_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.096_e_0.01_g_0.748_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.088_e_0.015_g_0.923_mu_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "8x8"
utilizations = ["0.1","0.2"][1:2]
experiment_name = "NDD Deflect Sweep"
run_infixes = [
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_4_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_6_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
        ]
run_suffix = ""

"""
net_size = "8x8"
utilizations = ["0.05","0.1", "0.15", "0.2"][0:3]
experiment_name = "MBD long"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01'
        ]
run_suffix = ""
"""

net_size = "8_3d"
utilizations = ["0.05","0.1", "0.15", "0.2"][1:2]
experiment_name = "8 3d grid"
run_infixes = [
        'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
        'rand_forward_mice-elephant_p_0.01',
        'rand_deflect_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "3_3_hex"
utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
experiment_name = "sparse"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
        'rand_forward_mice-elephant_p_0.01',
        'rand_deflect_mice-elephant_p_0.01',
        ]
run_suffix = ""

net_size = "5_3_hex"
utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
experiment_name = "sparse"
run_infixes = [
        'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
        'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
        'rand_forward_mice-elephant_p_0.01',
        'rand_deflect_mice-elephant_p_0.01',
        ]
run_suffix = ""


run_name_data = (run_infixes, run_suffix)

for utilization in utilizations:
    multiple_run_throughput(experiment_name, utilization, net_size, run_name_data)

    multiple_run_elephant_hop_ratio(experiment_name, utilization, net_size, run_name_data, y_lim=(1,1.5))
    multiple_run_hop_ratio(experiment_name, utilization, net_size, run_name_data, y_lim=(1,1.5))

    multiple_run_elephant_drop_rate(experiment_name, utilization, net_size, run_name_data, y_lim=(0,.1))
    multiple_run_drop_rate(experiment_name, utilization, net_size, run_name_data, y_lim=(0,.1))

    multiple_run_link_usage(experiment_name, utilization, net_size, run_name_data)
    multiple_run_forwarding(experiment_name, utilization, net_size, run_name_data, y_lim=(0,2))
    multiple_run_sent_rate(experiment_name, utilization, net_size, run_name_data)

    multiple_run_reward(experiment_name, utilization, net_size, run_name_data)
    for entropy in [6,8]:#[1,2,4,5,6,8]:
        multiple_run_all_entropy(experiment_name, utilization, net_size, run_name_data, entropy)
        multiple_run_deflection_entropy(experiment_name, utilization, net_size, run_name_data, entropy)

    #multiple_run_all_entropy_old(experiment_name, utilization, net_size, run_name_data)
    #multiple_run_deflection_entropy_old(experiment_name, utilization, net_size, run_name_data)

    #multiple_run_out_of_order(experiment_name, utilization, net_size, run_name_data)

    # only if grid topology
    #switch_link_usage_heatmap(experiment_name, utilization, net_size, run_name_data, 500, (None,None))#(0,.5))
    #switch_link_usage_grid_plots(experiment_name, utilization, net_size, run_name_data)

#plot_across_utils(experiment_name, utilizations, "averageReward mean", "Rewards", (0.5,1), "/switches.csv", net_size, run_name_data)
#plot_across_utils(experiment_name, utilizations, "DropRate mean", "Packet Loss", (0,.1), "/results.csv", net_size, run_name_data)
#plot_across_utils(experiment_name, utilizations, "elephant_DropRate mean", "Elephant Packet Loss", (0,.1), "/results.csv", net_size, run_name_data)
#plot_across_utils(experiment_name, utilizations, "HopRatio mean", "Hop Ratios", (0,3), "/results.csv", net_size, run_name_data)
#plot_across_utils(experiment_name, utilizations, "OutOfOrderRatio mean", "Out of Order Ratio", (0,.5), "/results.csv", net_size, run_name_data)
# link mean
#plot_across_utils(experiment_name, utilizations, "mean", "Link Usage", (0,.5), "/links.csv", net_size, run_name_data, exact_match=False, include_stdev=True, include_min_max=True)
#plot_across_utils(experiment_name, utilizations, "mean", "Link Usage stdev", (0,.2), "/links.csv", net_size, run_name_data, exact_match=False, include_stdev=False, include_min_max=False, stdev_not_mean=True)
#plot_across_utils(experiment_name, utilizations, "averageForwardInterfacesRatio mean", "Forwarding Interfaces", (0, 2), "/switches.csv", net_size, run_name_data)
