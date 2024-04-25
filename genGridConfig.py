import json
import random
import sys
import os
import itertools

import numpy as np

TRAFFIC_MICE_ELEPHANT = 1
TRAFFIC_STATIC = 2
TRAFFIC_CHANGING = 3

NETWORK_2D = 1
NETWORK_3D = 2
NETWORK_OTHER = 3

seed = 0

flowId = 1

# switches
switchConfigDefault = {
        "discount_factor": 1.0,
        }
switchConfig = {
        }

# links
linkConfigDefault = {
        "speed": 1500000,
        }
linkConfig = {
        }

# flows
flowConfigDefault = {
        "start_rate": linkConfigDefault["speed"], # TODO remove from flow class
        "type": "mbd",
        "start_time": 0,
        "end_time": 0,
        "ttl": None,
        "generator": {
            "type": "compound_poisson",
            "header": 20,
            "body": 1480,
            "burst_rate": linkConfigDefault["speed"],
            "burst_mean_len": 6.25/2,# 75000 b (6.25 * pktsize * 8)
            "mean_rate" : 500000/2,
            "buffer_size": 150000 # 100 packets TODO remove from generator class
            }
        }
flowConfig = {
        }
#

# interfaces
interfaceConfig = {
        "out_buf_size": 1*(flowConfigDefault["generator"]["header"] +
            flowConfigDefault["generator"]["body"]),
        "in_buf_size": 1*(flowConfigDefault["generator"]["header"] +
            flowConfigDefault["generator"]["body"])
        }

flowRoutes = [{
    "source_id": 1,
    "dest": 3,#9,
    "start_time": 25 # delayed start
    }]
'''
flowRoutes = [{
    "source_id": 1,
    "dest": 3,#9,
    "start_time": 25 # delayed start
    },{
    "source_id": 4,#1,
    "dest": 6,#3,
    },{
    "source_id": 7,
    "dest": 9,#3,
    }]
'''

def calculate_id(x, y, n):
    return y * n + x + 1

def calculate_3d_id(x, y, z, n):
    return z * n**2 + y * n + x + 1

def get_num_hops(source_id, dest_id, size, switches):
    return sum([abs(s - d) for s, d in zip(switches[source_id][2], switches[dest_id][2])])

def get_2d_sector(x, y, size, sector_per_dim):
    xs = int(x / size * sector_per_dim)
    ys = int(y / size * sector_per_dim)

    return xs + sector_per_dim * ys

def get_3d_sector(x, y, z, size, sector_per_dim):
    xs = int(x / size * sector_per_dim)
    ys = int(y / size * sector_per_dim)
    zs = int(z / size * sector_per_dim)

    return xs + sector_per_dim * ys + sector_per_dim**2 * zs

