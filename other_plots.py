import csv
import statistics
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
import os
import math
import pandas as pd
import seaborn as sns

import sys

import code

RAND_FORWARD_NAME = 'rand forward'
RAND_DEFLECT_NAME = 'rand deflect'
NDD_NAME = 'NDD'
OUR_SOLN_NAME = 'Our Solution'
MBD_ORIGINAL = 'LinUCB'
MBD_SLIDE = 'LinUCB Slide'
MBD_D_LINUCB = 'D-LinUCB'

FIGSIZE = (16,9)#(16/1.5,9/1.5)
FIGSIZE_1 = (8,2)
FIGSIZE_2 = (8,4)
FIGSIZE_4 = (8,8)
TOGETHER_FIGSIZE = (8,4)
#legend adjusts for FIGSIZE_1
#1:.1
#2:.12
#3:.14
#4:
#legend adjusts for FIGSIZE_2
#1:.18
#2:.22
#3:.26
#4:.3

SEM_FIGSIZE = (12,4)
SEM_SQ_FIGSIZE = (8,4)
output_dir = "plots"
plot_format = "pdf"
NUM_RUNS = 30

# plot averages of multiple runs throughputs
def multiple_run_throughput(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "Throughput mean"
    stdev_field = "Throughput stdev"
    defaults = {"ylim": (0, 300000), "smooth": (False, None),
            "figsize": FIGSIZE, "title":f"{test_name} throughput", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Throughputs", params["ylim"], "/results.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_all_entropy(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    defaults = {"ylim": (0, 2), "interval": 8, "smooth": (False, None),
            "figsize": FIGSIZE, "title":f"{test_name} entropy", "stderr":False}
    params = {**defaults, **params}

    mean_field = f"allActionsEntropy_{params['interval']} mean"
    stdev_field = f"allActionsEntropy_{params['interval']} stdev"
    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            f"entropy {params['interval']}", params["ylim"], "/switches.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_deflection_entropy(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    defaults = {"ylim": (0, 1), "interval": 8, "smooth": (False, None),
            "figsize": FIGSIZE, "title":f"{test_name} deflection entropy", "stderr":False}
    params = {**defaults, **params}

    mean_field = f"deflectionEntropy_{params['interval']} mean"
    stdev_field = f"deflectionEntropy_{params['interval']} stdev"
    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            f"action type entropy {params['interval']}", params["ylim"], "/switches.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_all_entropy_old(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "allActionsEntropy mean"
    stdev_field = "allActionsEntropy stdev"
    defaults = {"ylim": (0, 2), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} entropy", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "entropy", params["ylim"], "/switches.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_deflection_entropy_old(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "deflectionEntropy mean"
    stdev_field = "deflectionEntropy stdev"
    defaults = {"ylim": (0, 1), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} action type entropy", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "action type entropy", params["ylim"], "/switches.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_reward(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "averageReward mean"
    stdev_field = "averageReward stdev"
    defaults = {"ylim": (0, 1), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} reward", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Rewards", params["ylim"], "/switches.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_sent_rate(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "SentRate mean"
    stdev_field = "SentRate stdev"
    defaults = {"ylim": (0, 300000), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} sent rate", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Sent Rates", params["ylim"], "/results.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

# plot averages of multiple runs throughputs
def multiple_run_elephant_hop_ratio(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    #TODO plot both regular and elephant
    print("TODO plot both regular and elephant")
    exit()
    mean_field = "elephant_HopRatio mean"
    stdev_field = "elephant_HopRatio stdev"
    defaults = {"ylim": (1, 2), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} elephant hop ratio", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Elephant Hop Ratios", params["ylim"], "/results.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_elephant_drop_rate(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    #TODO plot both regular and elephant
    #provide two mean fields?
    print("TODO plot both regular and elephant")
    exit()
    mean_field = "elephant_DropRate mean"
    stdev_field = "elephant_DropRate stdev"
    defaults = {"ylim": (0,.2), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} elephant packet loss", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Elephant Packet Loss", params["ylim"], "/results.csv", net_size, run_name_data,
            percent_data=True, experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_hop_ratio(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "HopRatio mean"
    stdev_field = "HopRatio stdev"
    defaults = {"ylim": (1, 2), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} hop ratio", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Hop Ratios", params["ylim"], "/results.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_drop_rate(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "DropRate mean"
    stdev_field = "DropRate stdev"
    defaults = {"ylim": (0, .2), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} packet loss", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Packet Loss", params["ylim"], "/results.csv", net_size, run_name_data,
            percent_data=True, experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_out_of_order(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "OutOfOrderRatio mean"
    stdev_field = "OutOfOrderRatio stdev"
    defaults = {"ylim": (0, .5), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} out of order packets", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Out of Order Ratio", params["ylim"], "/results.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_run_forwarding(ax, test_name, utilization, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "averageForwardInterfacesRatio mean"
    stdev_field = "averageForwardInterfacesRatio stdev"
    defaults = {"ylim": (0,4), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} forward action ratio", "stderr":False}
    params = {**defaults, **params}

    multiple_run_plot(ax, test_name, params["title"], utilization, mean_field, stdev_field,
            "Forwarding Interfaces", params["ylim"], "/switches.csv", net_size, run_name_data,
            experiment_length=experiment_length, smooth=params["smooth"],
            figsize=params["figsize"], stderr=params["stderr"])

def multiple_util_action_type_entropy_together(test_name, utilizations, net_size, run_name_data, params, experiment_length=1000):
    defaults = {"ylim": (0, 1), "interval": 8, "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} action type entropy", "stderr":False}
    params = {**defaults, **params}

    mean_field = f"deflectionEntropy_{params['interval']} mean"
    stdev_field = f"deflectionEntropy_{params['interval']} stdev"

    multiple_util_plot(test_name, params["title"], utilizations, mean_field, stdev_field,
            f"action type entropy {params['interval']}", f"Action Type Entropy {params['interval']}",
            params["ylim"], "/switches.csv", net_size, run_name_data, experiment_length=experiment_length,
            figsize=params["figsize"], smooth=params["smooth"], stderr=params["stderr"])

def multiple_util_all_entropy_together(test_name, utilizations, net_size, run_name_data, params, experiment_length=1000):
    defaults = {"ylim": (0, 2), "interval": 8, "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} entropy", "stderr":False}
    params = {**defaults, **params}

    mean_field = f"allActionsEntropy_{params['interval']} mean"
    stdev_field = f"allActionsEntropy_{params['interval']} stdev"

    multiple_util_plot(test_name, params["title"], utilizations, mean_field, stdev_field,
            f"entropy {params['interval']}", f"Entropy {params['interval']}", params["ylim"], "/switches.csv",
            net_size, run_name_data, experiment_length=experiment_length, figsize=params["figsize"], smooth=params["smooth"],
            stderr=params["stderr"])

def multiple_util_reward_together(test_name, utilizations, net_size, run_name_data, params, experiment_length=1000):
    mean_field = "averageReward mean"
    stdev_field = "averageReward stdev"
    defaults = {"ylim": (0.5, 1), "smooth": (False, None), "figsize": FIGSIZE,
            "title":f"{test_name} reward", "stderr":False}
    params = {**defaults, **params}

    multiple_util_plot(test_name, params["title"], utilizations, mean_field, stdev_field,
            "Rewards", "Reward", params["ylim"], "/switches.csv", net_size, run_name_data,
            experiment_length=experiment_length, figsize=params["figsize"], smooth=params["smooth"],
            stderr=params["stderr"])

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

#TODO RUN INFIX HERE
    run_infixes = sorted(run_infixes, key=lambda t: t[-1])

    data = {}

    for run_infix in run_infixes:
        run_name = get_run_name(net_size, utilization, run_infix, run_suffix)
        plot_name = run_infix[1]
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

        data[plot_name] = [np.array(means), np.array(maxs), np.array(mins), stdevs]

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

def multiple_run_plot(ax, test_filename, test_title, utilization, mean_field, stdev_field, fig_type, ylim, file_suffix, net_size, run_name_data, percent_data=False, experiment_length=1000, smooth=(False, None), figsize=FIGSIZE, stderr=False):
    subfig_name = f"{float(utilization)*100:.0f}% Utilization".title()

    directory = "results/"

    #run_prefix = run_name_data[0]
    run_infixes = run_name_data[0]
    run_suffix = run_name_data[1]

    run_infixes = sorted(run_infixes, key=lambda t: t[-1])

    data_names = []

    data = {}

    # for each run calculate averages
    for run_infix in run_infixes:
        run_name = get_run_name(net_size, utilization, run_infix, run_suffix)
        plot_name = run_infix[1]
        filename = directory + run_name + file_suffix

        mean_data = get_runs_data(filename, mean_field, experiment_length)
        if stderr:
            stdev_data = get_runs_data(filename, stdev_field, experiment_length)

        data[plot_name] = [None,None]
        data_names.append(plot_name)
        data[plot_name][0] = np.nanmean(mean_data, 1)
        if stderr:
            data[plot_name][1] = np.nanmean(stdev_data, 1) / np.sqrt(NUM_RUNS)

        #data[run_name] = np.array(means)

    # plot data
    ax.set_title(subfig_name)

    #for run in sorted(data.keys()):
    for run in data_names:
        run_data = data[run]
        if smooth[0] != False:
            run_data[0] = pd.Series(run_data[0]).rolling(window=smooth[1], min_periods=1, center=True).mean()
            if stderr:
                run_data[1] = pd.Series(run_data[1]).rolling(window=smooth[1], min_periods=1, center=True).mean()
        #why does this not always work?
        mask = ~np.isnan(run_data[0])
        x = np.arange(len(run_data[0]))[mask]
        y = run_data[0][mask]
        ax.plot(x, y, label=run, alpha=0.8, linewidth=1)
        if stderr:
            error = run_data[1][mask]
            ax.fill_between(x, y - error, y + error, alpha=0.2)

    ax.set_ylim(bottom=ylim[0], top=ylim[1])
    ax.set_xlim(left=-1, right=experiment_length+1)
    #ax.set_xmargin(0)
    ax.grid()

    if percent_data:
        y_ticks = ax.get_yticks()
        ax.set_yticks(y_ticks.tolist())
        ax.set_yticklabels([f'{t * 100:.1f}%' for t in y_ticks])

    if smooth[0] != False:
        ax.set_ylabel(f"{fig_type} (Smoothed)".title())
    else:
        ax.set_ylabel(fig_type.title())
    ax.set_xlabel("Time (s)")


#TODO new! this should work
def multiple_util_plot(test_filename, test_title, utilizations, mean_field, stdev_field, fig_type, ylabel, ylim, file_suffix, net_size, run_name_data, percent_data=False, experiment_length=1000, smooth=(False,None), figsize=FIGSIZE, stderr=False):
    #fig_name = f"{test_name} {net_size} Bursty {fig_type}"
    #output_name = fig_name.replace(' ', '_')
    output_name = f"{test_filename} {net_size} Bursty {fig_type}".replace(' ','_')
    fig_name = f"{test_title}".title()

    directory = "results/"

    #run_prefix = run_name_data[0]
    run_infixes = run_name_data[0]
    run_suffix = run_name_data[1]

#TODO RUN INFIX HERE
    run_infixes = sorted(run_infixes, key=lambda t: t[-1])

    data = {}

    # for each run calculate averages
    for utilization in utilizations:
        for run_infix in run_infixes:
            run_name = get_run_name(net_size, utilization, run_infix, run_suffix)
            plot_name = f'{run_infix[1]} util: {int(float(utilization)*100):2d}%'
            filename = directory + run_name + file_suffix

            mean_data = get_runs_data(filename, mean_field, experiment_length)
            if stderr:
                stdev_data = get_runs_data(filename, stdev_field, experiment_length)

            data[plot_name] = [None,None]
            data[plot_name][0] = np.nanmean(mean_data, 1)
            if stderr:
                data[plot_name][1] = np.nanmean(stdev_data, 1) / np.sqrt(NUM_RUNS)

    # plot data
    plt.figure(figsize=figsize)
    plt.title(fig_name)

    for run in sorted(data.keys()):
        run_data = data[run]
        if smooth[0] != False:
            run_data[0] = pd.Series(run_data[0]).rolling(window=smooth[1], min_periods=1, center=True).mean()
            if stderr:
                run_data[1] = pd.Series(run_data[1]).rolling(window=smooth[1], min_periods=1, center=True).mean()
        #why does this not always work?
        mask = ~np.isnan(run_data[0])
        x = np.arange(len(run_data[0]))[mask]
        y = run_data[0][mask]
        plt.plot(x, y, label=run, alpha=0.8, linewidth=1)
        if stderr:
            error = run_data[1][mask]
            plt.fill_between(x, y - error, y + error, alpha=0.2)

    # new legend
    ax = plt.gca()  # Get current Axes
    handles, labels = ax.get_legend_handles_labels()  # Get current legend handles and labels

    # Create custom patches for the legend
    patches = [Patch(color=handle.get_color(), label=label) for handle, label in zip(handles, labels)]

    # Add custom legend with patches
    plt.legend(handles=patches, loc='center right')
    # new legent

    plt.ylim(bottom=ylim[0], top=ylim[1])
    plt.grid()

    if percent_data:
        #ax = plt.gca()
        y_ticks = ax.get_yticks()
        ax.set_yticklabels([f'{t * 100}%' for t in y_ticks])

    if smooth[0] != False:
        plt.ylabel(f"{ylabel} (Smoothed)".title())
    else:
        plt.ylabel(ylabel.title())
    plt.xlabel("Time (s)")

    try:
        filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
        plt.savefig(filepath, format=plot_format)
        plt.close()
        print("wrote to", filepath)
    except Exception as e:
        print("failed to plot", output_name, "exception", str(e))

def multiple_run_individual_grid(test_name, utilization, net_size, run_name_data, filename, metric, fig_type, ylim):
#TODO RUN INFIX HERE
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
#TODO RUN INFIX HERE
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

def plot_across_utils(test_name, plot_name, utilizations, net_size, run_name_data, experiment_length, params):
    field = params["field"]
    fig_type = params["fig_type"]
    ylabel = params["ylabel"]
    ylim = params["ylim"]
    file_suffix = params["file_suffix"]
    exact_match = params["exact_match"]
    include_stdev = params["include_stdev"]
    include_min_max = params["include_min_max"]
    stdev_not_mean = params["stdev_not_mean"]
    percent_data = params["percent_data"]
    figsize = params["figsize"]

    fig_name = f"{test_name} {net_size} Bursty {fig_type}"
    output_name = fig_name.replace(' ', '_')

    utilizations = sorted(utilizations)

    directory = "results/"

    run_infixes = run_name_data[0]
    run_suffix = run_name_data[1]

#TODO RUN INFIX HERE
    run_infixes = sorted(run_infixes, key=lambda t: t[-1])

    means = {}
    stdevs = {}
    mins = {}
    maxs = {}

    # for agent / run
    for run_infix in run_infixes:
        # for util
        run_name = get_no_util_run_name(net_size, run_infix, run_suffix)
        run_label = run_infix[1]
        means[run_label] = []
        stdevs[run_label] = []
        mins[run_label] = []
        maxs[run_label] = []

        for util in utilizations:
            file_run_name = get_run_name(net_size, util, run_infix, run_suffix)
            filename = directory + file_run_name + file_suffix

            run_data = get_runs_data(filename, field, experiment_length, exact_match=exact_match)
            # only look at second half
            run_data = run_data[run_data.shape[0]//2:]
            means[run_label].append(np.nanmean(run_data))
            stdevs[run_label].append(np.nanstd(run_data))

            if len(run_data.shape) > 1:
                mins[run_label].append(np.nanmin(run_data))
                maxs[run_label].append(np.nanmax(run_data))
            else:
                mins[run_label].append(run_data.min())
                maxs[run_label].append(run_data.max())

        means[run_label] = np.array(means[run_label])
        stdevs[run_label] = np.array(stdevs[run_label])
        mins[run_label] = np.array(mins[run_label])
        maxs[run_label] = np.array(maxs[run_label])

    plt.figure(figsize=figsize)

    loc = np.arange(len(utilizations))
    width = 1 / (len(means.keys())+1)

    for i, run in enumerate(sorted(means.keys())):
        #plt.plot(utilizations, means[run], label=run, alpha=.9)
        yerr_min_max = None
        yerr_stdev = None
        if include_stdev:
            yerr_stdev = stdevs[run]
        if include_min_max:
            yerr_min_max = np.stack([means[run] - mins[run], maxs[run] - means[run]])

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


    plt.title(plot_name)
    plt.ylabel(ylabel)
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
    return prefix + infix[0] + suffix

def get_run_name(net_size, utilization, infix, suffix):
    prefix = f"{net_size}_"
    return prefix + infix[0] + suffix + "_u_" + utilization

def get_single_util_func(name):
    mappings = {
            "hop_ratio": (multiple_run_hop_ratio, "hop-ratio"),
            "elephant_hop_ratio": (multiple_run_elephant_hop_ratio, "elephant hop-ratio"),
            "drop_rate": (multiple_run_drop_rate, "packet loss"),
            "elephant_drop_rate": (multiple_run_elephant_drop_rate, "elephant packet loss"),

            "throughput": (multiple_run_throughput, "throughput"),
            "sent_rate": (multiple_run_sent_rate, "sent rate"),

            "link_usage": (multiple_run_link_usage, "link usage"),
            "forwarding_links": (multiple_run_forwarding, "forwarding links" ),

            "reward": (multiple_run_reward, "reward"),
            "entropy": (multiple_run_all_entropy, "entropy"),
            "action_type_entropy": (multiple_run_deflection_entropy, "deflection entropy"),

            "entropy_old": (multiple_run_all_entropy_old, "entropy"),
            "action_type_entropy_old": (multiple_run_deflection_entropy_old, "deflection entropy"),

            "out_of_order": (multiple_run_out_of_order, "out of order packets"), # unsupported

            "switch_usage_heatmap": (switch_link_usage_heatmap, "switch usage"), # 2d grid only
            "switch_usage_plots": (switch_link_usage_grid_plots, "switch usage"), # 2d grid only
            }

    if name in mappings:
        return mappings[name]
    print("############################################")
    print(f"  Plot '{name}' not found")
    print("############################################")

#legend_cols = 3
#legend_adjust=.1,.25
def plot_data(experiment_name, plot_name, topology, utils, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols):
    #single_util_plots = {name: {params}}
    run_name_data = (run_infixes, run_suffix)
    for name, params_list in single_util_plots.items():
        fig, axs = plt.subplots(nrows=len(utils), figsize=figsize)#, constrained_layout=True)

        func, plot_type = get_single_util_func(name)

        for i, util in enumerate(utils):
            ax = axs
            if len(utils) != 1:
                ax = axs[i]
            params = params_list[i]
            func(ax, experiment_name, util, topology, run_name_data, params, experiment_length=experiment_length)
            if i != len(utils) - 1:
                ax.set_xlabel("")

        # Create custom patches for the legend
        ax = plt.gca()  # Get current Axes
        handles, labels = ax.get_legend_handles_labels()  # Get current legend handles and labels
        patches = [Patch(color=handle.get_color(), label=label) for handle, label in zip(handles, labels)]

        output_name = f"{experiment_name} {topology} Bursty {name}".replace(' ','_')
        fig_name = f"{plot_name} {plot_type.title()}"
        fig.suptitle(fig_name)
        fig.tight_layout()

        fig.legend(handles=patches, loc='lower center', ncols=legend_cols)#, bbox_to_anchor=(0.5, -0.05))
        fig.subplots_adjust(bottom=legend_adjust)
        fig_width, fig_height = fig.get_size_inches()
        fig.set_size_inches(fig_width, fig_height + figsize[1]*legend_adjust)

        try:
            filepath = os.path.join(output_dir, "{}.{}".format(output_name, plot_format))
            plt.savefig(filepath, format=plot_format)
            plt.close()
            print("wrote to", filepath)
        except Exception as e:
            print("failed to plot", output_name, "exception", str(e))

    #across_util_plots = [{params}, ...]
    for params in across_util_plots:
        plot_across_utils(experiment_name, plot_name, utils, topology, run_name_data, experiment_length, params)
        #TODO change field, fig_type/name, file_suffix, entropy, y_lim, etc to be in a dict of paramters

def plot_utilization_sweep():
    print("plot_utilization_sweep")

    figsize = TOGETHER_FIGSIZE
    legend_adjust = .22
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2","0.25"]
    experiment_name = "Utilization Sweep"
    plot_name = "Utilization Evaluation"
    experiment_length = 1000
    run_infixes = [
            ("rand_forward_mice-elephant_p_0.01", RAND_FORWARD_NAME, 1),
            ("rand_deflect_mice-elephant_p_0.01", RAND_DEFLECT_NAME, 2),
            #"rand_deflect_sd_2_mice-elephant_p_0.01",
            ]
    run_suffix = ""
    single_util_plots = {
            #"drop_rate": {"ylim": (0,.3)},#or (0,.05), (0,.1) at 10%, 20%
            #"hop_ratio": {"ylim": (1,1.5)},
            }
    across_util_plots = [
            {"field": "DropRate mean", "fig_type": "Packet Loss", "ylabel": "Packet Loss", "ylim": (0,.3), "file_suffix": "/results.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True, "figsize": figsize},
            {"field": "mean", "fig_type":"Link Usage", "ylabel": "Link Usage", "ylim":(0,.5), "file_suffix":"/links.csv", "exact_match":False, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True, "figsize": figsize},
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_algorithm_sweep_original():
    print("plot_algorithm_sweep_original")

    figsize = FIGSIZE_2
    legend_adjust = .22
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep Original"
    plot_name = "Original"
    experiment_length = 1000
    run_infixes = [
            ("mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", "1.0, 1.0", 1),

            ("mbd_original_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01", "0.37, 0.96", 2),
            ("mbd_original_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01", "0.7, 0.97", 3),
            ("mbd_original_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01", "0.86, 0.66", 4),
            ("mbd_original_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01", "0.95, 0.62", 5),
            ("mbd_original_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01", "1.2, 0.9", 6),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)}]*2,#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*4,

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_algorithm_sweep_slide():
    print("plot_algorithm_sweep_slide")

    figsize = FIGSIZE_2
    legend_adjust = .22
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep Slide"
    plot_name = "Slide"
    experiment_length = 1000
    run_infixes = [
            ("mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", "1.0, 1.0", 1),

            ("mbd_slide_s_[1_hop_shortest,section]_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01", "0.37, 0.96", 2),
            ("mbd_slide_s_[1_hop_shortest,section]_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01", "0.7, 0.97", 3),
            ("mbd_slide_s_[1_hop_shortest,section]_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01", "0.86, 0.66", 4),
            ("mbd_slide_s_[1_hop_shortest,section]_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01", "0.95 0.62", 5),
            ("mbd_slide_s_[1_hop_shortest,section]_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01", "1.2, 0.9", 6),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)}]*2,#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*4,

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_algorithm_sweep_D_LinUCB():
    print("plot_algorithm_sweep_D_LinUCB")

    figsize = FIGSIZE_2
    legend_adjust = .3
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep D-LinUCB"
    plot_name = "D-LinUCB"
    experiment_length = 1000
    run_infixes = [
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.99999, $\lambda$ 1.0, $\delta$ 1.0", 1),

            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.7_d_0.97_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.999, $\lambda$ 0.7, $\delta$ 0.97", 2),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.9999_r_1.2_d_0.9_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.9999, $\lambda$ 1.2, $\delta$ 0.9", 3),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.86_d_0.66_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.999, $\lambda$ 0.86, $\delta$ 0.66", 4),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.37_d_0.96_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.999, $\lambda$ 0.37, $\delta$ 0.96", 5),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_0.95_d_0.62_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.9999, $\lambda$ 0.95, $\delta$ 0.62", 6),

            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.26_d_0.99_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.99999, $\lambda$ 1.26, $\delta$ 0.99", 7),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.9999_r_1.96_d_0.99_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.9999, $\lambda$ 1.96, $\delta$ 0.99", 8),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.57_d_0.71_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.999, $\lambda$ 0.57, $\delta$ 0.71", 9),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.76_d_0.78_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.99999, $\lambda$ 1.76, $\delta$ 0.78", 10),
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.999_r_0.93_d_0.54_sd_-1_ei_5_mice-elephant_p_0.01", r"$\gamma$ 0.999, $\lambda$ 0.93, $\delta$ 0.54", 11),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.2)}]*2,#or (0,.2), (0,>.2) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*4,

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_algorithm_sweep_best():
    print("plot_algorithm_sweep_best")

    # TODO currently best D-LinUCB has a run that crashes
    figsize = FIGSIZE_2
    legend_adjust = .18
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Algorithm Sweep Best"
    plot_name = "Algorithm Evaluation Best"
    experiment_length = 1000
    run_infixes = [
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", MBD_D_LINUCB+" 0.99999, 1.0, 1.0", 1),
            ("mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", MBD_ORIGINAL+" 1.0, 1.0", 2),
            ("mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", MBD_SLIDE+" 1.0, 1.0", 3),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.1)}]*2,#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*2,

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_state_sweep_8x8():
    print("plot_state_sweep_8x8")

    figsize = FIGSIZE_2
    legend_adjust = .22
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "State Sweep"
    plot_name = "State Evaluation (8x8)"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop', 1),
            ('mbd_original_s_[1-2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-2-hop', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop + 2-hop', 3),
            ('mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop + sector', 4),
            ('mbd_original_s_[1-2_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-2-hop + sector', 5),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)}]*2,#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*2,

            "reward": [{}]*2,
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_state_sweep_16x16():
    print("plot_state_sweep_16x16")

    figsize = FIGSIZE_2
    legend_adjust = .22
    legend_cols = 3
    net_size = "16x16"
    utilizations = ["0.2"]
    experiment_name = "State Sweep"
    plot_name = "State Evaluation (16x16)"
    experiment_length = 2000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop', 1),
            ('mbd_original_s_[1-2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-2-hop', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop + 2-hop', 3),
            ('mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop + sector', 4),
            ('mbd_original_s_[1-2_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-2-hop + sector', 5),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.2)}],#or (0,.1), (0,.2) at 10%, 20%
            "hop_ratio": [{"ylim": (1,2)}],

            "reward": [{}],
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_dlrl_poster():
    print("plot_dlrl_poster")

    net_size = "16x16"
    utilizations = ["0.05"]
    experiment_name = "DLRL poster"
    experiment_length = 2000
    run_infixes = [
            ('NDD_Q-learning_a_0.5_e_0.05_g_0.99_mu_mice-elephant_p_0.01', NDD_NAME, 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', 'Our Solution', 2),
            ('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01', 'forward first, no deflection limit', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', 'free reign, no deflection limit', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_forward_first_mice-elephant_p_0.01', 'forward first, deflection limit: 2', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01', 'free reign, deflection limit: 2', 4),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_forward_first_mice-elephant_p_0.01', 'forward first, deflection limit: 4', 5),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_mice-elephant_p_0.01', 'free reign, deflection limit: 4', 6),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_forward_first_mice-elephant_p_0.01', 'forward first, deflection limit: 6', 7),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_mice-elephant_p_0.01', 'free reign, deflection limit: 6', 8),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_forward_first_mice-elephant_p_0.01', 'forward first, deflection limit: 8', 9),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_mice-elephant_p_0.01', 'free reign, deflection limit: 8', 10),

            #'NDD_Q-learning_a_0.05_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.05), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', 'no deflection limit', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01', 'deflection limit: 2', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_mice-elephant_p_0.01', 'deflection limit: 4', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_mice-elephant_p_0.01', 'deflection limit: 6', 4),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_mice-elephant_p_0.01', 'deflection limit: 8', 5),

            #'NDD_Q-learning_a_0.05_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.05), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01', 'no deflection limit', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_forward_first_mice-elephant_p_0.01', 'deflection limit: 2', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_forward_first_mice-elephant_p_0.01', 'deflection limit: 4', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_forward_first_mice-elephant_p_0.01', 'deflection limit: 6', 4),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_forward_first_mice-elephant_p_0.01', 'deflection limit: 8', 5),

            #'NDD_Q-learning_a_0.05_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.05), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01', OUR_SOLN_NAME+' forward only', 1),
            #'mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01',

            #'NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01',
            ('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 2),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.25)},#or (0,.15), (0,.25) at 10%, 20%
            #"hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01', 'forward first, no deflection limit', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', 'free reign no deflection limit', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01', 'only forward', 3)

            #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_forward_first_mice-elephant_p_0.01',
            #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_mice-elephant_p_0.01',
            #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_forward_first_mice-elephant_p_0.01',
            #'mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_mice-elephant_p_0.01',
            ]
    run_suffix = ""
    single_util_plots = {
            #"drop_rate": {"ylim": (0,.05)},#or (0,.02), (0,.05) at 10%, 20%
            #"hop_ratio": {"ylim": (1,1.5)},

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
            ('c_mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', 'free reign no deflection limit', 1),
            ('c_mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01', 'forward first no deflection limit', 2),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2)},#or (0,.05), (0,.2) at 10%, 20%
            "hop_ratio": {"ylim": (1,2)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_prop_delay():
    print("plot_prop_delay")

    figsize = FIGSIZE_4
    legend_adjust = .12
    legend_cols = 3
    net_size = "8x8"
    #utilizations = ["0.05","0.1","0.15","0.2"][0:4]
    experiment_name = "Propagation delay experiments"
    plot_name = "Propagation Delay"
    experiment_length = 1000
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2)},#or (0,.05), (0,.2) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},
            }
    across_util_plots = [
            ]

    utilizations = ["0.05","0.1","0.15","0.2"]
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.001', OUR_SOLN_NAME+' 1ms', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME+' 10ms', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.1', OUR_SOLN_NAME+' 100ms', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_1.0', OUR_SOLN_NAME+' 1s', 4),
            ]
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)},{"ylim": (0,.05)},{"ylim": (0,.2)},{"ylim": (0,.2)}],#or (0,.05), (0,.2) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*4,
            }

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_prop_delay_baselines():
    print("plot_prop_delay_baselines")

    figsize = FIGSIZE_4
    legend_adjust = .14
    legend_cols = 3
    net_size = "8x8"
    #utilizations = ["0.05","0.1","0.15","0.2"][0:4]
    experiment_name = "Propagation delay experiments with NDD"
    plot_name = "Propagation Delay with Baselines"
    experiment_length = 1000
    run_suffix = ""

    utilizations = ["0.05","0.1","0.15","0.2"]
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.001', OUR_SOLN_NAME+' 1ms', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME+' 10ms', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.1', OUR_SOLN_NAME+' 100ms', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_1.0', OUR_SOLN_NAME+' 1s', 4),

            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.001', NDD_NAME+' 1ms', 5),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME+' 10ms', 6),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.1', NDD_NAME+' 100ms', 7),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_1.0', NDD_NAME+' 1s', 8),
            ]
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)},{"ylim": (0,.05)},{"ylim": (0,.2)},{"ylim": (0,.2)}],#or (0,.05), (0,.2) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*4,
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_bandwidth():
    print("plot_bandwidth")

    figsize = FIGSIZE_4
    legend_adjust = .1
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2"]
    experiment_name = "bandwidth experiments"
    plot_name = "Bandwidth Evaluation"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_0.5', OUR_SOLN_NAME+' 0.5x bandwidth', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME+' 1x bandwidth', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_2', OUR_SOLN_NAME+' 2x bandwidth', 3),

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
            "drop_rate": [{"ylim": (0,.02)},{"ylim": (0,.02)},{"ylim": (0,.05)},{"ylim": (0,.05)},],#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*4,

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_bandwidth_all():
    print("plot_bandwidth_all")

    figsize = FIGSIZE_4
    legend_adjust = .12
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2"]
    experiment_name = "bandwidth experiments all"
    plot_name = "Bandwidth Evaluation Baselines"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_0.5', OUR_SOLN_NAME+' 0.5x bandwidth', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME+' 1x bandwidth', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01_b_2', OUR_SOLN_NAME+' 2x bandwidth', 3),

            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01_b_0.5', NDD_NAME+' 0.5x bandwidth', 4),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME+' 1x bandwidth', 5),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01_b_2', NDD_NAME+' 2x bandwidth', 6),

            #'rand_forward_mice-elephant_p_0.01_b_0.5',
            #'rand_forward_mice-elephant_p_0.01',
            #'rand_forward_mice-elephant_p_0.01_b_2',

            #'rand_deflect_mice-elephant_p_0.01_b_0.5',
            #'rand_deflect_mice-elephant_p_0.01',
            #'rand_deflect_mice-elephant_p_0.01_b_2',
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.02)},{"ylim": (0,.02)},{"ylim": (0,.05)},{"ylim": (0,.04)},],#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": [{"ylim": (1,1.5)}]*4,
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_update_freq_test():
    print("plot_update_freq_test")

    together_figsize = TOGETHER_FIGSIZE
    figsize = FIGSIZE_4
    legend_adjust = .12
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2"]
    experiment_name = "Update Frequency test"
    plot_name = "Update Frequency Evaluation"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', 'Update frequency: 1', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_2_mice-elephant_p_0.01', 'Update frequency: 2', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_4_mice-elephant_p_0.01', 'Update frequency: 4', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_8_mice-elephant_p_0.01', 'Update frequency: 8', 4),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_16_mice-elephant_p_0.01', 'Update frequency: 16', 5),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_32_mice-elephant_p_0.01', 'Update frequency: 32', 6),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.02)},{"ylim": (0,.02)},{"ylim": (0,.05)},{"ylim": (0,.05)}],
            "hop_ratio": [{"ylim": (1,1.5)}]*4,
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

    utilizations = ["0.05","0.1","0.15","0.2"]
    run_name_data = (run_infixes, run_suffix)

    #multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {"figsize": together_figsize}, experiment_length=experiment_length)
    #multiple_util_all_entropy_together(experiment_name, utilizations, net_size, run_name_data, {"interval": 8,"figsize": together_figsize}, experiment_length=experiment_length)
    #multiple_util_action_type_entropy_together(experiment_name, utilizations, net_size, run_name_data, {"interval": 8,"figsize": together_figsize}, experiment_length=experiment_length)

