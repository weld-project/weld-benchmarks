from csv import DictWriter as dw

import numpy as np
import subprocess
import sys

cmd = './bench'

p_values = np.linspace(0, 1, 20)
niters = 10
flags = [('n', '1000000')]

def parse_output(output):
    output_lines = output.split("\n")
    times = []
    for output_line in output_lines:
        output_line = output_line.strip()
        if output_line == "":
            continue
        output_line_tokens = output_line.split(": ")
        scheme = output_line_tokens[0]
        time = float(output_line_tokens[1].split()[0])
        times.append((scheme, time))
    return times

all_times = {}
for p in p_values:
    all_times[p] = []
    for n in range(niters):
        times = {}
        
        s = flags + [('p', str(p))]
        flag_settings = ( ' '.join(['-%s %s' % (x[0], str(x[1])) for x in s]))
        cmd = "./bench %s 2>/dev/null" % flag_settings
        print cmd
        output = subprocess.check_output(cmd,
                                         shell=True)
        try:
            output = output.decode('utf-8')
        except:
            pass
        parsed_output = parse_output(output)
        for (scheme, time) in parsed_output:
            if scheme not in times:
                times[scheme] = list()
                times[scheme].append(time)

        all_times[p].append(times[u'Weld'][0])

print all_times
out_fname = 'res_dynamic_costmodel.csv'
outfile = open(out_fname, 'w')
for p, t in sorted(all_times.iteritems()):
    outfile.write(str(p) + ',' + ','.join([str(x) for x in t]) + '\n')

outfile.close()