def gen_3d_network(n, config):
    for z in range(n):
        for y in range(n):
            for x in range(n):
                switch = {
                        "id": calculate_3d_id(x, y, z, n),
                        "internal_speed": 0,
                        "network_size": n,
                        "coordinates": [x, y, z],
                        "section": get_3d_sector(x, y, z, n, 3),
                        "num_sections": 3**3,
                        }
                switch.update(switchConfig)
                # create interfaces
                # interface IDs id * 6 + (0-6, [up, right, in, down, left, out])
                interfaces = []

                # create up
                if y > 0:
                    interfaces.append({
                        "id": switch["id"] * 6 + 0,
                        "handler_id": switch["id"]
                        })
                    interfaces[-1].update(interfaceConfig)
                # create right
                if x < n - 1:
                    interfaces.append({
                        "id": switch["id"] * 6 + 1,
                        "handler_id": switch["id"]
                        })
                    interfaces[-1].update(interfaceConfig)
                # create in
                if z < n - 1:
                    interfaces.append({
                        "id": switch["id"] * 6 + 2,
                        "handler_id": switch["id"]
                        })
                    interfaces[-1].update(interfaceConfig)
                # create down
                if y < n - 1:
                    interfaces.append({
                        "id": switch["id"] * 6 + 3,
                        "handler_id": switch["id"]
                        })
                    interfaces[-1].update(interfaceConfig)
                # create left
                if x > 0:
                    interfaces.append({
                        "id": switch["id"] * 6 + 4,
                        "handler_id": switch["id"]
                        })
                    interfaces[-1].update(interfaceConfig)
                # create out
                if z > 0:
                    interfaces.append({
                        "id": switch["id"] * 6 + 5,
                        "handler_id": switch["id"]
                        })
                    interfaces[-1].update(interfaceConfig)

                # create up and right links
                links = []
                # create up link
                if y > 0:
                    links.append({
                        "id": switch["id"] * 3,
                        "interfaces": [switch["id"] * 6 + 0, calculate_3d_id(x, y-1, z, n) * 6 + 3] # switch up, neighbour down
                        })
                    links[-1].update(linkConfig)
                # create right link
                if x < n - 1:
                    links.append({
                        "id": switch["id"] * 3 + 1,
                        "interfaces": [switch["id"] * 6 + 1, calculate_3d_id(x+1, y, z, n) * 6 + 4] # switch right, neighbour left
                        })
                    links[-1].update(linkConfig)
                # create in link
                if z < n - 1:
                    links.append({
                        "id": switch["id"] * 3 + 2,
                        "interfaces": [switch["id"] * 6 + 2, calculate_3d_id(x, y, z+1, n) * 6 + 5] # switch in, neighbour out
                        })
                    links[-1].update(linkConfig)

                config["switches"].append(switch)
                config["interfaces"] += interfaces
                config["links"] += links

def gen_2d_network(n, config):
    # create network
    for y in range(n):
        for x in range(n):
            # create switch
            switch = {
                    "id": calculate_id(x, y, n),
                    "internal_speed": 0,
                    "network_size": n,
                    "coordinates": [x, y],
                    "sector": get_2d_sector(x, y, n, 3),
                    }
            switch.update(switchConfig)

            # create interfaces
            # interface IDs i*n*4 + j*4 + (0-3, [up, right, down, left])
            interfaces = []

            # create up
            if y > 0:
                interfaces.append({
                    "id": switch["id"] * 4 + 0,
                    "handler_id": switch["id"]
                    })
                interfaces[-1].update(interfaceConfig)
            # create right
            if x < n - 1:
                interfaces.append({
                    "id": switch["id"] * 4 + 1,
                    "handler_id": switch["id"]
                    })
                interfaces[-1].update(interfaceConfig)
            # create down
            if y < n - 1:
                interfaces.append({
                    "id": switch["id"] * 4 + 2,
                    "handler_id": switch["id"]
                    })
                interfaces[-1].update(interfaceConfig)
            # create left
            if x > 0:
                interfaces.append({
                    "id": switch["id"] * 4 + 3,
                    "handler_id": switch["id"]
                    })
                interfaces[-1].update(interfaceConfig)

            # create up and right links
            links = []
            # create up link
            if y > 0:
                links.append({
                    "id": switch["id"] * 2,
                    "interfaces": [switch["id"] * 4 + 0, calculate_id(x, y-1, n) * 4 + 2] # switch up, neighbour down
                    })
                links[-1].update(linkConfig)
            # create right link
            if x < n - 1:
                links.append({
                    "id": switch["id"] * 2 + 1,
                    "interfaces": [switch["id"] * 4 + 1, calculate_id(x+1, y, n) * 4 + 3] # switch right, neighbour left
                    })
                links[-1].update(linkConfig)

            config["switches"].append(switch)
            config["interfaces"] += interfaces
            config["links"] += links

