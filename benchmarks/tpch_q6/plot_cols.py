from matplotlib import pyplot as plt
plt.style.use('ggplot_light')

import numpy as np

fnames = ['predicated_%s.tsv', 'unpredicated_%s.tsv', 'dynamic_%s.tsv']
sel = ['0.01', '0.5']

data = []
plt.figure()
for s in sel:
    for fname in fnames:
        with open(fname % s, 'r') as f:
            lines = [l.strip().split('\t') for l in f.readlines()]
            lines = [[float(x) for x in l] for l in lines]
            xvals = list(range(len(lines)))
            yvals = [l[0] for l in lines]
            plt.plot(xvals, yvals, label=fname, marker='o')

    plt.xlabel('Number of accesses in conditional')
    plt.ylabel('Time (s)')
    plt.legend()
    plt.show()