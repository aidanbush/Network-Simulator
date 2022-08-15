import json

# 1,1 in the bottom left
# n = network_size + 1
# change to 1,1
# coordinates: x, y
#   switch:
#       id = x*n + y
#       id % n = y
#       id / n = x (id / n % n = x, if endpoint)
#   interface:
#       id = switch id * 6 + direction (0 = up, 1 = right, 2 = down, 3 = left, 4 = switch, 5 = endpoint)
#       id / 6 = switch id
#       id % 6 = direction (0 = up, 1 = right, 2 = down, 3 = left, 4 = switch, 5 = endpoint)
#   link:
#       id = (left or down switch id) * 3 + direction (0 up, 1 right, 2 to endpoint)
#       id / 3 = switch id (down or left)
#       id % 3 = direction (0 up, 1 right, 2 to endpoint)

size = 3
n = size+1

# endpoints

# switches
switchConfig = {
        "type": "mdc", #"rand_deflect",
        "deflect_thresh": 1.0
        }

# flows
flowConfig = {
        "start_rate": 1500000,
        "type": "mdc",
        "start_time": 0,
        "end_time": 0,
        "generator": {
            "type": "poisson",
            "header": 20,
            "body": 1480,
            "bitrate": 1500000,
            "buffer_size": 150000
            }
        }
#
flowRoutes = [{
    "source_id": 21,
    "dest": 31
    },{
    "source_id": 21,
    "dest": 31
    },{
    "source_id": 23,
    "dest": 29
    }
        ]

# interfaces
interfaceConfig = {
        "out_buf_size": flowConfig["generator"]["header"] + flowConfig["generator"]["body"],
        "in_buf_size": flowConfig["generator"]["header"] + flowConfig["generator"]["body"]
        }

endpointInterfaceConfig = {
        "out_buf_size": interfaceConfig["out_buf_size"] * 4,
        "in_buf_size": interfaceConfig["in_buf_size"] * 4
        }

# links
linkConfig = {
        "speed": 1500000,
        "time": 0.1 # propagation delay
        }
endpointLinkConfig = {
        "speed": linkConfig["speed"] * 4, # update not true at edges
        "time": 0.0 # propagation delay
        }

# create base object
config = {}

# create switches and endpoints - 1 endpoint per switch
config["switches"] = []
config["endpoints"] = []
config["interfaces"] = []
config["links"] = []

for x in range(1,n):
    for y in range(1,n):
        # create switch and endpoint
        switch = {
                "id": x*n + y,
                "internal_speed": 0,
                "network_size": size
                }
        switch.update(switchConfig)

        endpoint = {
                "id": n**2 + switch["id"],
                "internal_speed": 0
                }

        # create interfaces
        # interface IDs i*n*4 + j*4 + (0-3, [up, right, down, left])
        interfaces = []

        # create up
        if y != n - 1:
            interfaces.append({
                "id": switch["id"] * 6 + 0,
                "handler_id": switch["id"]
                })
            interfaces[-1].update(interfaceConfig)
        # create right
        if x != n - 1:
            interfaces.append({
                "id": switch["id"] * 6 + 1,
                "handler_id": switch["id"]
                })
            interfaces[-1].update(interfaceConfig)
        # create down
        if y != 0:
            interfaces.append({
                "id": switch["id"] * 6 + 2,
                "handler_id": switch["id"]
                })
            interfaces[-1].update(interfaceConfig)
        # create left
        if x != 0:
            interfaces.append({
                "id": switch["id"] * 6 + 3,
                "handler_id": switch["id"]
                })
            interfaces[-1].update(interfaceConfig)

        # create switch to endpoint
        interfaces.append({
            "id": switch["id"] * 6 + 4,
            "handler_id": switch["id"]
            })
        interfaces[-1].update(endpointInterfaceConfig)
        # create endpoint to switch
        interfaces.append({
            "id": switch["id"] * 6 + 5,
            "handler_id": endpoint["id"]
            })
        interfaces[-1].update(endpointInterfaceConfig)

        # create up and right links
        links = []
        # create up link
        if y != n - 1:
            links.append({
                "id": switch["id"] * 3,
                "interfaces": [switch["id"] * 6 + 0, (switch["id"] + 1) * 6 + 2] # switch up, neighbour down
                })
            links[-1].update(linkConfig)
        # create right link
        if x != n - 1:
            links.append({
                "id": switch["id"] * 3 + 1,
                "interfaces": [switch["id"] * 6 + 1, (switch["id"] + n) * 6 + 3] # switch right, neighbour left
                })
            links[-1].update(linkConfig)

        links.append({
            "id": switch["id"] * 3 + 2,
            "interfaces": [switch["id"] * 6 + 4, switch["id"] * 6 + 5] # switch endpoint, endpoint switch
            })
        links[-1].update(endpointLinkConfig)

        config["switches"].append(switch)
        config["endpoints"].append(endpoint)
        config["interfaces"] += interfaces
        config["links"] += links

# create flows
config["flows"] = []
for i in range(len(flowRoutes)):
    route = flowRoutes[i]

    flow = {
            "id": i + 1
            }
    flow.update(route)
    flow.update(flowConfig)
    config["flows"].append(flow)

# print
print(json.dumps(config, indent=4))