def createValidSwitches(config):
    # structure: {id: [current, max]}
    switchIds = [(s["id"], s["coordinates"]) for s in config["switches"]]
    switch_max_rate = {sId: linkConfig["speed"] * sum([1 if i["handler_id"] == sId else 0 for i in config["interfaces"]])
                    for sId, _ in switchIds}

    return {switchId: [0, switch_max_rate[switchId], coordinate] for switchId, coordinate in switchIds}

def add_random_flow(config, size, switches, open_switches, fullSwitches, start_time, end_time):
    global flowId

    flowRate = flowConfig["generator"]["mean_rate"]

    sIndex = random.randint(0, len(open_switches) - 1)
    dIndex = random.randint(0, len(open_switches) - 2)
    if dIndex >= sIndex:
        dIndex += 1

    sourceId = open_switches[sIndex]
    destId = open_switches[dIndex]

    flow = {
            "id": flowId,
            "source_id": sourceId,
            "dest": destId,
            "start_time": start_time,
            "end_time": end_time
            }
    flow = dict(list(flowConfig.items()) + list(flow.items()))
    config["flows"].append(flow)

    flowId += 1
    # update
    switches[sourceId][0] += flowRate
    switches[destId][0] += flowRate

    # if not able to accept another flow then remove
    if switches[sourceId][0] + flowRate > switches[sourceId][1]:
        open_switches.pop(sIndex)
        fullSwitches.append(sIndex)
    if dIndex >= sIndex:
        dIndex -= 1
    if switches[destId][0] + flowRate > switches[destId][1]:
        open_switches.pop(destId)
        fullSwitches.append(destId)

    num_hops = get_num_hops(sourceId, destId, size, switches)
    return flowRate * num_hops, (sourceId, destId) # utilization of the flow

def gen_flows(config, util_thresh, net_size, sim_end_time):
    num_elephants = 2
    flow_length = 10
    cur_util = 0
    net_band = len(config["links"]) * linkConfig["speed"] * 2

    valid_switches = createValidSwitches(config)
    full_switches = []
    available_switches = [switch_id for switch_id in valid_switches.keys()]

    flow_rate = flowConfig["generator"]["mean_rate"]
    live_flows = [] # (end time, util, source_id, dest_id)

    util_data = {
            "time": [],
            "util": []
            }

    # generate two elephant flows
    for _ in range(num_elephants):
        flow_band, source_dest = add_random_flow(config, net_size, valid_switches, available_switches, full_switches, 0, sim_end_time)
        cur_util += flow_band / net_band


    # generate initial flows
    while cur_util < util_thresh:
        start_time = 0
        end_time = flow_length
        flow_band, source_dest = add_random_flow(config, net_size, valid_switches, available_switches, full_switches, start_time, end_time)
        flow_util = flow_band / net_band
        cur_util += flow_util

        # add flow to life flow tracking list
        live_flows.append((end_time, flow_util, source_dest[0], source_dest[1]))

    # go through all the current flows and update their ending times
    for i in range(len(live_flows) - num_elephants):
        new_end = (i+1) / len(config["flows"]) * flow_length
        flow_i = i + num_elephants
        config["flows"][flow_i]["end_time"] = new_end

        # update flow
        live_flows[i] = list(live_flows[i])
        live_flows[i][0] = new_end
        live_flows[i] = tuple(live_flows[i])

    # need flows sorted to remove the next to close
    live_flows.sort()

    util_data["time"].append(0.0)
    util_data["util"].append(cur_util)

    # while there is still a flow that will end before the end
    while live_flows[0][0] < sim_end_time:
        dead_flow = live_flows.pop(0)
        cur_util -= dead_flow[1]
        current_time = dead_flow[0]
        source_id = dead_flow[2]
        dest_id = dead_flow[3]

        # switches are free'd up - TODO move into function
        valid_switches[source_id][0] -= flow_rate
        valid_switches[dest_id][0] -= flow_rate
        if source_id in full_switches:
            available_switches.append(source_id)
            full_switches.remove(source_id)
        if dest_id in full_switches:
            available_switches.append(dest_id)
            full_switches.remove(dest_id)

        # add new flows while room
        while cur_util < util_thresh:
            start_time = current_time
            end_time = start_time + max(1, np.random.normal(flow_length, 1))
            #flow = gen_flow(size, new_start, flow_length)
            #flow_list.append(flow)
            flow_band, source_dest = add_random_flow(config, net_size, valid_switches, available_switches, full_switches, start_time, end_time)
            flow_util = flow_band / net_band
            cur_util += flow_util
            #live_flows.append((flow["end"], flow["util"]))
            live_flows.append((end_time, flow_util, source_dest[0], source_dest[1]))

        live_flows.sort()

        # if new time then add the entry
        if current_time != util_data["time"][-1]:
            util_data["time"].append(current_time)
            util_data["util"].append(cur_util)
        else: # multiple flow's ended at the same time
            util_data["util"][-1] = cur_util

    time_deltas = np.array([util_data["time"][i + 1] - util_data["time"][i] for i in range(len(util_data["time"])-1)]
            + [sim_end_time - util_data["time"][-1]])
    time_deltas /= time_deltas.mean()
    avg_util = (time_deltas * np.array(util_data["util"])).mean()

    print(f"average utilisation {avg_util}", file=sys.stderr)
    return util_data

