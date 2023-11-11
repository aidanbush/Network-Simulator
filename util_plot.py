import pandas as pd
import matplotlib.pyplot as plt
import json

def plot_utils(df):
    # need to convert to total_seconds (float) or else fill_between breaks
    utils, times = df["util"], df["time"].dt.total_seconds()
    plt.plot(times, utils['mean'], color='blue')
    #plt.plot(times, utils['min'], alpha=0.4, color='blue')
    #plt.plot(times, utils['max'], alpha=0.4, color='blue')
    plt.fill_between(times,
            utils['mean'] - utils['std'],
            utils['mean'] + utils['std'],
            alpha=0.4)

    plt.show()

def load_file(filename, interval):
    df = None

    with open(filename, 'r') as f:
        obj = json.load(f)
        utils = obj["util"]
        times = pd.to_timedelta(obj["time"], "s")
        # load into pandas
        df = pd.DataFrame(list(zip(utils, times)), columns=["util", "time"])
        df = df.set_index("time").resample(interval).mean().fillna(method="ffill")
        # resample at given interval

    return df

def load_files(filename_list):
    df_list = []
    interval = '100ms'

    for filename in filename_list:
        df_list.append(load_file(filename, interval))

    df = pd.concat(df_list).resample(interval).agg({'util': ['mean', 'std', 'min', 'max']})
    df.reset_index(inplace=True)

    return df

def main():
    test_dir = "results/"
    test_name = "8x8_bursty_" + "mbd_slide_[1_hop_shortest,3x3_section]_prop_0.1/"
    files = [test_dir + test_name + f"run{i}_util_data" for i in range(30)]

    df = load_files(files)

    plot_utils(df)

if __name__ == "__main__":
    main()
