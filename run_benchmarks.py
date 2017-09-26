import argparse
import csv
import itertools
import json
import math
import numpy as np
import subprocess
import sys

import utils

def labeled_params(param_dict):
    ''' {p:[v]} -> [(p, v1), ..., (p, vn)] '''
    ret = []
    for p, vlist in param_dict.items():
        p_list = []
        for val in vlist:
            p_list.append((p, val))
        ret.append(p_list)
    return ret
        
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

def run_benchmark(benchmark, num_iterations, csv_filename, default, verbose):
    if verbose:
        print("++++++++++++++++++++++++++++++++++++++")
        print(benchmark)
        print("++++++++++++++++++++++++++++++++++++++")

    b_config = read_config('benchmarks/%s/config.json' % benchmark)

    if b_config['compile'] == True:
        compile_benchmark(benchmark)

    if default:
        params = b_config.get('default_params', {})
        params = {key: [value] for (key, value) in params.items()}
    else:
        params = b_config.get('params', {}).copy() ## we'll mutate this with scaled values ##
        params = expand_params(params)

        scaled_params = b_config.get('scaled_params', {})
        scaled_params = expand_params(scaled_params)
        scaled_params = scale_params(scaled_params, scale_factor)

        params.update(scaled_params)

    csvf = open(csv_filename, 'a+')
    writer = csv.writer(csvf, delimiter='\t')
    logfile = "benchmarks/%s/output.log" % benchmark

    with open(logfile, 'w') as nf:
        nf.write("++++++++++++++++++++++++++++++++++++++\n")
        nf.write(benchmark + "\n")
        nf.write("++++++++++++++++++++++++++++++++++++++\n\n")

    all_times = list()
    param_settings = itertools.product(*labeled_params(params))
    for s in param_settings:
        log_settings  = (', '.join(['%s=%s'  % (x[0], str(x[1])) for x in s]))
        csv_settings  = ( ';'.join(['%s=%s'  % (x[0], str(x[1])) for x in s]))
        flag_settings = ( ' '.join(['-%s %s' % (x[0], str(x[1])) for x in s]))
        
        if verbose:
            print(log_settings)

        with open(logfile, 'a') as nf:
            nf.write(log_settings)
            nf.write("\n")

        times = {}
        for i in range(num_iterations):
            output = subprocess.check_output("cd benchmarks/%s; ./bench %s 2>/dev/null"
                                             % (benchmark, flag_settings),
                                             shell=True)
            try:
                output = output.decode('utf-8')
            except:
                pass
            with open(logfile, 'a') as nf:
                nf.write(output)
                nf.write("\n")
            parsed_output = parse_output(output)
            for (scheme, time) in parsed_output:
                if scheme not in times:
                    times[scheme] = list()
                times[scheme].append(time)

        for scheme in times:
            row = [benchmark, scheme, log_settings]
            if verbose:
                time_mean = np.mean(times[scheme])
                time_stddev = np.std(times[scheme])
                print("%s: %.4f +/- %.4f seconds" % (scheme, time_mean, time_stddev))
                sys.stdout.flush()
            row.extend([str(elem) for elem in times[scheme]])
            writer.writerow(row)
        if verbose:
            print("\n")
        all_times.append(times)
    
    csvf.close()
    return all_times

def read_config(config_file):
    return json.load(open(config_file, 'r'))

def compile_benchmark(benchmark):
    subprocess.call("make -C benchmarks/%s" % benchmark, shell=True)
 
def scale_params(scaled_params_dict, scale_factor):
    ret = {}
    for p, vlist in scaled_params_dict.items():
        ret[p] = [v*scale_factor for v in vlist]
    return ret

def expand_params(params_dict):
    ''' if params specified by a range, unroll the range into values '''
    ret = {}
    for p, v in params_dict.items():
        if isinstance(v, dict):
            if v['scale'] == 'linear':
                ret[p] = np.linspace(v['start'], v['stop'], v['n'])
            elif v['scale'].startswith('log'):
                base = int(v['scale'][3:])
                ret[p] = np.logspace(math.log(v['start'], base),
                                     math.log(v['stop'], base),
                                     v['n'], base=base)
            else:
                raise ValueError

            if v['type'] == 'int':
                ret[p] = [int(x) for x in ret[p]]
        elif isinstance(v, list):
            ret[p] = v
        else:
            raise ValueError
    return ret
        
if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Run the performance suite for the passed in benchmarks"
    )
    parser.add_argument('-n', "--num_iterations", type=int, required=True,
                        help="Number of iterations to run each benchmark")
    parser.add_argument('-s', "--scale_factor", type=int, default=1,
                        help="Scale factor for scaled parameters")
    parser.add_argument('-f', "--csv_filename", type=str, required=True,
                        help="Name of CSV to dump output in")
    parser.add_argument('-b', "--benchmarks", type=str, default=None, nargs='+',
                        help="List of benchmarks to run")
    parser.add_argument('-v', "--verbose", action='store_true',
                        help="Output verbose statistics")
    parser.add_argument('-d', "--default", action='store_true',
                        help="Use default arguments for every binary")
    parser.add_argument('-p', "--plot_filename", type=str, default=None,
                        help="Plot filename")

    cmdline_args = parser.parse_args()
    opt_dict = vars(cmdline_args)

    num_iterations = opt_dict["num_iterations"]
    scale_factor = opt_dict["scale_factor"]
    csv_filename = opt_dict["csv_filename"]
    default = opt_dict["default"]
    verbose = opt_dict["verbose"]
    open(csv_filename, 'w').close() ## erase current contents ##

    csvf = open(csv_filename, 'a+')
    writer = csv.writer(csvf, delimiter='\t')
    row = ["Benchmark", "Scheme", "Parameters"]
    for i in range(num_iterations):
        row.append("Trial %d" % (i + 1))
    writer.writerow(row)
    csvf.close()

    benchmarks = opt_dict["benchmarks"]

    all_times = []
    for benchmark in benchmarks:
        times = run_benchmark(benchmark, num_iterations, csv_filename, default, verbose)
        all_times.append((benchmark, times[0]))  # Only consider first parameter for plotting

    plot_filename = opt_dict["plot_filename"]
    if plot_filename is not None:
        utils.plot(all_times, plot_filename)