def genChangingFlows(config, net_util, net_size, num_changes, sim_end_time):
    net_band = len(config["links"]) * linkConfig["speed"] * 2
    flows_band = 0
    valid_switches = createValidSwitches(config)
    full_switches = [] # structure [id]
    available_switches = [switchId for switchId in valid_switches.keys()]
    flow_rate = flowConfig["generator"]["mean_rate"]
    # track flows structure [(end_time, utilization, source_id, dest_id)]
    flows = []

    flow_i = 0
    start_time = 0

    avg_util = 0

    util_data = {
            "time": [0.0],
            "util": [0]
            }

    # queue of flows by their end times
    while start_time < sim_end_time:
        end_time = sim_end_time / num_changes * flow_i
        flow_i += 1
        # generate and track flow
        flow_util, source_dest = add_random_flow(config, net_size, valid_switches, available_switches, full_switches, start_time, end_time)
        flows.append((end_time, flow_util, source_dest[0], source_dest[1]))
        flows_band += flow_util

        # if current time == last util time
        if start_time == util_data["time"][-1]:
            util_data["time"][-1] = start_time
            util_data["util"][-1] = flows_band / net_band
        else:
            util_data["time"].append(start_time)
            util_data["util"].append(flows_band / net_band)

        # move forward start_time to next time flows_band is low
        while flows_band > net_band * net_util:
            # remove top flow and to update parameters
            new_start_time, flow_util, source_id, dest_id = flows.pop(0)
            time_delta = new_start_time - start_time
            start_time = new_start_time

            valid_switches[source_id][0] -= flow_rate
            valid_switches[dest_id][0] -= flow_rate

            # if source or dest were full then remove and put in available list
            if source_id in full_switches:
                available_switches.append(source_id)
                full_switches.remove(source_id)
            if dest_id in full_switches:
                available_switches.append(dest_id)
                full_switches.remove(dest_id)

            # update average util since time has passed
            avg_util += flows_band / net_band * time_delta / sim_end_time

            flows_band -= flow_util

    time_deltas = np.array([util_data["time"][i + 1] - util_data["time"][i] for i in range(len(util_data["time"])-1)]
            + [sim_end_time - util_data["time"][-1]])
    time_deltas /= time_deltas.mean()
    avg_util = (time_deltas * np.array(util_data["util"])).mean()

    print(f"average utilisation {avg_util}", file=sys.stderr)
    return util_data

