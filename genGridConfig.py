import json
import random
import sys
import os

import numpy as np

TRAFFIC_MICE_ELEPHANT = 1
TRAFFIC_STATIC = 2
TRAFFIC_CHANGING = 3

seed = 0

flowId = 1

# switches
switchConfig = {
        "type": "mbd",
        #"type": "rand_deflect",
        "deflect_thresh": 1.0,
        "regularizer": 1.0,
        "discount_factor": 0.999,
        "delta": 1.0,
        "drop_action": False,
        "states": ["flow_id"],
        }

# links
linkConfig = {
        "speed": 1500000,
        "time": 0.1 # propagation delay
        }

# flows
flowConfig = {
        "start_rate": linkConfig["speed"], # TODO remove from flow class
        "type": "mbd",
        "start_time": 0,
        "end_time": 0,
        "ttl": None,
#        "generator": {
#            "type": "poisson",
#            "header": 20,
#            "body": 1480,
#            "bitrate": 1000000,
#            "buffer_size": 150000
#            }
        "generator": {
            "type": "compound_poisson",
            "header": 20,
            "body": 1480,
            "burst_rate": linkConfig["speed"],
            "burst_mean_len": 6.25/2,# 75000 b (6.25 * pktsize * 8)
            "mean_rate" : 500000/2,
            "buffer_size": 150000 # 100 packets TODO remove from generator class
            }
        }
#

# interfaces
interfaceConfig = {
        "out_buf_size": 1*(flowConfig["generator"]["header"] + flowConfig["generator"]["body"]),
        "in_buf_size": 1*(flowConfig["generator"]["header"] + flowConfig["generator"]["body"])
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

def get_X(Id, n):
    return (Id - 1) % n

def get_Y(Id, n):
    return (Id - 1) // n

def gen_nxn_network(n, config):
    # create network
    for y in range(n):
        for x in range(n):
            # create switch
            switch = {
                    "id": calculate_id(x, y, n),
                    "internal_speed": 0,
                    "network_size": n
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
                    "interfaces": [switch["id"] * 4 + 0, (switch["id"] - n) * 4 + 2] # switch up, neighbour down
                    })
                links[-1].update(linkConfig)
            # create right link
            if x < n - 1:
                links.append({
                    "id": switch["id"] * 2 + 1,
                    "interfaces": [switch["id"] * 4 + 1, (switch["id"] + 1) * 4 + 3] # switch right, neighbour left
                    })
                links[-1].update(linkConfig)

            config["switches"].append(switch)
            config["interfaces"] += interfaces
            config["links"] += links

# generate flows

def createValidSwitches(config):
    # structure: {id: [current, max]}
    switchIds = [s["id"] for s in config["switches"]]
    switch_max_rate = {sId: linkConfig["speed"] * sum([1 if i["handler_id"] == sId else 0 for i in config["interfaces"]])
                    for sId in switchIds}

    return {switchId: [0, switch_max_rate[switchId]] for switchId in switchIds}

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

    num_hops = abs(get_X(sourceId, size) - get_X(destId, size)) + abs(get_Y(sourceId, size) - get_Y(destId, size))
    return flowRate * num_hops, (sourceId, destId) # utilization of the flow

def gen_flows(config, util_thresh, net_size, sim_end_time):
    flow_length = 250#10
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
    flow_band, source_dest = add_random_flow(config, net_size, valid_switches, available_switches, full_switches, 0, sim_end_time)
    cur_util += flow_band / net_band
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
    for i in range(len(live_flows)):
        new_end = (i+1) / len(config["flows"]) * flow_length
        config["flows"][i]["end_time"] = new_end

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

#def create_config(dest_dir, size, traffic_type, net_util, agent_type, agent_alg, states, simulation_length, num_flow_sets, num_flow_changes, runs, flowRoutes=None):
def create_config(filepath, size, traffic_type, net_util, simulation_length, flow_gen_type, runs, flowRoutes=None, numChanges=None):

    for run in range(runs):
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

        gen_nxn_network(size, config)

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

        #os.makedirs(filepath, exist_ok=True)

        filename = f"run_{run}"
        pathname = os.path.join(filepath, filename)
        print("writing to file:", pathname)
        #with open(pathname, "w") as f:
        #    f.write(json.dumps(config, indent=4))

