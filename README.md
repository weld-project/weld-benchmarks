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

## Running instructions

The main script is `run_benchmarks.py` in the root directory. It takes the following
arguments:
- `-n / --num_iterations`: Specifies the number of trials for each benchmark.
- `-s / --scale_factor`: Specifies the factor by which scaled parameters need to be scaled.
- `-f / --csv_filename`: Specifies the output file for dumped experiment results.
- `-b / --benchmarks`: Comma-separated list of benchmarks that should be run (must be
  a subset of benchmarks listed in the configuration file).
- `-v / --verbose`: A flag specifying whether to print verbose statistics.

Sample output looks like this:
```bash
$ python run_benchmarks.py -b crime_index -n 5 -f results.csv -v
++++++++++++++++++++++++++++++++++++++
crime_index
++++++++++++++++++++++++++++++++++++++
s=1, f=data/us_cities_states_counties_sf=%d.csv
Grizzly: 1.4256 +/- 0.0102 seconds
Pandas: 0.0229 +/- 0.0009 seconds

s=10, f=data/us_cities_states_counties_sf=%d.csv
Grizzly: 2.1567 +/- 0.2303 seconds
Pandas: 0.0725 +/- 0.0066 seconds

s=100, f=data/us_cities_states_counties_sf=%d.csv
Grizzly: 6.8954 +/- 0.2109 seconds
Pandas: 0.6040 +/- 0.0640 seconds

```
