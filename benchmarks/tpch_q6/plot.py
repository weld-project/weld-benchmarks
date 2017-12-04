from matplotlib import pyplot as plt
plt.style.use('ggplot_light')

import numpy as np

fnames = ['res_predicated.csv', 'res_dynamic_costmodel.csv', 'res_unpredicated.csv']

data = []
plt.figure()
for fname in fnames:
    with open(fname, 'r') as f:
        lines = [l.strip().split(',') for l in f.readlines()]
        lines = [[float(x) for x in l] for l in lines]
        lines = sorted(lines)
        xvals = [l[0] for l in lines]
        yvals = [np.mean(l[1:]) for l in lines]
        plt.plot(xvals, yvals, label=fname, marker='o')

plt.xlabel('Selectivity')
plt.ylabel('Time (s)')
plt.legend()
plt.show()