import os
import genGridConfig as helper

def gen_experiment_name(size, config):
    net_util = config["net_util"]
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
        agent += "_r_" + str(config["regularizer"]) + "_d_" + str(config["delta"])
    elif agent == "ndd":
        ndd_type = config["ndd_alg"]
        agent += f"_{ndd_type}"
        if ndd_type == "Q-learning":
            agent += "_a_" + str(config["ndd_alpha"]) + "_e_" + str(config["ndd_epsilon"]) \
                    + "_g_" + str(config["ndd_gamma"])
    elif agent == "rand_forward":
        pass
    elif agent == "rand_deflect":
        pass
    else:
        return None

    return f"{size}x{size}_{agent}_{traffic_type}_p_{prop_delay}_u_{net_util}"

def main():
    size = 8
    num_runs = 30
    config_dir = "configs"

    traffic_types = [helper.TRAFFIC_MICE_ELEPHANT, helper.TRAFFIC_CHANGING, helper.TRAFFIC_STATIC][0:1]
    net_utils = [0.05, 0.1, 0.15, 0.2][0:1]
    prop_delays = [0.01,0.1,0.5][1:2]
    agent_types = ["mbd", "rand_forward", "rand_deflect", "NDD"][1:2]
    mbd_agent_algs = ["original", "slide", "D-LinUCB", None][1:2]
    mbd_regularizers = [1.0]
    mbd_deltas = [1.0]
    mbd_discount_factors = [0.99, 0.999, 0.9999][2:3]
    mbd_states = [["1-2_hop_shortest"],
            ["1-2_hop_shortest", "3x3_section"],
            ["2_hop_shortest"],
            ["1_hop_shortest"],
            ["1_hop_shortest", "3x3_section"],
            ["2_hop_shortest","1_hop_shortest"],
            ["2_hop_shortest","1_hop_shortest","3x3_section"],
            ["1_hop_shortest", "3x3_section", "deflect_probability"],
            ["2_hop_shortest", "1_hop_shortest", "3x3_section", "deflect_probability"],
            ["1_hop_shortest", "3x3_section", "drop_probability"],
            ["2_hop_shortest", "1_hop_shortest", "3x3_section", "drop_probability"],
            ["dest_id"],
            ["dest_id", "deflect_probability"],
            ["dest_id", "drop_probability"]][5:6]
    ndd_algs = ["rand", "Q-learning"][0:2]
    ndd_alphas = [0.05][0:1]
    ndd_epsilons = [0.05][0:1]
    ndd_gammas = [0.99][0:1]

    experiment_configs = helper.gen_config_list(net_utils, prop_delays, traffic_types,
            agent_types, mbd_agent_algs, mbd_states, mbd_regularizers, mbd_deltas,
            mbd_discount_factors, ndd_algs, ndd_alphas, ndd_epsilons, ndd_gammas)

    for config in experiment_configs:
        run_name = gen_experiment_name(size, config)
        run_path = helper.gen_path(config_dir, size, config)
        command_line = f"bash run_test.sh -t 50000 -p {num_runs} -n {num_runs} {run_path} {run_name} {size}x{size}manhattan_flow_params.json"
        print(command_line)
        os.system(command_line)

if __name__ == "__main__":
    main()
