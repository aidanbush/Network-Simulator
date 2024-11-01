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
def multiple_run_throughput(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "Throughput mean"
    defaults = {"ylim": (0, 300000)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Throughputs", params["ylim"], "/results.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_all_entropy(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    defaults = {"ylim": (0, 2), "interval": 8}
    params = {**defaults, **params}

    mean_field = f"allActionsEntropy_{params['interval']} mean"
    multiple_run_plot(test_name, utilization, mean_field, f"entropy {params['interval']}", params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_deflection_entropy(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    defaults = {"ylim": (0, 1), "interval": 8}
    params = {**defaults, **params}

    mean_field = f"deflectionEntropy_{params['interval']} mean"
    multiple_run_plot(test_name, utilization, mean_field, f"action type entropy {params['interval']}", params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_all_entropy_old(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "allActionsEntropy mean"
    defaults = {"ylim": (0, 2)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "entropy", params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_deflection_entropy_old(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "deflectionEntropy mean"
    defaults = {"ylim": (0, 1)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "action type entropy", params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_reward(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "averageReward mean"
    defaults = {"ylim": (0, 1)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Rewards", params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_sent_rate(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "SentRate mean"
    defaults = {"ylim": (0, 300000)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Sent Rates", params["ylim"], "/results.csv", net_size, run_name_data, experiment_length=experiment_length)

# plot averages of multiple runs throughputs
def multiple_run_elephant_hop_ratio(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "elephant_HopRatio mean"
    defaults = {"ylim": (1, 2)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Elephant Hop Ratios", params["ylim"], "/results.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_elephant_drop_rate(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "elephant_DropRate mean"
    defaults = {"ylim": (0,.2)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Elephant Packet Loss", params["ylim"], "/results.csv", net_size, run_name_data, percent_data=True, experiment_length=experiment_length)

def multiple_run_hop_ratio(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "HopRatio mean"
    defaults = {"ylim": (1, 2)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Hop Ratios", params["ylim"], "/results.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_drop_rate(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "DropRate mean"
    defaults = {"ylim": (0, .2)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Packet Loss", params["ylim"], "/results.csv", net_size, run_name_data, percent_data=True, experiment_length=experiment_length)

def multiple_run_out_of_order(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "OutOfOrderRatio mean"
    defaults = {"ylim": (0, .5)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Out of Order Ratio", params["ylim"], "/results.csv", net_size, run_name_data, experiment_length=experiment_length)

def multiple_run_forwarding(test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "averageForwardInterfacesRatio mean"
    defaults = {"ylim": (0,4)}
    params = {**defaults, **params}

    multiple_run_plot(test_name, utilization, mean_field, "Forwarding Interfaces", params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)

#TODO new!
def multiple_util_reward_together(test_name, utilizations, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "averageReward mean"
    defaults = {"ylim": (0, 1)}
    params = {**defaults, **params}

    multiple_util_plot(test_name, utilizations, mean_field, "Rewards", params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)

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

def get_runs_data(filename, field_name, experiment_length, exact_match=True):
    data = []
    with open(filename) as f:
        reader = csv.DictReader(f)

        fieldnames = []

        for name in reader.fieldnames:
            if (exact_match and field_name == name) or (not exact_match and field_name in name):
                fieldnames.append(name)

        for line, row in enumerate(reader):
            #break after n lines
            if line >= experiment_length:
                break

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

def multiple_run_plot(test_name, utilization, mean_field, fig_type, ylim, file_suffix, net_size, run_name_data, percent_data=False, experiment_length=1000):
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

        run_data = get_runs_data(filename, mean_field, experiment_length)
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

#TODO new! this should work
def multiple_util_plot(test_name, utilizations, mean_field, fig_type, ylim, file_suffix, net_size, run_name_data, percent_data=False, experiment_length=1000):
    fig_name = f"{test_name} {net_size} Bursty {fig_type}"
    output_name = fig_name.replace(' ', '_')

    directory = "results/"

    #run_prefix = run_name_data[0]
    run_infixes = run_name_data[0]
    run_suffix = run_name_data[1]

    run_infixes = sorted(run_infixes)

    data = {}

    # for each run calculate averages
    for utilization in utilizations:
        for run_infix in run_infixes:
            run_name = get_run_name(net_size, utilization, run_infix, run_suffix)
            filename = directory + run_name + file_suffix

            means = []

            run_data = get_runs_data(filename, mean_field, experiment_length)
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

#def plot_across_utils(test_name, utilizations, field, fig_type, ylim, file_suffix, net_size, run_name_data, exact_match=True, include_stdev=True, include_min_max=False, stdev_not_mean=False, experiment_length=1000):
def plot_across_utils(test_name, utilizations, net_size, run_name_data, experiment_length, params):
    #defaults = {"field": None, "fig_type":None, "ylim":None, "file_suffix":"", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":False}
    #params = {**defaults, **params}

    field = params["field"]
    fig_type = params["fig_type"]
    ylim = params["ylim"]
    file_suffix = params["file_suffix"]
    exact_match = params["exact_match"]
    include_stdev = params["include_stdev"]
    include_min_max = params["include_min_max"]
    stdev_not_mean = params["stdev_not_mean"]
    percent_data = params["percent_data"]

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

            run_data = get_runs_data(filename, field, experiment_length, exact_match=exact_match)
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

    if percent_data:
        ax = plt.gca()
        y_ticks = ax.get_yticks()
        ax.set_yticklabels([f'{t * 100:g}%' for t in y_ticks])

    plt.xlabel("Network Utilization")
    plt.xticks(np.arange(len(utilizations)) + width/len(means.keys()), [f"{float(u)*100:g}%" for u in utilizations])

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

def get_single_util_func(name):
    mappings = {
            "hop_ratio": multiple_run_hop_ratio,
            "elephant_hop_ratio": multiple_run_elephant_hop_ratio,
            "drop_rate": multiple_run_drop_rate,
            "elephant_drop_rate": multiple_run_elephant_drop_rate,

            "throughput": multiple_run_throughput,
            "sent_rate": multiple_run_sent_rate,

            "link_usage": multiple_run_link_usage,
            "forwarding_links": multiple_run_forwarding,

            "reward": multiple_run_reward,
            "entropy": multiple_run_all_entropy,
            "action_type_entropy": multiple_run_deflection_entropy,

            "entropy_old": multiple_run_all_entropy_old,
            "action_type_entropy_old": multiple_run_deflection_entropy_old,

            "out_of_order": multiple_run_out_of_order, # unsupported

            "switch_usage_heatmap": switch_link_usage_heatmap, # 2d grid only
            "switch_usage_plots": switch_link_usage_grid_plots, # 2d grid only
            }

    if name in mappings:
        return mappings[name]
    print("############################################")
    print(f"  Plot '{name}' not found")
    print("############################################")

def plot_data(experiment_name, topology, utils, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots):
    #single_util_plots = {name: {params}}
    run_name_data = (run_infixes, run_suffix)
    for util in utils:
        for name, params in single_util_plots.items():
            func = get_single_util_func(name)
            func(experiment_name, util, topology, run_name_data, params, experiment_length=experiment_length)
            #TODO change entropy, y_lim, etc to be in a dict of paramters
    #across_util_plots = [{params}, ...]
    for params in across_util_plots:
        plot_across_utils(experiment_name, utils, topology, run_name_data, experiment_length, params)
        #TODO change field, fig_type/name, file_suffix, entropy, y_lim, etc to be in a dict of paramters

def plot_utilization_sweep():
    print("plot_utilization_sweep")

    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2","0.25"]
    experiment_name = "Utilization Sweep"
    experiment_length = 1000
    run_infixes = [
            "rand_forward_mice-elephant_p_0.01",
            "rand_deflect_mice-elephant_p_0.01",
            #"rand_deflect_sd_2_mice-elephant_p_0.01",
            ]
    run_suffix = ""
    single_util_plots = {
            #"drop_rate": {"ylim": (0,.3)},#or (0,.05), (0,.1) at 10%, 20%
            #"hop_ratio": {"ylim": (1,1.5)},
            }
    across_util_plots = [
            {"field": "DropRate mean", "fig_type": "Packet Loss", "ylim": (0,.3), "file_suffix": "/results.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True},
            {"field": "mean", "fig_type":"Link Usage", "ylim":(0,.5), "file_suffix":"/links.csv", "exact_match":False, "include_stdev":True, "include_min_max": True, "stdev_not_mean":False, "percent_data":True},
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_algorithm_sweep_original():
    print("plot_algorithm_sweep_original")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep Original"
    experiment_length = 1000
    run_infixes = [
            "mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",

            "mbd_original_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_original_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_original_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_original_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_original_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_algorithm_sweep_slide():
    print("plot_algorithm_sweep_slide")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep Slide"
    experiment_length = 1000
    run_infixes = [
            "mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",

            "mbd_slide_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_slide_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_slide_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_slide_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_slide_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01",
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_algorithm_sweep_D_LinUCB():
    print("plot_algorithm_sweep_D_LinUCB")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep D-LinUCB"
    experiment_length = 1000
    run_infixes = [
            "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",

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
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2)},#or (0,.2), (0,>.2) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_algorithm_sweep_best():
    print("plot_algorithm_sweep_best")

    # TODO currently best D-LinUCB has a run that crashes
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep Best"
    experiment_length = 1000
    run_infixes = [
            "mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
            "mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01",
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_state_sweep_8x8():
    print("plot_state_sweep_8x8")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "State Sweep"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1-2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1-2_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_state_sweep_16x16():
    print("plot_state_sweep_16x16")

    net_size = "16x16"
    utilizations = ["0.1","0.2"][1:2]
    experiment_name = "State Sweep"
    experiment_length = 2000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1-2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1-2_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2)},#or (0,.1), (0,.2) at 10%, 20%
            "hop_ratio": {"ylim": (1,2)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_dlrl_poster():
    print("plot_dlrl_poster")

    net_size = "16x16"
    utilizations = ["0.05"]
    experiment_name = "DLRL poster"
    experiment_length = 2000
    run_infixes = [
            'NDD_Q-learning_a_0.5_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'rand_forward_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2)},#or (0,.05), (0,.1) at 10%, 20%
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_limited_deflections():
    print("plot_limited_deflections")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Limited Deflections"
    experiment_length = 1000
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
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.05), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_limited_deflections_free_reign():
    print("plot_limited_deflections_free_reign")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Limited Deflections Free Reign"
    experiment_length = 1000
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
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.05), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_limited_deflections_forward_first():
    print("plot_limited_deflections_forward_first")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Limited Deflections Forward First"
    experiment_length = 1000
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
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.05), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_limited_deflecitons_only_forward():
    print("plot_limited_deflecitons_only_forward")

    net_size = "8x8"
    utilizations = ["0.1","0.2"][0:2]
    experiment_name = "Limited Deflections Only Forward"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01',
            #'mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01',

            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            'rand_forward_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.25)},#or (0,.15), (0,.25) at 10%, 20%
            #"hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_limited_deflections_best():
    print("plot_limited_deflections_best")

    # TODO do something about only forward
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Limited Deflections best"
    experiment_length = 1000
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
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_limited_deflections_best_cut():
    print("plot_limited_deflections_best_cut")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Limited Deflections best cut"
    experiment_length = 1000
    run_infixes = [
            'c_mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'c_mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2)},#or (0,.05), (0,.2) at 10%, 20%
            "hop_ratio": {"ylim": (1,2)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_prop_delay():
    print("plot_prop_delay")

    net_size = "8x8"
    #utilizations = ["0.05","0.1","0.15","0.2"][0:4]
    experiment_name = "Propagation delay experiments"
    experiment_length = 1000
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2)},#or (0,.05), (0,.2) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            }
    across_util_plots = [
            ]

    utilizations = ["0.05","0.15"][0:2]
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.001',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.1',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_1.0',

            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.001',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.1',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_1.0',

            #'rand_forward_mice-elephant_p_0.001',
            #'rand_forward_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.1',
            #'rand_forward_mice-elephant_p_1.0',

            #'rand_deflect_mice-elephant_p_0.001',
            #'rand_deflect_mice-elephant_p_0.01',
            #'rand_deflect_mice-elephant_p_0.1',
            #'rand_deflect_mice-elephant_p_1.0',
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    utilizations = ["0.1","0.2"][0:2]
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.001',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.1',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_1.0',

            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.001',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.1',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_1.0',

            #'rand_forward_mice-elephant_p_0.001',
            #'rand_forward_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.1',
            #'rand_forward_mice-elephant_p_1.0',

            #'rand_deflect_mice-elephant_p_0.001',
            #'rand_deflect_mice-elephant_p_0.01',
            #'rand_deflect_mice-elephant_p_0.1',
            #'rand_deflect_mice-elephant_p_1.0',
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_bandwidth():
    print("plot_bandwidth")

    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2"][:2]
    experiment_name = "bandwidth experiments"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_0.5',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_2',

            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01_b_0.5',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01_b_2',

            #'rand_forward_mice-elephant_p_0.01_b_0.5',
            #'rand_forward_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01_b_2',

            #'rand_deflect_mice-elephant_p_0.01_b_0.5',
            #'rand_deflect_mice-elephant_p_0.01',
            #'rand_deflect_mice-elephant_p_0.01_b_2',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    utilizations = ["0.05","0.1","0.15","0.2"][2:]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_bandwidth_all():
    print("plot_bandwidth_all")

    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2"][:2]
    experiment_name = "bandwidth experiments all"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_0.5',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_2',

            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01_b_0.5',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01_b_2',

            #'rand_forward_mice-elephant_p_0.01_b_0.5',
            #'rand_forward_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01_b_2',

            #'rand_deflect_mice-elephant_p_0.01_b_0.5',
            #'rand_deflect_mice-elephant_p_0.01',
            #'rand_deflect_mice-elephant_p_0.01_b_2',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    utilizations = ["0.05","0.1","0.15","0.2"][2:]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_update_freq_test():
    print("plot_update_freq_test")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]#[1:2]
    experiment_name = "Update Frequency test"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_2_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_4_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_8_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_16_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_32_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.02), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_random_deflect_deflect_count():
    print("plot_random_deflect_deflect_count")

    net_size = "8x8"
    utilizations = ["0.1","0.2"][0:1]
    experiment_name = "Random Deflect Deflect Count Test"
    experiment_length = 1000
    run_infixes = [
            'rand_deflect_mice-elephant_p_0.01',
            'rand_deflect_sd_2_mice-elephant_p_0.01',
            'rand_deflect_sd_4_mice-elephant_p_0.01',
            'rand_deflect_sd_6_mice-elephant_p_0.01',
            'rand_deflect_sd_8_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.15)},#or (0,.05), (0,.15) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_compare_2_deflect():
    print("plot_compare_2_deflect")

    net_size = "8x8"
    utilizations = ["0.1","0.2"][1:2]
    experiment_name = "Compare 2 deflect algorithms"
    experiment_length = 1000
    run_infixes = [
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_forward_first_mice-elephant_p_0.01',
            'rand_deflect_sd_2_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.15)},#or (0,.1), (0,.15) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_compare_limitless_deflect():
    print("plot_compare_limitless_deflect")

    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Compare no limit deflect algorithms"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01',
            'rand_deflect_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.02), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_ndd_param_sweep():
    print("plot_ndd_param_sweep")

    net_size = "8x8"
    utilizations = ["0.1","0.2"][0:2]
    experiment_name = "NDD Param Sweep"
    experiment_length = 1000
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
    single_util_plots = {
            "drop_rate": {"ylim": (0,.15)},#or (0,.05), (0,.15) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_ndd_deflect_sweep():
    print("plot_ndd_deflect_sweep")

    net_size = "8x8"
    utilizations = ["0.1","0.2"][0:2]
    experiment_name = "NDD Deflect Sweep"
    experiment_length = 1000
    run_infixes = [
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_4_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_6_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.15)},#or (0,.05), (0,.15) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_8_8_long():
    print("plot_8_8_long")

    net_size = "8x8"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    experiment_name = "long run"
    experiment_length = 5000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            'rand_forward_mice-elephant_p_0.01',
            'rand_deflect_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5)},

            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    # 5 + 10 for single
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:2]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "elephant_drop_rate": {"ylim": (0,.02)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5)},
            "elephant_hop_ratio": {"ylim": (1,1.5)},

            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # 15 + 20 for single
    utilizations = ["0.05","0.1", "0.15", "0.2"][2:4]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.04)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "elephant_drop_rate": {"ylim": (0,.04)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5)},
            "elephant_hop_ratio": {"ylim": (1,1.5)},

            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # combine reward
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            ]
    run_name_data = (run_infixes, run_suffix)
    multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {}, experiment_length=experiment_length)

def plot_16x16():
    print("plot_16x16")

    net_size = "16x16"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    experiment_name = "large network"
    experiment_length = 2000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01',
            'rand_deflect_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5)},
            }
    across_util_plots = [
            ]

    # 5 + 10 for single
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:2]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5)},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # 15 + 20 for single
    utilizations = ["0.05","0.1", "0.15", "0.2"][3:4]# TODO add 15% back later
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5)},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # combine reward
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]# TODO add 15% back later
    utilizations = ["0.05","0.1", "0.2"][0:3]
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01',
            ]
    run_name_data = (run_infixes, run_suffix)
    #multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {}, experiment_length=experiment_length)

def plot_8_3d_grid():
    print("plot_8_3d_grid")

    net_size = "8_3d"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:3]
    experiment_name = "action dense experiments"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01',
            'rand_deflect_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#(0,.05) at 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    utilizations = ["0.05","0.1", "0.15", "0.2"][3:4]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#(0,.05) at 20%
            "hop_ratio": {"ylim": (1,1.5)},

            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # combine reward
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    run_infixes = [
            'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            ]
    run_name_data = (run_infixes, run_suffix)
    multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {}, experiment_length=experiment_length)

def plot_3_3_hex():
    print("plot_3_3_hex")

    net_size = "3_3_hex"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    experiment_name = "sparse (3 3 hex)"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            'rand_forward_mice-elephant_p_0.01',
            'rand_deflect_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},
            "hop_ratio": {"ylim": (1,2)},

            "reward": {},
            "entropy": {"interval": 8},
            "action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_5_3_hex():
    print("plot_5_3_hex")

    net_size = "5_3_hex"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:2]
    experiment_name = "sparse topology"
    experiment_length = 1000
    run_infixes = [
            'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01',
            'rand_deflect_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},
            "hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            #{"field": "DropRate mean", "fig_type": "Packet Loss", "ylim": (0,.1), "file_suffix": "/results.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True},
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    utilizations = ["0.05","0.1", "0.15", "0.2"][2:4]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.15)},
            "hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # combine reward
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    run_infixes = [
            'mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01',
            ]
    run_name_data = (run_infixes, run_suffix)
    multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {}, experiment_length=experiment_length)

## samples ##

sample_single_util_plots = {
        "throughput": {},
        #multiple_run_throughput(experiment_name, utilization, net_size, run_name_data, experiment_length=experiment_length)
        "sent_rate": {},
        #multiple_run_sent_rate(experiment_name, utilization, net_size, run_name_data, experiment_length=experiment_length)

        "elephant_hop_ratio": {"ylim": (1,2)},
        #multiple_run_elephant_hop_ratio(experiment_name, utilization, net_size, run_name_data, y_lim=(1,1.5), experiment_length=experiment_length)
        "hop_ratio": {"ylim": (1,2)},
        #multiple_run_hop_ratio(experiment_name, utilization, net_size, run_name_data, y_lim=(1,1.5), experiment_length=experiment_length)

        "elephant_drop_rate": {"ylim": (0,.1)},
        #multiple_run_elephant_drop_rate(experiment_name, utilization, net_size, run_name_data, y_lim=(0,.1), experiment_length=experiment_length)
        "drop_rate": {"ylim": (0,.1)},
        #multiple_run_drop_rate(experiment_name, utilization, net_size, run_name_data, y_lim=(0,.1), experiment_length=experiment_length)

        "link_usage": {},
        #multiple_run_link_usage(experiment_name, utilization, net_size, run_name_data, experiment_length=experiment_length)
        "forwarding_links": {"ylim": (0,2)},
        #multiple_run_forwarding(experiment_name, utilization, net_size, run_name_data, y_lim=(0,2), experiment_length=experiment_length)

        "reward": {},
        #multiple_run_reward(experiment_name, utilization, net_size, run_name_data, experiment_length=experiment_length)
        "entropy": {"interval": 8},
        #multiple_run_all_entropy(experiment_name, utilization, net_size, run_name_data, entropy, experiment_length=experiment_length)
        "action_type_entropy": {"interval": 8},
        #multiple_run_deflection_entropy(experiment_name, utilization, net_size, run_name_data, entropy, experiment_length=experiment_length)

        "entropy_old": {},
        #multiple_run_all_entropy_old(experiment_name, utilization, net_size, run_name_data, experiment_length=experiment_length)
        "action_type_entropy_old": {},
        #multiple_run_deflection_entropy_old(experiment_name, utilization, net_size, run_name_data, experiment_length=experiment_length)

        # only if grid topology
        "switch_usage_heatmap": {"start_offset": 500, "v_range": (None,None)},
        #switch_link_usage_heatmap(experiment_name, utilization, net_size, run_name_data, 500, (None,None))#(0,.5))
        "switch_usage_plots": {},
        #switch_link_usage_grid_plots(experiment_name, utilization, net_size, run_name_data)
        }

sample_combine_util_plots = {
        "reward_across_utils": {},
        }

sample_across_util_plots = [
        {"field": "averageReward mean", "fig_type": "Rewards", "ylim": (.5,1), "file_suffix": "/switches.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":False},
        #plot_across_utils(experiment_name, utilizations, "averageReward mean", "Rewards", (0.5,1), "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)
        {"field": "DropRate mean", "fig_type": "Packet Loss", "ylim": (0,.1), "file_suffix": "/results.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True},
        #plot_across_utils(experiment_name, utilizations, "DropRate mean", "Packet Loss", (0,.1), "/results.csv", net_size, run_name_data, experiment_length=experiment_length)
        {"field": "elephant_DropRate mean", "fig_type": "Elephant Packet Loss", "ylim": (0,.1), "file_suffix": "/results.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True},
        #plot_across_utils(experiment_name, utilizations, "elephant_DropRate mean", "Elephant Packet Loss", (0,.1), "/results.csv", net_size, run_name_data, experiment_length=experiment_length)
        {"field": "HopRatio mean", "fig_type": "Hop Ratios", "ylim": (0,3), "file_suffix": "/results.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":False},
        #plot_across_utils(experiment_name, utilizations, "HopRatio mean", "Hop Ratios", (0,3), "/results.csv", net_size, run_name_data, experiment_length=experiment_length)

        {"field": "mean", "fig_type":"Link Usage", "ylim":(0,.5), "file_suffix":"/links.csv", "exact_match":False, "include_stdev":True, "include_min_max": True, "stdev_not_mean":False, "percent_data":True},
        #plot_across_utils(experiment_name, utilizations, "mean", "Link Usage", (0,.5), "/links.csv", net_size, run_name_data, exact_match=False, include_stdev=True, include_min_max=True, experiment_length=experiment_length)
        {"field": "mean", "fig_type":"Link Usage stdev", "ylim":(0,.2), "file_suffix":"/links.csv", "exact_match":False, "include_stdev":False, "include_min_max": False, "stdev_not_mean":True, "percent_data":False},
        #plot_across_utils(experiment_name, utilizations, "mean", "Link Usage stdev", (0,.2), "/links.csv", net_size, run_name_data, exact_match=False, include_stdev=False, include_min_max=False, stdev_not_mean=True, experiment_length=experiment_length)

        {"field": "averageForwardInterfacesRatio mean", "fig_type":"Forwarding Interfaces", "ylim":(0,2), "file_suffix":"/switches.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True},
        #plot_across_utils(experiment_name, utilizations, "averageForwardInterfacesRatio mean", "Forwarding Interfaces", (0, 2), "/switches.csv", net_size, run_name_data, experiment_length=experiment_length)
        ]

###########
## plots ##
###########

## sweep experiments ##

#plot_utilization_sweep()

#plot_algorithm_sweep_original()
#plot_algorithm_sweep_slide()
#plot_algorithm_sweep_D_LinUCB()
#plot_algorithm_sweep_best()

#plot_state_sweep_8x8()
#plot_state_sweep_16x16()

#plot_dlrl_poster()

#plot_limited_deflections()
#plot_limited_deflections_best()

#plot_ndd_param_sweep()
#plot_ndd_deflect_sweep()

#plot_random_deflect_deflect_count()

## evaluation experiments ##

plot_8_8_long()
#plot_update_freq_test()
#plot_prop_delay()
#plot_bandwidth()
#plot_bandwidth_all()
#plot_16x16()
#plot_8_3d_grid()
#plot_5_3_hex()

#cut results
#plot_limited_deflections_free_reign()
#plot_limited_deflections_forward_first()
#plot_limited_deflecitons_only_forward()
#plot_limited_deflections_best_cut()
#plot_compare_2_deflect()
#plot_compare_limitless_deflect()
#plot_3_3_hex()