def genRandomFlows(config, netUtil, n, start_time, end_time):
    netBand = len(config["links"]) * linkConfig["speed"] * 2
    flowBand = 0
    validSwitches = createValidSwitches(config)
    fullSwitches = [] # structure [id]
    availableSwitches = [switchId for switchId in validSwitches.keys()]
    flowRate = flowConfig["generator"]["mean_rate"]

    while flowBand < netBand * netUtil:
        flow_util, _ = add_random_flow(config, n, validSwitches, availableSwitches, fullSwitches, start_time, end_time)
        flowBand += flow_util

    print(f"utilisation {flowBand/netBand}", file=sys.stderr)
    return {"time": 0.0, "util": flowBand/netBand}

def addRoutes(config, flowRoutes):
    for i in range(len(flowRoutes)):
        route = flowRoutes[i]

        flow = {
                "id": i + 1
                }
        flow.update(route)
        flow = dict(list(flowConfig.items()) + list(flow.items()))
        config["flows"].append(flow)

def create_config(filepath, size, net_util, simulation_length, flow_gen_type, runs, network_type,
                  numChanges=None):

    for run in range(runs):
        np.random.seed(seed + run)
        random.seed(seed + run) # run lengths can be increased without changing the initial configuration
        #random.seed(seed) # uncomment to have all runs have the same flow configuration
        # create base object
        config = {}

        # create switches and endpoints - 1 endpoint per switch
        config["switches"] = []
        config["endpoints"] = []
        config["interfaces"] = []
        config["links"] = []
        config["flows"] = []

        if network_type == NETWORK_2D:
            gen_2d_network(size, config)
        elif network_type == NETWORK_3D:
            gen_3d_network(size, config)
        else:
            print("network type not supported")
            return

        global flowId
        flowId = 1

        util_data = None

        if flow_gen_type == TRAFFIC_MICE_ELEPHANT:
            util_data = gen_flows(config, net_util, size, simulation_length)
        elif flow_gen_type == TRAFFIC_STATIC:
            start_time = 0
            end_time = 0
            util_data = genRandomFlows(config, net_util, size, start_time, end_time)
        elif flow_gen_type == TRAFFIC_CHANGING:
            util_data = genChangingFlows(config, net_util, size, numChanges, simulation_length)

        # update switch flow counts
        for i in range(len(config["switches"])):
            config["switches"][i]["num_flows"] = len(config["flows"])

        # add util_data to the flow
        config["util_data"] = util_data

        os.makedirs(filepath, exist_ok=True)

        filename = f"run_{run}"
        pathname = os.path.join(filepath, filename)
        print("writing to file:", pathname)
        with open(pathname, "w") as f:
            f.write(json.dumps(config, indent=4))