def plot_random_deflect_deflect_count():
    print("plot_random_deflect_deflect_count")

    figsize = FIGSIZE_2
    legend_adjust = .22
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"]
    experiment_name = "Random Deflect Deflect Count Test"
    plot_name = "Random Deflect Deflection Count Evaluation"
    experiment_length = 1000
    run_infixes = [
            ('rand_deflect_mice-elephant_p_0.01', 'no deflection limit', 1),
            ('rand_deflect_sd_2_mice-elephant_p_0.01', 'deflection limit: 2', 2),
            ('rand_deflect_sd_4_mice-elephant_p_0.01', 'deflection limit: 4', 3),
            ('rand_deflect_sd_6_mice-elephant_p_0.01', 'deflection limit: 6', 4),
            ('rand_deflect_sd_8_mice-elephant_p_0.01', 'deflection limit: 8', 5),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)}, {"ylim": (0,.15)}],
            "hop_ratio": [{"ylim": (1,1.5)}]*2,

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_compare_2_deflect():
    print("plot_compare_2_deflect")

    net_size = "8x8"
    utilizations = ["0.1","0.2"][1:2]
    experiment_name = "Compare 2 deflect algorithms"
    experiment_length = 1000
    run_infixes = [
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01', NDD_NAME+' deflection limit: 2', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01', OUR_SOLN_NAME+' free reign, deflection limit: 2', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_forward_first_mice-elephant_p_0.01', OUR_SOLN_NAME+' forward first, deflection limit: 2', 3),
            ('rand_deflect_sd_2_mice-elephant_p_0.01', RAND_DEFLECT_NAME+' deflection limit: 2', 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.15)},#or (0,.1), (0,.15) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', OUR_SOLN_NAME+' free reign, no deflection limit', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01', OUR_SOLN_NAME+' forward first, no deflection limit', 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME+' no deflection limit', 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME+' no deflection limit', 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#or (0,.02), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_ndd_param_sweep():
    print("plot_ndd_param_sweep")

    figsize = FIGSIZE_2
    legend_adjust = .3
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"][0:2]
    experiment_name = "NDD Param Sweep"
    plot_name = "NDD Param Evaluation"
    experiment_length = 1000
    run_infixes = [
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01', NDD_NAME+' 0.1, 0.05 0.99', 1),
            ('NDD_Q-learning_a_0.006_e_0.023_g_0.685_mu_mice-elephant_p_0.01', NDD_NAME+' 0.006, 0.023, 0.685', 2),
            ('NDD_Q-learning_a_0.081_e_0.046_g_0.999_mu_mice-elephant_p_0.01', NDD_NAME+' 0.081, 0.046, 0.999', 3),
            ('NDD_Q-learning_a_0.002_e_0.059_g_0.996_mu_mice-elephant_p_0.01', NDD_NAME+' 0.002, 0.059, 0.996', 4),
            ('NDD_Q-learning_a_0.013_e_0.066_g_0.958_mu_mice-elephant_p_0.01', NDD_NAME+' 0.013, 0.066, 0.958', 5),
            ('NDD_Q-learning_a_0.05_e_0.012_g_0.993_mu_mice-elephant_p_0.01', NDD_NAME+' 0.05, 0.012, 0.993', 6),
            ('NDD_Q-learning_a_0.009_e_0.029_g_0.131_mu_mice-elephant_p_0.01', NDD_NAME+' 0.009, 0.029, 0.131', 7),
            ('NDD_Q-learning_a_0.021_e_0.058_g_0.997_mu_mice-elephant_p_0.01', NDD_NAME+' 0.021, 0.058, 0.997', 8),
            ('NDD_Q-learning_a_0.002_e_0.047_g_0.997_mu_mice-elephant_p_0.01', NDD_NAME+' 0.002, 0.047, 0.997', 9),
            ('NDD_Q-learning_a_0.096_e_0.01_g_0.748_mu_mice-elephant_p_0.01', NDD_NAME+' 0.096, 0.01, 0.748', 10),
            ('NDD_Q-learning_a_0.088_e_0.015_g_0.923_mu_mice-elephant_p_0.01', NDD_NAME+' 0.088, 0.015, 0.923', 11),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)}, {"ylim": (0,.15)}],
            "hop_ratio": [{"ylim": (1,1.5)}]*2,

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_ndd_deflect_sweep():
    print("plot_ndd_deflect_sweep")

    figsize = FIGSIZE_2
    legend_adjust = .22
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.1","0.2"][0:2]
    experiment_name = "NDD Deflect Sweep"
    plot_name = "NDD Deflection Count Evaluation"
    experiment_length = 1000
    run_infixes = [
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_mice-elephant_p_0.01', NDD_NAME+' deflection limit: 2', 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_4_mice-elephant_p_0.01', NDD_NAME+' deflection limit: 4', 2),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_6_mice-elephant_p_0.01', NDD_NAME+' deflection limit: 6', 3),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME+' deflection limit: 8', 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.05)}, {"ylim": (0,.15)}],
            "hop_ratio": [{"ylim": (1,1.5)}]*2,

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

