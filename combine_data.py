import os, sys, re
import pandas as pd

def get_filenames(data_dir, pattern_str):
    pattern = re.compile(pattern_str)
    filenames = [] # (string, groups)

    for filename in os.listdir(data_dir):
        match = pattern.match(filename)
        if match:
            filenames.append((os.path.join(data_dir, match.string), match.groups()))

    return filenames

# extract mean and std for each metric accross multiple runs
def extract_mean_std_per_column(filename_tuples, metric_name=None):
    if len(filename_tuples) == 0:
        print(f"No files for {metric_name}")
        return None

    df_list = []
    
    print("Reading Data(extract_mean_std_per_column)")

    # get all link sets
    for filename, _ in filename_tuples:
        new_df = pd.read_csv(filename)
        df_list.append(new_df)

    df = pd.concat(df_list, ignore_index=True)

    print("Calculating statistical metrics")

    # calculate means and stdev
    mean_df = df.groupby('Time').mean()
    std_df = df.groupby('Time').std()

    # quantiles
    top_5_df = df.groupby('Time').quantile(0.95)
    bottom_5_df = df.groupby('Time').quantile(0.05)
    top_10_df = df.groupby('Time').quantile(0.90)
    bottom_10_df = df.groupby('Time').quantile(0.10)

    # add metric name infix and type suffix
    mean_df = mean_df.add_suffix(f" {metric_name} mean")
    std_df = std_df.add_suffix(f" {metric_name} stdev")

    top_5_df = top_5_df.add_suffix(f" {metric_name} top 5%")
    top_10_df = top_10_df.add_suffix(f" {metric_name} top 10%")
    bottom_5_df = bottom_5_df.add_suffix(f" {metric_name} bottom 5%")
    bottom_10_df = bottom_10_df.add_suffix(f" {metric_name} bottom 10%")

    return pd.concat([mean_df, std_df, top_5_df, bottom_5_df, top_10_df, bottom_10_df], axis=1).reset_index()

# extract a single mean and std for all metrics accross multiple runs
# used with flows where the individual flow means don't matter
def extract_single_mean_std(filename_tuples, metric_name):
    if len(filename_tuples) == 0:
        print(f"No files for {metric_name}")
        return pd.DataFrame()

    df = None

    print("Reading Data (extract_single_mean_std)")

    # get all path sets
    for filename, groups in filename_tuples:
        new_df = pd.read_csv(filename)
        new_df = new_df.set_index(["Time"]).add_suffix(f"_{groups[0]}").reset_index()
        if df is None:
            df = new_df
        else:
            df = df.merge(new_df, on="Time")

    print("Calculating statistical metrics")

    times_df = df.iloc[:,0]
    # calculate means and stdev
    mean_df = df.apply(lambda row: row[1:].mean(skipna=True), axis=1).rename(f"{metric_name} mean")
    std_df = df.apply(lambda row: row[1:].std(skipna=True), axis=1).rename(f"{metric_name} stdev")

    # quantiles
    top_5_df = df.apply(lambda row: row[1:].quantile(0.95), axis=1).rename(f"{metric_name} top 5%")
    bottom_5_df = df.apply(lambda row: row[1:].quantile(0.05), axis=1).rename(f"{metric_name} bottom 5%")
    top_10_df = df.apply(lambda row: row[1:].quantile(0.90), axis=1).rename(f"{metric_name} top 10%")
    bottom_10_df = df.apply(lambda row: row[1:].quantile(0.10), axis=1).rename(f"{metric_name} bottom 10%")

    # combine all dataframes
    return pd.concat([times_df, mean_df, std_df, top_5_df, bottom_5_df, top_10_df, bottom_10_df], axis=1)

def extract_sum_single_mean_std(filename_tuples, metric_name):
    if len(filename_tuples) == 0:
        print(f"No files for {metric_name}")
        return pd.DataFrame()

    df = None

    # the first element of the tuple is the run number
    df_run_map = {}

    print("Reading Data")

    # loop over path, merging into dataframe mapped to by run number
    for filename, groups in filename_tuples:
        new_df = pd.read_csv(filename)
        run = groups[0]
        new_df = new_df.set_index(["Time"]).add_suffix(f"_{groups[0]}").reset_index()
        if run not in df_run_map:
            df_run_map[run] = new_df
        else:
            df_run_map[run] = df_run_map[run].merge(new_df, on="Time")

    print("Summing Data")

    # loop over runs and calculate sums
    for run, run_df in df_run_map.items():
        # group by
        sum_df = run_df.groupby("Time").sum()
        if df == None:
            df = sum_df
        else:
            df = df.merge(sum_df, on="Time")

    print("Calculating statistical metrics")

    times_df = df.iloc[:,0]
    # calculate means and stdev
    mean_df = df.apply(lambda row: row[1:].mean(skipna=True), axis=1).rename(f"{metric_name} mean")
    std_df = df.apply(lambda row: row[1:].std(skipna=True), axis=1).rename(f"{metric_name} stdev")

    # quantiles
    top_5_df = df.apply(lambda row: row[1:].quantile(0.95), axis=1).rename(f"{metric_name} top 5%")
    bottom_5_df = df.apply(lambda row: row[1:].quantile(0.05), axis=1).rename(f"{metric_name} bottom 5%")
    top_10_df = df.apply(lambda row: row[1:].quantile(0.90), axis=1).rename(f"{metric_name} top 10%")
    bottom_10_df = df.apply(lambda row: row[1:].quantile(0.10), axis=1).rename(f"{metric_name} bottom 10%")

    # combine all dataframes
    return pd.concat([times_df, mean_df, std_df, top_5_df, bottom_5_df, top_10_df, bottom_10_df], axis=1)

