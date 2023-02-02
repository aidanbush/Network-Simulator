import json
import random
import sys

random.seed(0)

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

def getValidSwitches(config):
    switchIds = [s["id"] for s in config["switches"]]
    switchMaxRate = [linkConfig["speed"] * sum([1 if i["handler_id"] == sId else 0 for i in config["interfaces"]])
                     for sId in switchIds] # count number of interfaces per switch and multiply by rate
    return list(map(list, zip(switchIds, switchMaxRate, [0 for _ in range(len(config["switches"]))])))

flowId = 1
def genRandomFlows(config, netUtil, n, start_time, end_time):
    netBand = len(config["links"]) * linkConfig["speed"] * 2
    flowBand = 0
    global flowId
    validSwitches = getValidSwitches(config)
    flowRate = flowConfig["generator"]["mean_rate"]

    while flowBand < netBand * netUtil:
        sIndex = random.randint(0, len(validSwitches) - 1)
        dIndex = random.randint(0, len(validSwitches) - 2)
        if dIndex >= sIndex:
            dIndex += 1

        source = validSwitches[sIndex]
        dest = validSwitches[dIndex]

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
        num_hops = abs(get_X(sourceId, n) - get_X(destId, n)) + abs(get_Y(sourceId, n) - get_Y(destId, n))
        flowBand += flowRate * num_hops# * number of links
        # update
        validSwitches[sIndex][2] += flowRate
        validSwitches[dIndex][2] += flowRate

        # if not able to accept another flow then remove
        if validSwitches[sIndex][2] + flowRate > validSwitches[sIndex][1]:
            validSwitches.pop(sIndex)
        if dIndex >= sIndex:
            dIndex -= 1
        if validSwitches[dIndex][2] + flowRate > validSwitches[dIndex][1]:
            validSwitches.pop(dIndex)

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

def create_config(size, traffic_type, net_util, agent_type, states, simulation_length, num_flow_sets, flowRoutes=None):
    # create base object
    config = {}

    # create switches and endpoints - 1 endpoint per switch
    config["switches"] = []
    config["endpoints"] = []
    config["interfaces"] = []
    config["links"] = []
    config["flows"] = []

    gen_nxn_network(size, config)

    if num_flow_sets == 1:
        start_time = 0
        end_time = 0
        genRandomFlows(config, net_util, size, start_time, end_time)
    else:
        for i in range(num_flow_sets):
            start_time = simulation_length / num_flow_sets * (i-1)
            end_time = simulation_length / num_flow_sets * (i)
            genRandomFlows(config, net_util, size, start_time, end_time)
    #addRoutes(config, flowRoutes)

    # update switch flow counts
    for i in range(len(config["switches"])):
        config["switches"][i]["num_flows"] = len(config["flows"])

    fname_state = ""
    if states != None:
        fname_state = f"_[{','.join(states)}]"
    fname_flow_sets = ""
    if num_flow_sets != 1:
        fname_flow_sets = f"_fs_{num_flow_sets}"
    filename = f"{size}x{size}_{traffic_type}_{net_util}_{agent_type}{fname_state}{fname_flow_sets}.json"
    print("writing to file:", filename)
    with open(filename, "w") as f:
        f.write(json.dumps(config, indent=4))

def main():
    size = 5

    simulation_length = 200
    num_flow_sets = 1

    traffic_type = "bursty"
    net_utils = [0.1,0.2,0.3,0.4,0.5]
    agent_type = "rand_deflect"#"mbd"
    states = [None]#["2_hop_shortest","1_hop_shortest"]]#["1-2_hop_shortest"]]#["2_hop_shortest"]]#["1_hop_shortest"]]#[["dest_id"],["flow_id"]]

    for net_util in net_utils:
        for state in states:
            switchConfig["states"] = state
            switchConfig["type"] = agent_type

            create_config(size, traffic_type, net_util, agent_type, state, simulation_length, num_flow_sets)

if __name__ == "__main__":
    main()