def plot_8_8_long():
    print("plot_8_8_long")

    together_figsize = TOGETHER_FIGSIZE
    figsize = FIGSIZE_4
    legend_adjust = .1
    legend_cols = 3
    net_size = "8x8"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    experiment_name = "long run"
    plot_name = "Long Run"
    experiment_length = 5000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": [{"ylim": (0,.02)}, {"ylim": (0,.02)}, {"ylim": (0,.04)}, {"ylim": (0,.04)}],
            #(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": [{"ylim": (1,1.5)}]*4,
            #"elephant_drop_rate": {"ylim": (0,.04)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            #"elephant_hop_ratio": {"ylim": (1,1.5)},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, plot_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots, figsize, legend_adjust, legend_cols)

    # combine reward
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ]
    run_name_data = (run_infixes, run_suffix)
    multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {"figsize": together_figsize}, experiment_length=experiment_length)
    multiple_util_all_entropy_together(experiment_name, utilizations, net_size, run_name_data, {"figsize": together_figsize, "interval": 8}, experiment_length=experiment_length)
    multiple_util_action_type_entropy_together(experiment_name, utilizations, net_size, run_name_data, {"figsize": together_figsize, "interval": 8}, experiment_length=experiment_length)

def plot_16x16():
    print("plot_16x16")

    net_size = "16x16"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    experiment_name = "large network"
    experiment_length = 2000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
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
    utilizations = ["0.05","0.1", "0.15", "0.2"][2:4]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5)},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # combine reward
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ]
    run_name_data = (run_infixes, run_suffix)
    multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {}, experiment_length=experiment_length)

