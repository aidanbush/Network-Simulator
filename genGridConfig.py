import json
import random
import sys
import os

seed = 0

flowId = 1

# switches
switchConfig = {
        "type": "mbd",
        #"type": "rand_deflect",
        "deflect_thresh": 1.0,
        "regularizer": 1.0,
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
            "burst_mean_len": 6.25,# 75000 b (6.25 * pktsize * 8)
            "mean_rate" : 500000,
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
    # structure
    # {id: available speed}
    switchIds = [s["id"] for s in config["switches"]]
    switchMaxRate = [linkConfig["speed"] * sum([1 if i["handler_id"] == sId else 0 for i in config["interfaces"]])
                     for sId in switchIds] # count number of interfaces per switch and multiply by rate
    return list(map(list, zip(switchIds, switchMaxRate, [0 for _ in range(len(config["switches"]))])))

def add_random_flow(config, size, flowBand, switches, fullSwitches, start_time, end_time):
    global flowId

    flowRate = flowConfig["generator"]["mean_rate"]

    sIndex = random.randint(0, len(switches) - 1)
    dIndex = random.randint(0, len(switches) - 2)
    if dIndex >= sIndex:
        dIndex += 1

    source = switches[sIndex]
    dest = switches[dIndex]

    sourceId = source[0]
    destId = dest[0]

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
    num_hops = abs(get_X(sourceId, size) - get_X(destId, size)) + abs(get_Y(sourceId, size) - get_Y(destId, size))
    flowBand += flowRate * num_hops# * number of links
    # update
    switches[sIndex][2] += flowRate
    switches[dIndex][2] += flowRate

    # if not able to accept another flow then remove
    if switches[sIndex][2] + flowRate > switches[sIndex][1]:
        switches.pop(sIndex)
        fullSwitches.push(sIndex)
    if dIndex >= sIndex:
        dIndex -= 1
    if switches[dIndex][2] + flowRate > switches[dIndex][1]:
        switches.pop(dIndex)
        fullSwitches.push(dIndex)

    return flowBand

def genRandomFlows(config, netUtil, n, start_time, end_time):
    netBand = len(config["links"]) * linkConfig["speed"] * 2
    flowBand = 0
    global flowId
    validSwitches = createValidSwitches(config)
    fullSwitches = {} # structure {id: available speed}
    flowRate = flowConfig["generator"]["mean_rate"]

    while flowBand < netBand * netUtil:
        flowBand = add_random_flow(config, n, flowBand, validSwitches, fullSwitches, start_time, end_time)

    print(f"utilisation {flowBand/netBand}", file=sys.stderr)

def addRoutes(config, flowRoutes):
    for i in range(len(flowRoutes)):
        route = flowRoutes[i]

        flow = {
                "id": i + 1
                }
        flow.update(route)
        flow = dict(list(flowConfig.items()) + list(flow.items()))
        config["flows"].append(flow)

def create_config(dest_dir, size, traffic_type, net_util, agent_type, states, simulation_length, num_flow_sets, runs, flowRoutes=None):
    random.seed(seed)

    for run in range(runs):
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

        if num_flow_sets == 1:
            start_time = 0
            end_time = 0
            genRandomFlows(config, net_util, size, start_time, end_time)
        else:
            for i in range(num_flow_sets):
                start_time = simulation_length / num_flow_sets * (i)
                end_time = simulation_length / num_flow_sets * (i+1)
                genRandomFlows(config, net_util, size, start_time, end_time)
        #addRoutes(config, flowRoutes)

        # update switch flow counts
        for i in range(len(config["switches"])):
            config["switches"][i]["num_flows"] = len(config["flows"])

        fname_state = ""
        if states != None:
            fname_state = f"_[{','.join(states)}]"

        #fname_flow_sets = ""
        #if num_flow_sets != 1:
        #    fname_flow_sets = f"_fs_{num_flow_sets}"
        #filename = f"{size}x{size}_{traffic_type}_{net_util}_{agent_type}{fname_state}{fname_flow_sets}.json"
        #pathname = os.path.join(dest_dir, filename)

        if num_flow_sets == 1:
            filepath = os.path.join(dest_dir, f"{size}x{size}", f"{agent_type}{fname_state}", f"u_{net_util}")
        else:
            filepath = os.path.join(dest_dir, f"{size}x{size}", f"{agent_type}{fname_state}", f"fs_{num_flow_sets}", f"u_{net_util}")

        os.makedirs(filepath, exist_ok=True)

        filename = f"run_{run}"
        pathname = os.path.join(filepath, filename)
        print("writing to file:", pathname)
        with open(pathname, "w") as f:
            f.write(json.dumps(config, indent=4))

def main():
    size = 5
    runs = 20
    simulation_length = 200
    num_flow_sets = 1

    dest_dir = "configs"

    traffic_type = "bursty"
    net_utils = [0.1,0.2,0.3,0.4] #, 0.5]
    agent_type = ["mbd", "rand_deflect", "rand_forward"][0]
    states = [[None],["2_hop_shortest","1_hop_shortest"],["1-2_hop_shortest"],["2_hop_shortest"],\
            ["1_hop_shortest"],["1_hop_shortest", "3x3_section"],["dest_id"],["flow_id"]][4:5]

    for net_util in net_utils:
        for state in states:
            switchConfig["states"] = state
            switchConfig["type"] = agent_type

            create_config(dest_dir, size, traffic_type, net_util, agent_type, state, simulation_length, num_flow_sets, runs)

if __name__ == "__main__":
    main()
