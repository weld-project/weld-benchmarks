import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.patches as mpatches
import seaborn as sns
sns.set_style('ticks')
sns.set_palette('Paired', 10)
sns.set_context('talk')
sns.set(font_scale=2)
colors = ['#002A5E', '#FD151B', '#8EBA42', '#348ABD', '#988ED5', '#777777', '#8EBA42', '#FFB5B8']

# Helper function to plot results.
def plot(all_times, filename=None):
    plt.figure(figsize=(15, 8))
    xpos = 0.0
    ticks = []
    tick_labels = []
    scheme_ordering = ["Single-threaded C++", "Weld", "Weld compile time"]
    for (benchmark, times) in all_times:
        i = 0
        for scheme in scheme_ordering:
            time_list = times[scheme]
            time = sum(time_list) / len(time_list)
            if scheme == 'Weld compile time':
                bottom = sum(times['Weld']) / len(times['Weld'])
                plt.bar(xpos, time, label=scheme, bottom=bottom, color=colors[i])
            else:
                plt.bar(xpos, time, label=scheme, color=colors[i])
            if scheme != 'Weld':
                xpos += 0.8
            i += 1
        tick_labels.append(benchmark)
        ticks.append(xpos - 1.2)
        xpos += 0.8
    plt.ylabel("Time (in seconds)")
    plt.xlabel("Benchmark")

    patches = []
    labels = []
    for i in range(len(scheme_ordering)):
        patch = mpatches.Patch(color=colors[i])
        labels.append(scheme_ordering[i])
        patches.append(patch)
    leg = plt.figlegend(patches, labels, loc='upper center', ncol=3)

    plt.xticks(ticks, tick_labels)

    if filename is not None:
        with PdfPages(filename) as pdf:
            pdf.savefig(bbox_extra_artists=(leg,), bbox_inches='tight')

    plt.show()