def plot_8_3d_grid():
    print("plot_8_3d_grid")

    net_size = "8_3d"
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:3]
    experiment_name = "action dense experiments"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02)},#(0,.05) at 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    utilizations = ["0.05","0.1", "0.15", "0.2"][3:4]
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05)},#(0,.05) at 20%
            "hop_ratio": {"ylim": (1,1.5)},

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    # combine reward
    utilizations = ["0.05","0.1", "0.15", "0.2"][0:4]
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            ('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1)},
            "hop_ratio": {"ylim": (1,2)},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
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
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
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
            ('mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ]
    run_name_data = (run_infixes, run_suffix)
    multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {}, experiment_length=experiment_length)

def plot_sem_state_sweep():
    print("plot_sem_state_sweep")

    net_size = "16x16"
    utilizations = ["0.2"]
    experiment_name = "Context Sweep Results"
    experiment_length = 2000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop', 1),
            ('mbd_original_s_[1-2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-2-hop', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop + 2-hop', 3),
            ('mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-hop + sector', 4),
            ('mbd_original_s_[1-2_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', '1-2-hop + sector', 5),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.2), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title": "Context Selection Packet Loss", "stderr":True},#or (0,.1), (0,.2) at 10%, 20%
            "hop_ratio": {"ylim": (1,2), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title": "Context Selection Hop Ratio", "stderr":True},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_limited_deflections_deflect_count():
    print("plot_sem_limited_deflections_count")

    net_size = "8x8"
    utilizations = ["0.1"]
    experiment_name = "Deflection Count Results"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', 'no deflection limit', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_2_ei_5_mice-elephant_p_0.01', 'deflection limit: 2', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_4_ei_5_mice-elephant_p_0.01', 'deflection limit: 4', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_6_ei_5_mice-elephant_p_0.01', 'deflection limit: 6', 4),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_8_ei_5_mice-elephant_p_0.01', 'deflection limit: 8', 5),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Deflection Count Packet Loss", "stderr":True},#or (0,.05), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Deflection Count Hop Ratio", "stderr":True},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_limited_deflections_type():
    print("plot_sem_limited_deflections_type")

    # TODO do something about only forward
    net_size = "8x8"
    utilizations = ["0.1"]
    experiment_name = "Action Limit Results"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_forward_first_mice-elephant_p_0.01', 'forward first; no deflection limit', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', 'free reign; no deflection limit', 2),
            #('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_only_forward_mice-elephant_p_0.01', 'only forward', 3)
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Action Limit Packet Loss", "stderr":True},#or (0,.02), (0,.05) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Action Limit Hop Ratio", "stderr":True},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_comparison():
    print("plot_sem_comparison")

    net_size = "8x8"
    utilizations = ["0.1"]
    experiment_name = "baseline comparison"
    experiment_length = 5000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02), "smooth":(True, 20), "figsize":SEM_FIGSIZE, "title":"Baseline Comparison Packet Loss", "stderr":True},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 20), "figsize":SEM_FIGSIZE, "title":"Baseline Comparison Hop Ratio", "stderr":True},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_utils_comparison():
    print("plot_sem_utils_comparison")

    net_size = "8x8"
    utilizations = ["0.05","0.1","0.15","0.2"]
    experiment_name = "baseline comparison utils"
    experiment_length = 2000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05), "smooth":(True, 5), "figsize":SEM_SQ_FIGSIZE, "title":"Baseline Comparison Packet Loss", "stderr":True},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 20), "figsize":SEM_SQ_FIGSIZE, "title":"Baseline Comparison Hop Ratio", "stderr":True},
            }
    across_util_plots = [
            ]

    #plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

    utilizations = ["0.05","0.1","0.15","0.2"]
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ]
    experiment_length = 5000
    run_name_data = (run_infixes, run_suffix)
    multiple_util_reward_together(experiment_name, utilizations, net_size, run_name_data, {"ylim":(0.5,1), "smooth":(True,5), "figsize":SEM_FIGSIZE, "stderr":True}, experiment_length=experiment_length)
    #multiple_util_all_entropy_together(experiment_name, utilizations, net_size, run_name_data, {"interval": 8}, experiment_length=experiment_length, "stderr":True)
    #multiple_util_action_type_entropy_together(experiment_name, utilizations, net_size, run_name_data, {"interval": 8}, experiment_length=experiment_length, "stderr":True)