def link_data(data_path, results_path, output_filename):
    print("extracting link data")
    # get set of all files
    data_pattern = "^run_(\d+)_LinkUsage.csv"
    filename_tuples = get_filenames(data_path, data_pattern)

    df = extract_mean_std_per_column(filename_tuples, metric_name="link usage")

    # write to file
    filepath = os.path.join(results_path, output_filename)
    print(f"writing to {filepath}")
    df.to_csv(filepath, index=False)

def switch_data(data_path, results_path, output_filename):
    print("extracting switch data")
    metrics = [
            "actionablePackets",
            "deflectedPackets",
            "droppedPackets",
            "encounteredPackets",
            "forwardedPackets",
            "timedOutPackets",
            "averageAvailableInterfacesRatio",
            "averageForwardInterfacesRatio",
            "averageOutgoingLinkUsage",

            # learning specific metrics
            "allActionsEntropy_1",
            "allActionsEntropy_2",
            "allActionsEntropy_4",
            "allActionsEntropy_6",
            "allActionsEntropy_8",
            "deflectionEntropy_1",
            "deflectionEntropy_2",
            "deflectionEntropy_4",
            "deflectionEntropy_6",
            "deflectionEntropy_8",
            "numAvailableActions",
            "entropyActionsTaken",
            "learningActionsTaken",
            "averageReward",
            "actionsRewarded",
            ]

    # combine into master dataframe
    df = None
    for metric in metrics:
        print(f"extracting switch {metric} data")
        data_pattern = f"^run_(\d+)_{metric}.csv"
        filename_tuples = get_filenames(data_path, data_pattern)
        # if no filenames report and continue
        if len(filename_tuples) == 0:
            print(f" - no data for switch {metric} found")
            continue

        # calculate per switch
        print("calculate per switch")
        metric_df = extract_mean_std_per_column(filename_tuples, metric_name=metric)
        if df is None:
            df = metric_df
        elif metric_df is not None:
            df = df.merge(metric_df, on="Time")
        metric_df = None

        # calculate over the network
        print("calculate over the network")
        metric_df = extract_single_mean_std(filename_tuples, metric)
        if df is None:
            df = metric_df
        elif metric_df is not None:
            df = df.merge(metric_df, on="Time")
        metric_df = None

    # write
    filepath = os.path.join(results_path, output_filename)
    print(f"writing to {filepath}")
    df.to_csv(filepath, index=False)

def calculate_difference(metric1_filename, metric2_filename, name):
    # calculate drop / sent
    metric1_df = pd.read_csv(metric1_filename)
    metric2_df = pd.read_csv(metric2_filename)
    # sum metrics and calculate rate
    metric1_sum = pd.DataFrame({
        "Time": metric1_df["Time"],
        "metric1 sum": metric1_df.apply(lambda row: row[1:].sum(), axis=1)
        })
    metric2_sum = pd.DataFrame({
        "Time": metric2_df["Time"],
        "metric2 sum": metric2_df.apply(lambda row: row[1:].sum(), axis=1)
        })

    # calculate rate and put into dataframe with time
    difference_df = pd.merge(metric1_sum, metric2_sum, on="Time")
    difference_df = pd.DataFrame({
        "Time": difference_df["Time"],
        name: difference_df["metric1 sum"] / difference_df["metric2 sum"]
        })

    return difference_df

# calculates drop rate over the entire network not by flow
# so the std is the std of the network between runs
def calculate_rate(data_path, denominator_pattern, numerator_pattern, metric_name):
    denominator_file_tuples = get_filenames(data_path, denominator_pattern)
    numerator_file_tuples = get_filenames(data_path, numerator_pattern)
    # if no filenames report and continue
    if len(denominator_file_tuples) == 0 or len(numerator_file_tuples) == 0:
        print(f" - no data rate metric found")
        return None

    # combines tuples
    # sort tuples - assume there are corresponding files
    combined_file_tuples = list(zip(sorted(denominator_file_tuples), sorted(numerator_file_tuples)))
    # [(denominator tuple 1, numerator tuple 1), ...]
    df = None
    for (denominator_filename, den_groups), (numerator_filename, _2) in combined_file_tuples:
        rate_df = calculate_difference(denominator_filename, numerator_filename, "rate")
        rate_df = rate_df.set_index(["Time"]).add_suffix(f"_{den_groups[0]}").reset_index()

        if df is None: # copy over
            df = rate_df
        else: # merge on Time
            df = df.merge(rate_df, on="Time")

    times_df = df.iloc[:,0]
    # calculate means and stdev
    mean_df = df.apply(lambda row: row[1:].mean(skipna=True), axis=1).rename(f"{metric_name} mean")
    std_df = df.apply(lambda row: row[1:].std(skipna=True), axis=1).rename(f"{metric_name} stdev")

    # quantiles
    top_5_df = df.apply(lambda row: row[1:].quantile(0.95), axis=1).rename(f"{metric_name} top 5%")
    bottom_5_df = df.apply(lambda row: row[1:].quantile(0.05), axis=1).rename(f"{metric_name} bottom 5%")
    top_10_df = df.apply(lambda row: row[1:].quantile(0.90), axis=1).rename(f"{metric_name} top 10%")
    bottom_10_df = df.apply(lambda row: row[1:].quantile(0.10), axis=1).rename(f"{metric_name} bottom 10%")

    # combine all three dataframes
    return pd.concat([times_df, mean_df, std_df, top_5_df, bottom_5_df, top_10_df, bottom_10_df], axis=1)