def gen_path(dest_dir, size, network_type, experiment_config):
    filepath = os.path.join(dest_dir, f"{size}x{size}")
    if network_type == NETWORK_3D:
        filepath = os.path.join(dest_dir, f"{size}_3d")

    if experiment_config["type"] == "mbd":
        filepath = os.path.join(filepath, f"mbd")
        mbd_update = experiment_config["mbd_alg"]
        if mbd_update == "slide":
            filepath = os.path.join(filepath, f"slide")
        elif mbd_update == "original":
            filepath = os.path.join(filepath, f"original")
        elif mbd_update == "D-LinUCB":
            mbd_discount_factor = experiment_config["discount_factor"]
            filepath = os.path.join(filepath, f"D-LinUCB", f"d_{mbd_discount_factor}")
        else:
            print(f"mbd_update: {mbd_update} is not supported")
            return None
        # parameters
        mbd_states = experiment_config["states"]
        states = ','.join(sorted(mbd_states))

        mbd_regularizer = experiment_config["regularizer"]
        mbd_delta = experiment_config["delta"]
        mbd_static_deflect = experiment_config["static_deflections"]
        if experiment_config["action_limit"] != "none":
            filepath = os.path.join(filepath, experiment_config["action_limit"])

        filepath = os.path.join(filepath, f"s_{states}",
                f"r_{mbd_regularizer}-d_{mbd_delta}-sd_{mbd_static_deflect}")
    # NDD
    elif experiment_config["type"] == "NDD":
        filepath = os.path.join(filepath, f"NDD")
        ndd_agent = experiment_config["ndd_alg"]
        if ndd_agent == "rand":
            filepath = os.path.join(filepath, f"rand")
        elif ndd_agent == "Q-learning":
            ndd_alpha = experiment_config["ndd_alpha"]
            ndd_epsilon = experiment_config["ndd_epsilon"]
            ndd_gamma = experiment_config["ndd_gamma"]
            filepath = os.path.join(filepath, f"Q-learning", f"a_{ndd_alpha}-e_{ndd_epsilon}-g_{ndd_gamma}")
        else:
            print(f"ndd_agent: {ndd_agent} is not supported")
            return None
        if experiment_config["only_forward"] == True:
            filepath = os.path.join(filepath, "of")
        if experiment_config["multiple_updates"] == True:
            filepath = os.path.join(filepath, "mu")
    elif experiment_config["type"] == "rand_deflect":
        filepath = os.path.join(filepath, f"rand_deflect")
        sd = experiment_config["static_deflections"]
        if sd != -1:
            filepath = os.path.join(filepath, f"sd_{sd}")
    elif experiment_config["type"] == "rand_forward":
        filepath = os.path.join(filepath, f"rand_forward")
    else:
        print(f"agent_type: {agent_type} is not supported")
        return None

    # add prop delay
    prop_delay = experiment_config["prop_delay"]
    filepath = os.path.join(filepath, f"p_{prop_delay}")

    # add traffic type
    traffic_type = experiment_config["traffic_type"]
    if traffic_type == TRAFFIC_MICE_ELEPHANT:
        filepath = os.path.join(filepath, f"mice-elephant")
    elif traffic_type == TRAFFIC_STATIC:
        filepath = os.path.join(filepath, f"static")
    else:
        print("traffic type not in allowed set")
        return None

    # utilization
    utilization = experiment_config["net_util"]
    filepath = os.path.join(filepath, f"u_{utilization}")

    if "None" in filepath:
        print("found None in filepath")
        return None

    return filepath

def setup_configs(size, experiment_config):
    global switchConfig, linkConfig, flowConfig
    switchConfig = switchConfigDefault.copy()
    linkConfig = linkConfigDefault.copy()
    flowConfig = flowConfigDefault.copy()

    linkConfig["time"] = experiment_config["prop_delay"]
    flowConfig["type"] = experiment_config["flow_type"]

    # set switch type
    switchConfig["type"] = experiment_config["type"]
    if experiment_config["type"] == "mbd":
        switchConfig["drop_action"] = False
        switchConfig["action_limit"] = experiment_config["action_limit"]

        switchConfig["agent_alg"] = experiment_config["mbd_alg"]
        switchConfig["states"] = experiment_config["states"]

        switchConfig["delta"] = experiment_config["delta"]
        switchConfig["regularizer"] = experiment_config["regularizer"]

        if experiment_config["mbd_alg"] == "D-LinUCB":
            switchConfig["discount_factor"] = experiment_config["discount_factor"]

        flowConfig["static_deflections"] = experiment_config["static_deflections"]
        switchConfig["deflect_thresh"] = 1.0 # required because it inherits from rand_deflect
    elif experiment_config["type"] == "NDD":
        switchConfig["DHC_max"] = 2
        # DN max time = longest path with most deflections
        switchConfig["DN_max_time"] = (2 * (size - 1) + 2) * 2 * experiment_config["prop_delay"]
        switchConfig["NDDAgent"] = {"NDD_type": experiment_config["ndd_alg"]}

        if experiment_config["ndd_alg"] == "Q-learning":
            switchConfig["NDDAgent"]["alpha"] = experiment_config["ndd_alpha"]
            switchConfig["NDDAgent"]["epsilon"] = experiment_config["ndd_epsilon"]
            switchConfig["NDDAgent"]["gamma"] = experiment_config["ndd_gamma"]
        switchConfig["only_forward"] = experiment_config["only_forward"]
        switchConfig["multiple_updates"] = experiment_config["multiple_updates"]

    elif experiment_config["type"] == "rand_forward":
        pass
    elif experiment_config["type"] == "rand_deflect":
        switchConfig["deflect_thresh"] = 1.0
        flowConfig["static_deflections"] = experiment_config["static_deflections"]