def plot_sem_large():
    print("plot_sem_large")

    net_size = "16x16"
    utilizations = ["0.1"]
    experiment_name = "larger network"
    experiment_length = 2000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Larger Network (16x16) Packet Loss", "stderr":True},#(0,.01) at 5 and 10 (0,0.4) at 15 and 20
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Larger Network (16x16) Hop Ratio", "stderr":True},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_3D():
    print("plot_sem_3D")

    net_size = "8_3d"
    utilizations = ["0.1"]
    experiment_name = "Topology 3D"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"3D Topology Packet Loss", "stderr":True},#(0,.05) at 20%
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"3D Topology Hop Ratio", "stderr":True},

            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_rings():
    print("plot_sem_rings")

    net_size = "5_3_hex"
    utilizations = ["0.1"]
    experiment_name = "Topology Connected Rings"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', OUR_SOLN_NAME, 1),
            ('NDD_Q-learning_a_0.1_e_0.05_g_0.99_mu_dc_8_mice-elephant_p_0.01', NDD_NAME, 2),
            #('rand_forward_mice-elephant_p_0.01', RAND_FORWARD_NAME, 3),
            ('rand_deflect_mice-elephant_p_0.01', RAND_DEFLECT_NAME, 4),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.05), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Rings Topology Packet Loss", "stderr":True},
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Rings Topology Hop Ratio", "stderr":True},

            #"reward": {},
            #"entropy": {"interval": 8},
            #"action_type_entropy": {"interval": 8},
            }
    across_util_plots = [
            #{"field": "DropRate mean", "fig_type": "Packet Loss", "ylim": (0,.1), "file_suffix": "/results.csv", "exact_match":True, "include_stdev":True, "include_min_max": False, "stdev_not_mean":False, "percent_data":True},
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_update_freq():
    print("plot_sem_update_freq")

    net_size = "8x8"
    utilizations = ["0.1"]
    experiment_name = "Update Frequency"
    experiment_length = 1000
    run_infixes = [
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_mice-elephant_p_0.01', 'Update frequency:  1', 1),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_2_mice-elephant_p_0.01', 'Update frequency:  2', 2),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_4_mice-elephant_p_0.01', 'Update frequency:  4', 3),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_8_mice-elephant_p_0.01', 'Update frequency:  8', 4),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_16_mice-elephant_p_0.01', 'Update frequency: 16', 5),
            ('mbd_original_s_[1_hop_shortest,2_hop_shortest]_r_1.0_d_1.0_sd_-1_ei_1,2,4,6,8_m_ui_32_mice-elephant_p_0.01', 'Update frequency: 32', 6),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.02), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Update Frequency (Rare Switching) Packet Loss", "stderr":True},#or (0,.02), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Update Frequency (Rare Switching) Hop Ratio", "stderr":True},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)

