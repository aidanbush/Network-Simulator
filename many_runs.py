import os
import genGridConfig as helper
import numpy as np

def gen_experiment_name(size, config, network_type):
    net_util = config["net_util"]

    if network_type == helper.NETWORK_2D:
        network_shape = f"{size}x{size}"
    elif network_type == helper.NETWORK_2D_CUT:
        network_shape = f"{size}x{size}_c"
    elif network_type == helper.NETWORK_3D:
        network_shape = f"{size}_3d"

    traffic_type = ""
    if config["traffic_type"] == helper.TRAFFIC_MICE_ELEPHANT:
        traffic_type = "mice-elephant"
    elif config["traffic_type"] == helper.TRAFFIC_STATIC:
        traffic_type = "static"
    else:
        return None
    prop_delay = config["prop_delay"]

    agent = config["type"]
    if agent == "mbd":
        mbd_type = config["mbd_alg"]
        agent += f"_{mbd_type}" + "_s_[" + ",".join(sorted(config["states"])) + "]"
        if mbd_type == "D-LinUCB":
            agent += "_df_" + str(config["discount_factor"])
        agent += "_r_" + str(config["regularizer"]) + "_d_" + str(config["delta"]) \
                + "_sd_" + str(config["static_deflections"]) \
                + "_ei_" + ",".join(map(str, config["entropy_intervals"]))
        if config["action_limit"] != "none":
            agent += "_" + config["action_limit"]
    elif agent == "NDD":
        ndd_type = config["ndd_alg"]
        agent += f"_{ndd_type}"
        if ndd_type == "Q-learning":
            agent += "_a_" + str(config["ndd_alpha"]) + "_e_" + str(config["ndd_epsilon"]) \
                    + "_g_" + str(config["ndd_gamma"])
        if config["only_forward"] == True:
            agent += "_of"
        if config["multiple_updates"] == True:
            agent += "_mu"
        if config["ndd_deflect_count"] != 2:
            agent += "_dc_" + str(config["ndd_deflect_count"])
    elif agent == "rand_forward":
        pass
    elif agent == "rand_deflect":
        sd = config["static_deflections"]
        if sd != -1:
            agent += f"_sd_{sd}"
    else:
        print("agent not valid type")
        return None

    return f"{network_shape}_{agent}_{traffic_type}_p_{prop_delay}_u_{net_util}"

def main():
    size = 8#16
    num_runs = 30
    parallel = 40
    config_dir = "configs"
    network_type = [helper.NETWORK_2D, helper.NETWORK_2D_CUT, helper.NETWORK_3D][1]

    traffic_types = [helper.TRAFFIC_MICE_ELEPHANT, helper.TRAFFIC_CHANGING, helper.TRAFFIC_STATIC][0:1]
    net_utils = [0.05, 0.1, 0.15, 0.2, 0.25][0:2]
    net_utils = [0.1, 0.2][0:2] # [0.1,0.2,0.3,0.4]
    prop_delays = [0.01,0.1,0.5][0:1]
    #prop_delays = [0.001,0.1,1.0]
    agent_types = ["mbd", "NDD", "rand_forward", "rand_deflect"][2:3]
    mbd_agent_algs = ["original", "slide", "D-LinUCB", None][0:1]
    mbd_hyper_params = [
            # regularizer, delta, discount factor
            [1.0, 1.0, 0.99999],
            #[1.0, 2.0, 0.999],
            #[1.0, 2.0, 0.999],
            ]
    #mbd_hyper_params = [(0.7, 0.97, 0.999), (1.2, 0.9, 0.9999), (0.86, 0.66, 0.999), (0.37, 0.96, 0.999), (0.95, 0.62, 0.99999), (1.26, 0.99, 0.99999), (1.96, 0.99, 0.9999), (0.57, 0.71, 0.999), (1.76, 0.78, 0.99999), (0.93, 0.54, 0.999), (1.33, 0.33, 0.99999), (0.9, 1.0, 0.999), (1.17, 0.7, 0.999), (1.41, 0.25, 0.99999), (1.32, 0.92, 0.999)][5:10]
    # ranges delta [0.1, 2]
    # ranges discount factor [0.9999,0.9] -> log
    # single sample 1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    # [1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    #   for low, high in [[l,h],[l,h]]] # loop over ranges
    #mbd_regularizers = [0.5,0.9,1.0,1.1,2.0][2:3]
    #mbd_deltas = [0.1,0.5,0.9,1.0][3:4]
    #mbd_discount_factors = [0.99, 0.999, 0.9999][3:4]
    mbd_static_deflect = [-1,2,4,6,8][4:5]
    mbd_action_limits = ["none", "forward_first", "only_deflect"][0:2]
    mbd_states = [
            ["1_hop_shortest"],
            ["1-2_hop_shortest"],
            ["1_hop_shortest", "2_hop_shortest"],
            ["1_hop_shortest", "section"],
            ["1-2_hop_shortest", "section"],
            ["1_hop_shortest", "2_hop_shortest", "section"],
            ["1_hop_shortest", "section", "deflect_probability"],
            ["2_hop_shortest", "1_hop_shortest", "section", "deflect_probability"],
            ["1_hop_shortest", "section", "drop_probability"],
            ["2_hop_shortest", "1_hop_shortest", "section", "drop_probability"],
            ["dest_id"],
            ["dest_id", "deflect_probability"],
            ["dest_id", "drop_probability"], ["flow_id"]][2:3]#[0:3][3:5]
    mbd_entropy_interval = [[1,2,4,6,8],[5]][1:2]
    ndd_algs = ["rand", "Q-learning"][1:2]
    ndd_hyper_params =[
            #alpha, epsilon, gamma
            [0.5,0.05,0.99],
            ]
    ndd_deflect_counts = [2,4,6,8][0:1]
    # ranges alpha [0.1, 0.0001] -> 10-10000 steps - log
    # ranges epsilon [0.05, 0.001] -> 20-200 steps - log
    # ranges gamma [0.99,0.9] -> ?-? log
    # single sample 1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    # [1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    #   for low, high in [[0.0001,0.1],[0.001,0.05],[0.9,0.99]]] # loop over ranges
    ndd_only_forward = [False, True][0:1]
    ndd_multiple_updates = [False, True][1:2]
    rand_deflect_static_deflects = [-1,2][0:2]

    experiment_configs = helper.gen_config_list(net_utils, prop_delays,
            traffic_types, agent_types, mbd_agent_algs, mbd_states,
            mbd_hyper_params, mbd_static_deflect, mbd_action_limits,
            mbd_entropy_interval, ndd_algs, ndd_hyper_params, ndd_deflect_counts,
            ndd_only_forward, ndd_multiple_updates, rand_deflect_static_deflects)

    for config in experiment_configs:
        run_name = gen_experiment_name(size, config, network_type)
        run_path = helper.gen_path(config_dir, size, network_type, config)
        timeout = 60*60*24*5 # 5 days
        command_line = f"bash run_test.sh -t {timeout} -p {parallel} -n {num_runs} {run_path} {run_name} {size}x{size}manhattan_flow_params.json"
        print(command_line)
        #os.system(command_line)

if __name__ == "__main__":
    main()
