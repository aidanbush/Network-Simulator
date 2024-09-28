import matplotlib.pyplot as plt
import numpy as np

fig, ax = plt.subplots(figsize=(10, 3))

# mice flows
max_length = 150
mice_y_start = 0
mice_y_end = 1
mice_length = 20
mice_depth = 6

for i in range(mice_depth):
    for j in range(int(i/mice_depth * mice_length*2), max_length, mice_length*2):
        start = j
        end = start + mice_length
        if end > max_length:
            continue

        # Draw the flow on the same line
        mice_y = mice_y_start + (i/(mice_depth-1)) * (mice_y_end-mice_y_start)
        ax.hlines(y=mice_y, xmin=start, xmax=end, color='black', linewidth=2)

# elephant flows
num_elephants = 2
elephant_flow_start, elephant_flow_end = 0, max_length

# Elephant flows
for i in range(1,num_elephants+1):
    ax.hlines(y=1 + i/(mice_depth-1), xmin=elephant_flow_start, xmax=elephant_flow_end, color='black', linewidth=2)
    ax.hlines(y=1 + i/(mice_depth-1), xmin=elephant_flow_start, xmax=elephant_flow_end, color='black', linewidth=2)

# labels etc
ax.set_title('Elephant and Mice Flows Over Time')
ax.set_yticklabels(["Elephant Flows", "Mice Flows"])
ax.set_yticks([1 + num_elephants/(mice_depth-1), mice_y_end])  # Two y-axis ticks: one for elephant flows, one for mice flows

ax.tick_params(bottom=False, labelbottom=False)
ax.legend().set_visible(False)

# Save plot
plt.savefig("elephant_mice_flows.svg", format="pdf")