def plot_sem_algorithm_best():
    print("plot_algorithm_sweep_best")

    # TODO currently best D-LinUCB has a run that crashes
    net_size = "8x8"
    utilizations = ["0.2"]
    experiment_name = "Algorithm comparison"
    experiment_length = 1000
    run_infixes = [
            ("mbd_D-LinUCB_s_[1_hop_shortest,section]_df_0.99999_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", MBD_D_LINUCB, 1),
            ("mbd_original_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", MBD_ORIGINAL, 2),
            ("mbd_slide_s_[1_hop_shortest,section]_r_1.0_d_1.0_sd_-1_ei_5_mice-elephant_p_0.01", MBD_SLIDE, 3),
            ]
    run_suffix = ""
    single_util_plots = {
            "drop_rate": {"ylim": (0,.1), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Algorithm comparison Packet Loss", "stderr":True},#or (0,.02), (0,.1) at 10%, 20%
            "hop_ratio": {"ylim": (1,1.5), "smooth":(True, 5), "figsize":SEM_FIGSIZE, "title":"Algorithm comparison Hop Ratio", "stderr":True},
            }
    across_util_plots = [
            ]

    plot_data(experiment_name, net_size, utilizations, run_infixes, run_suffix, experiment_length, single_util_plots, across_util_plots)


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

'''
plot_utilization_sweep()

plot_algorithm_sweep_original()
plot_algorithm_sweep_slide()
plot_algorithm_sweep_D_LinUCB()
plot_algorithm_sweep_best()

plot_state_sweep_8x8()
plot_state_sweep_16x16()
'''

#plot_dlrl_poster()

#plot_limited_deflections_free_reign()
#plot_limited_deflections_best()

'''
plot_ndd_param_sweep()
plot_ndd_deflect_sweep()

plot_random_deflect_deflect_count()
'''

## evaluation experiments ##
plot_bandwidth()
plot_bandwidth_all()

'''
plot_8_8_long()

plot_update_freq_test()

plot_prop_delay()
plot_prop_delay_baselines()
plot_bandwidth()
plot_bandwidth_all()

plot_16x16()
plot_8_3d_grid()
plot_5_3_hex()
'''

## seminar ##
#FIGSIZE = (12,4)

#plot_sem_state_sweep()
#plot_sem_limited_deflections_deflect_count()
#plot_sem_limited_deflections_type()
#plot_sem_update_freq()

#plot_sem_comparison()
#plot_sem_utils_comparison()
#plot_sem_large()
#plot_sem_3D()
#plot_sem_rings()

#plot_sem_algorithm_best()

## cut results ##

#plot_limited_deflections()
#plot_limited_deflections_forward_first()
#plot_limited_deflecitons_only_forward()
#plot_limited_deflections_best_cut()
#plot_compare_2_deflect()
#plot_compare_limitless_deflect()
#plot_3_3_hex()