def gen_path(dest_dir, size, agent_type, prop_delay, traffic_type, utilization,
        mbd_update=None, mbd_states=None, mbd_regularizer=None, mbd_delta=None,
        mbd_discount_factor=None, ndd_agent=None, ndd_alpha=None, ndd_epsilon=None,
        ndd_gamma=None):
    filepath = os.path.join(dest_dir, f"{size}x{size}")

    # TODO mbd state
    if agent_type == "mbd":
        filepath = os.path.join(filepath, f"mbd")
        if mbd_update == "slide":
            filepath = os.path.join(filepath, f"slide")
        elif mbd_update == "original":
            filepath = os.path.join(filepath, f"original")
        elif mbd_update == "D-LinUCB":
            filepath = os.path.join(filepath, f"D-LinUCB", f"d_{mbd_discount_factor}")
        else:
            print(f"mbd_update: {mbd_update} is not supported")
            return None
        # parameters
        states = ','.join(sorted(mbd_states))
        filepath = os.path.join(filepath, f"s_{states}", f"r_{mbd_regularizer}-d_{mbd_delta}")
    # NDD
    elif agent_type == "NDD":
        filepath = os.path.join(filepath, f"NDD")
        if ndd_agent == "rand":
            filepath = os.path.join(filepath, f"rand")
        elif ndd_agent == "Q-learning":
            filepath = os.path.join(filepath, f"rand", f"a_{ndd_alpha}-e_{ndd_epsilon}-g_{ndd_gamma}")
        else:
            print(f"ndd_agent: {ndd_agent} is not supported")
            return None
    elif agent_type == "rand_deflect":
        filepath = os.path.join(filepath, f"rand_deflect")
    elif agent_type == "rand_forward":
        filepath = os.path.join(filepath, f"rand_forward")
    else:
        print(f"agent_type: {agent_type} is not supported")
        return None

    # add prop delay
    filepath = os.path.join(filepath, f"p_{prop_delay}")

    # add traffic type
    if traffic_type == TRAFFIC_MICE_ELEPHANT:
        filepath = os.path.join(filepath, f"mice-elephant")
    elif traffic_type == TRAFFIC_STATIC:
        filepath = os.path.join(filepath, f"static")
    else:
        print("traffic type not in allowed set")
        return None

    # utilization
    filepath = os.path.join(filepath, f"u_{utilization}")

    if "None" in filepath:
        print("found None in filepath")
        return None

    return filepath

def main():
    size = 8
    runs = 30
    simulation_length = 2000 # 1000
    num_flow_sets = 1
    num_flow_changes = 200 # 100

    flowConfig["ttl"] = size * 3
    switchConfig["drop_action"] = False#True
    switchConfig["discount_factor"] = 0.9999

    linkConfig["time"] = 0.1 # 0.01, 0.5 # propagation delay

    dest_dir = "configs"

    traffic_type = "bursty"
    net_utils = [0.05, 0.1, 0.15, 0.2] # [0.1,0.2,0.3,0.4]
    agent_type = ["mbd", "rand_deflect", "rand_forward", "NDD"][0]
    agent_alg = ["original", "slide", "D-LinUCB", None][1]
    ndd_agent = ["rand", "Q-learning"]
    ndd_alpha = [0.05][0]
    ndd_epsilon = [0.05][0]
    ndd_gamma = [0.99][0]
    states = [[None],
            ["1-2_hop_shortest"],
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
            ["dest_id", "drop_probability"], ["flow_id"]][5:7]#[1:12]#[1:14]#[3:8]

    for net_util in net_utils:
        for state in states:
            switchConfig["states"] = state
            switchConfig["type"] = agent_type
            switchConfig["agent_alg"] = agent_alg

            #flow_gen_type = TRAFFIC_CHANGING#TRAFFIC_MICE_ELEPHANT
            flow_gen_type = TRAFFIC_MICE_ELEPHANT

            filepath = gen_path(dest_dir, size, agent_type, linkConfig['time'],
                    flow_gen_type, net_util, mbd_update=agent_alg, mbd_states=state,
                    mbd_regularizer=switchConfig["regularizer"], mbd_delta=switchConfig["delta"],
                    mbd_discount_factor=switchConfig["discount_factor"], ndd_agent=ndd_agent,
                    ndd_alpha=ndd_alpha, ndd_epsilon=ndd_epsilon, ndd_gamma=ndd_gamma)
            print(filepath)
            #create_config(filepath, size, traffic_type, net_util,
            #        simulation_length, flow_gen_type, runs, numChanges=num_flow_changes)

if __name__ == "__main__":
    main()