def gen_config_list(net_utils, prop_delay, traffic_types, agent_types, mbd_agent_algs, mbd_states,
        mbd_hyper_params, mbd_static_deflect, mbd_action_limits, ndd_alg,
        ndd_hyper_params, ndd_only_forward, ndd_multiple_updates, rand_deflect_static_deflects):

    agent_dicts = []

    if "NDD" in agent_types:
        ndd_algs = []
        ndd_dict = [{"type":"NDD", "flow_type":"ndd", "only_forward": ndd_of, "multiple_updates": ndd_mu}
                for ndd_of in ndd_only_forward for ndd_mu in ndd_multiple_updates]
        # ndd qlearning
        if "Q-learning" in ndd_alg:
            ndd_q_learning = [{**d, **{"ndd_alg":"Q-learning", "ndd_alpha":a, "ndd_epsilon":e, "ndd_gamma":g}}
                    for d in ndd_dict for a, e, g in ndd_hyper_params]
            ndd_algs += ndd_q_learning
        if "rand" in ndd_alg:
            ndd_rand = [{**d, **{"ndd_alg": "rand"}}
                    for d in ndd_dict]
            ndd_algs += ndd_rand
        agent_dicts += ndd_algs

    # mbd
    if "mbd" in agent_types:
        mbd_algs = []
        mbd_dict = [{"type": "mbd", "flow_type": "mbd", "drop_action": False,
            "static_deflections": sd, "states": s, "action_limit": al}
                for sd in mbd_static_deflect for s in mbd_states for al in mbd_action_limits]

        if "D-LinUCB" in mbd_agent_algs:
            print(mbd_hyper_params)
            mbd_algs += [{**d, **{"mbd_alg": "D-LinUCB", "regularizer": r, "delta": de, "discount_factor": df}}
                    for d in mbd_dict for r, de, df in mbd_hyper_params]
        # TODO handle non D-LinUCB
        if "original" in mbd_agent_algs:
            mbd_hyper_params_copy = [h for h, _ in itertools.groupby(
                sorted([[r, de] for r, de, _ in mbd_hyper_params])
                )]
            mbd_algs += [{**d, **{"mbd_alg": "original", "regularizer": r, "delta": de}}
                    for d in mbd_dict for r, de in mbd_hyper_params_copy]
        if "slide" in mbd_agent_algs:
            mbd_hyper_params_copy = [h for h, _ in itertools.groupby(
                sorted([[r, de] for r, de, _ in mbd_hyper_params])
                )]
            mbd_algs += [{**d, **{"mbd_alg": "slide", "regularizer": r, "delta": de}}
                    for d in mbd_dict for r, de in mbd_hyper_params_copy]

        agent_dicts += mbd_algs

    if "rand_forward" in agent_types:
        rand_forward_dict = [{"type": "rand_forward", "flow_type": "basic"}]
        agent_dicts += rand_forward_dict

    if "rand_deflect" in agent_types:
        rand_deflect_dict = [{"type": "rand_deflect", "flow_type": "rand_deflect", "static_deflections": sd}
                for sd in rand_deflect_static_deflects]
        agent_dicts += rand_deflect_dict

    experiment_dicts = [{**d, **{"net_util": u, "prop_delay": p, "traffic_type": t}}
            for d in agent_dicts for u in net_utils for p in prop_delay for t in traffic_types]

    return experiment_dicts

