import matplotlib.pyplot as plt
import numpy as np

fig, ax = plt.subplots(figsize=(10, 3))
file_format = "pdf"

# mice flows
max_length = 100
mice_y_start = 0
mice_y_end = 1
mice_length = 20
mice_depth = 6

for i in range(mice_depth):
    for j in range(int(i/mice_depth * mice_length*2 - mice_length*1.5), max_length, int(mice_length*1.5)):
        start = j #+ np.random.normal(0, 2)
        end = start + mice_length#+ np.random.normal(mice_length, 2)

        # Adjust for partial start
        if start < 0:
            start = 0

        # Adjust for partial end flows
        if end > max_length:
            end = max_length

        # Skip if starts beyond the plot range
        if start > max_length or end < 0:
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

# labels etc
#ax.set_title('Elephant and Mice Flows Over Time')
ax.set_yticklabels(["Elephant Flows", "Mice Flows"])
ax.set_yticks([1 + num_elephants/(mice_depth-1), mice_y_end])  # Two y-axis ticks: one for elephant flows, one for mice flows

ax.set_xlabel("Time")

ax.tick_params(bottom=False, labelbottom=False)
ax.legend().set_visible(False)

# Save plot
plt.savefig(f"elephant_mice_flows.{file_format}", format=file_format)
#plt.show()