def calculate_custom_flow_metrics(data_path, prefix):
    print("Calculating drop rate")
    df = None
    # drop rate = dropped packets / sent packets
    drop_pattern = f"^run_(\d+)_{prefix}DroppedPackets.csv"
    sent_pattern = f"^run_(\d+)_{prefix}SentPackets.csv"
    drop_rate_df = calculate_rate(data_path, drop_pattern, sent_pattern, f"{prefix}DropRate")
    df = drop_rate_df

    print("Calculating timeout drop rate")
    # timeout drop rate = timedout packets / sent packets
    timeout_pattern = f"^run_(\d+)_{prefix}TimedOutPackets.csv"
    timeout_drop_rate_df = calculate_rate(data_path, timeout_pattern, sent_pattern, f"{prefix}TimeoutDropRate")
    if not timeout_drop_rate_df is None:
        df = df.merge(timeout_drop_rate_df, on="Time")

    print("Calculating congested drop rate")
    # congested drop rate = congested packets / sent packets
    congested_pattern = f"^run_(\d+)_{prefix}CongestedPackets.csv"
    congested_drop_rate_df = calculate_rate(data_path, congested_pattern, sent_pattern, f"{prefix}CongestedDropRate")
    if not congested_drop_rate_df is None:
        df = df.merge(congested_drop_rate_df, on="Time")

    return df

def flow_data(data_path, results_dir, output_filename):
    print("extracting flow data")
    metrics = [
            "AcksArrived",
            "AverageHops",
            "AverageRTT",
            "HopRatio",
            "MinRTT",
            "OutOfOrderRatio",
            "SentRate",
            "Throughput",
            ]

    count_metrics = [
            "CongestedPackets",
            "DroppedPackets",
            "ErroredPackets",
            "PacketsArrived",
            "SentPackets",
            "TimedOutPackets",
            ]

    metrics = [pre + m for pre in ["", "elephant_"] for m in metrics]
    count_metrics = [pre + m for pre in ["", "elephant_"] for m in count_metrics]

    df = None
    for metric in metrics:
        print(f"extracting flow {metric} data")
        data_pattern = f"^run_(\d+)_{metric}.csv"
        filename_tuples = get_filenames(data_path, data_pattern)
        # if no filenames report and continue
        if len(filename_tuples) == 0:
            print(f" - no data for switch {metric} found")
            continue

        metric_df = extract_single_mean_std(filename_tuples, metric)
        if df is None:
            df = metric_df
        elif metric_df is not None:
            df = df.merge(metric_df, on="Time")

    for metric in count_metrics:
        print(f"extracting flow {metric} data (sum)")
        data_pattern = f"^run_(\d+)_{metric}.csv"
        filename_tuples = get_filenames(data_path, data_pattern)
        # if no filenames report and continue
        if len(filename_tuples) == 0:
            print(f" - no data for switch {metric} found")
            continue

        metric_df = extract_single_mean_std(filename_tuples, metric)
        if df is None:
            df = metric_df
        elif metric_df is not None:
            df = df.merge(metric_df, on="Time")

    print("Calculating custom flow metrics")
    # calculate custom_metrics
    custom_df = calculate_custom_flow_metrics(data_path, "")
    custom_elephant_df = calculate_custom_flow_metrics(data_path, "elephant_")

    if custom_df is not None:
        df = df.merge(custom_df, on="Time")

    if custom_elephant_df is not None:
        df = df.merge(custom_elephant_df, on="Time")

    filepath = os.path.join(results_dir, output_filename)
    print(f"writing to {filepath}")
    df.to_csv(filepath, index=False)

def main():
    data_path = sys.argv[1]
    result_path = sys.argv[2]
    flow_filename = sys.argv[3]
    link_filename = sys.argv[4]
    switch_filename = sys.argv[5]

    link_data(data_path, result_path, link_filename)
    flow_data(data_path, result_path, flow_filename)
    switch_data(data_path, result_path, switch_filename)

if __name__ == "__main__":
    main()