def main():
    size = 8
    runs = 30
    simulation_length = 2000 # 1000
    num_flow_changes = 200 # 100
    network_type = [NETWORK_2D, NETWORK_3D][0]

    flowConfigDefault["ttl"] = size * 3

    dest_dir = "configs"

    traffic_types = [TRAFFIC_MICE_ELEPHANT, TRAFFIC_CHANGING, TRAFFIC_STATIC][0:1]
    net_utils = [0.05, 0.1, 0.15, 0.2][0:1] # [0.1,0.2,0.3,0.4]
    prop_delays = [0.01,0.1,0.5][0:1]
    agent_types = ["mbd", "NDD", "rand_forward", "rand_deflect"][0:1]
    mbd_agent_algs = ["original", "slide", "D-LinUCB", None][1:3]
    mbd_hyper_params = [
            # regularizer, delta, discount factor
            [1.0, 1.0, 0.999],
            [1.0, 2.0, 0.999],
            [1.0, 2.0, 0.999],
            ]
    # TODO add S in D-linUCB
    # ranges delta [0.1, 2]
    # ranges discount factor [0.9999,0.9] -> log
    # single sample 1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    # [1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    #   for low, high in [[l,h],[l,h]]] # loop over ranges
    #mbd_regularizers = [0.5,0.9,1.0,1.1,2.0][2:3]
    #mbd_deltas = [0.1,0.5,0.9,1.0][3:4]
    #mbd_discount_factors = [0.99, 0.999, 0.9999][1:2]
    mbd_static_deflect = [-1,2,4][0:1]
    mbd_action_limits = ["none", "only_deflect", "forward_first"][0:1]
    mbd_states = [["1-2_hop_shortest"],
            ["1-2_hop_shortest", "section"],
            ["2_hop_shortest"],
            ["1_hop_shortest"],
            ["1_hop_shortest", "section"],
            ["2_hop_shortest","1_hop_shortest"],
            ["2_hop_shortest","1_hop_shortest","section"],
            ["1_hop_shortest", "section", "deflect_probability"],
            ["2_hop_shortest", "1_hop_shortest", "section", "deflect_probability"],
            ["1_hop_shortest", "section", "drop_probability"],
            ["2_hop_shortest", "1_hop_shortest", "section", "drop_probability"],
            ["dest_id"],
            ["dest_id", "deflect_probability"],
            ["dest_id", "drop_probability"], ["flow_id"]][4:5]#[1:12]#[1:14]#[3:8]
    ndd_algs = ["rand", "Q-learning"][1:2]
    ndd_hyper_params =[
            #alpha, epsilon, gamma
            [0.5,0.05,0.99],
            ]
    # ranges alpha [0.1, 0.0001] -> 10-10000 steps - log
    # ranges epsilon [0.05, 0.001] -> 20-200 steps - log
    # ranges gamma [0.99,0.9] -> ?-? log
    # single sample 1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    # [1/(10**np.random.uniform(np.log10(1 / low), np.log10(1 / high)))
    #   for low, high in [[0.0001,0.1],[0.001,0.05],[0.9,0.99]]] # loop over ranges
    ndd_only_forward = [False, True][0:1]
    ndd_multiple_updates = [False, True][1:2]
    rand_deflect_static_deflects = [-1,2][0:1]

    experiment_configs = gen_config_list(net_utils, prop_delays, traffic_types, agent_types,
            mbd_agent_algs, mbd_states, mbd_hyper_params,
            mbd_static_deflect, mbd_action_limits, ndd_algs, ndd_hyper_params,
            ndd_only_forward, ndd_multiple_updates, rand_deflect_static_deflects)

    for experiment_config in experiment_configs:
        setup_configs(size, experiment_config)
        net_util = experiment_config["net_util"]
        flow_gen_type = experiment_config["traffic_type"]

        filepath = gen_path(dest_dir, size, network_type, experiment_config)
        print("generating:", filepath)
        create_config(filepath, size, net_util, simulation_length, flow_gen_type, runs, network_type)

if __name__ == "__main__":
    main()
