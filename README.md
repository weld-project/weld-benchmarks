# Weld benchmarks

This repository tries to run benchmarks for the Weld project in an easy-to-reproduce
way.

## Key requirements

- Each benchmark is in a separate directory under `benchmarks/`.

- All benchmark binaries *have to* to be named `bench` in the corresponding directory
  (if compilation is required, the output binary produced by `make` should be called
  `bench`).

- In addition, each benchmark binary is responsible for its own timing; all timing
  information needs to be printed to `stdout` in the following format,
  ```
  <Experiment description>: <Time> <Other metadata>
  ```
  Each benchmark binary can print multiple pieces of timing information.
  
  To make this more concrete, consider the output of the matrix multiplication
  benchmark,
  ```bash
  $ ./bench
  Transposed: 0.028259 (result=38238491904)
  Unblocked: 0.029373 (result=38288406288)
  Blocked: 0.028529 (result=38228352573)
  ```
  
- All parameters need to be passed into the `bench` binary in the form
  `-<parameter name> <parameter value>`.

- Benchmark directories must include a configuration file named "config.json".

  A sample configuration file looks like this:
  ```json
  {
      "compile": true,
      "params":
      {
        "b": [64, 128]
      },
      "scaled_params":
      {
        "n": [512, 1024]
      }
  }
  ```

  The `params` field specifies the parameters that need to be swept over.

  The `compile` field is a `true/false` field and specifies whether workloads need to
  be compiled beforehand using `make` or not.

- Benchmark directories may additionally contain a Python model
for the benchmark cost, in a file called "model_cost.py". It should
support the same parameter flags as the benchmark itself.

## Running instructions

The main script is `run_benchmarks.py` in the root directory. It takes the following
arguments:
- `-n / --num_iterations`: Specifies the number of trials for each benchmark.
- `-s / --scale_factor`: Specifies the factor by which scaled parameters need to be scaled.
- `-f / --csv_filename`: Specifies the output file for dumped experiment results.
- `-b / --benchmarks`: Comma-separated list of benchmarks that should be run (must be
  a subset of benchmarks listed in the configuration file).
- `-m / --cost_model`: A flag specifying whether the cost model should be executed in place of the benchmark.
- `-v / --verbose`: A flag specifying whether to print verbose statistics.

Sample output looks like this:
```bash
$ python run_benchmarks.py --config config.json -n 2 -f output.csv
++++++++++++++++++++++++++++++++++++++
matrix_multiplication
++++++++++++++++++++++++++++++++++++++
b = 64, n = 512
Transposed: 0.2533 +/- 0.0017 seconds
Unblocked: 0.3360 +/- 0.0035 seconds
Blocked: 0.1607 +/- 0.0002 seconds

b = 64, n = 1024
Transposed: 2.0677 +/- 0.0004 seconds
Unblocked: 13.0135 +/- 1.2649 seconds
Blocked: 2.0570 +/- 0.0070 seconds

b = 128, n = 512
Transposed: 0.2546 +/- 0.0032 seconds
Unblocked: 0.3312 +/- 0.0066 seconds
Blocked: 0.2699 +/- 0.0033 seconds

b = 128, n = 1024
Transposed: 2.0920 +/- 0.0211 seconds
Unblocked: 12.2830 +/- 1.5111 seconds
Blocked: 2.0394 +/- 0.0054 seconds

++++++++++++++++++++++++++++++++++++++
swapped_loops
++++++++++++++++++++++++++++++++++++++
k = 1, n = 1000
Cached: 0.0002 +/- 0.0000 seconds
Original: 0.0002 +/- 0.0000 seconds
Interchanged: 0.0003 +/- 0.0000 seconds

k = 1, n = 10000
Cached: 0.0017 +/- 0.0000 seconds
Original: 0.0018 +/- 0.0001 seconds
Interchanged: 0.0032 +/- 0.0001 seconds

k = 10, n = 1000
Cached: 0.0017 +/- 0.0000 seconds
Original: 0.0018 +/- 0.0003 seconds
Interchanged: 0.0027 +/- 0.0004 seconds

k = 10, n = 10000
Cached: 0.0166 +/- 0.0004 seconds
Original: 0.0171 +/- 0.0008 seconds
Interchanged: 0.0263 +/- 0.0027 seconds

```
