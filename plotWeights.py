import matplotlib.pyplot as plt
import numpy as np

# open weights file
weightFile = ""

tiles = 11
tilings = 1

# get weights
weights = [-0.87729,-0.0399035,-0.00447566,0.00751508,0.247738,-0.00802878,0.38742,-0.00545796,0.362292,-0.0314865,0,-0.332682,1.30092,1.03629,0,0,0.0307052,0,0.254945,0.0143942,1.71139,0,4.69196,0.0262032,0.0306627,0.527858,0,0.214878,0,0,0,-0.0615549,0,-0.197671,-0.0149074,0.0255301,0,0,0.0208025,0,0,0.0136502,-0.0347894,0]

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
                       ha="center", va="center", color="w")

ax.set_title("state values")
fig.tight_layout()
plt.show()
