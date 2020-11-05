import matplotlib.pyplot as plt
import numpy as np
import sys

# open weights file
weightFile = sys.argv[1]
tiles = int(sys.argv[2])
tilings = int(sys.argv[3])

# get weights
with open(weightFile) as f:
    weights = list(map(float, f.readline().split(',')))

# get all possible values for given weights
def getValues(tiles, tilings, weights, weightOffset):
    values = []
    for i in range(tiles*tilings - tilings + 1):
        state=[]
        for j in range(tilings):
            state.append(j*tiles + (i+j)//tilings);

        values.append(sum(weights[k + weightOffset] for k in state))

    return values

# for each action

actions = ["multiply", "divide", "add", "subtract"]
values = []

if len(weights) != tiles * tilings * len(actions):
    print("Specified size does not match file")
    exit()

for i in range(4):
    values.append(getValues(tiles, tilings, weights, i*tiles*tilings))

print(values)

fig, ax = plt.subplots()
im = ax.imshow(values)

ax.set_xticks(np.arange(len(values[0])))
ax.set_yticks(np.arange(len(actions)))

ax.set_yticklabels(actions)
ax.set_xticklabels([i / (len(values[0])-1) for i in range(len(values[0]))])

for i in range(len(actions)):
    for j in range(len(values[0])):
        text = ax.text(j, i, "{:.2f}".format(values[i][j]),
                       ha="center", va="center", color="w", fontsize=8)

ax.set_title("action values")
fig.tight_layout()
plt.show()
