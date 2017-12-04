/**
 * q1.c
 *
 * A test for varying parameters for queries similar to Q1.
 *
 */

#ifdef __linux__
#define _BSD_SOURCE 500
#define _POSIX_C_SOURCE 2
#endif

#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <omp.h>

#include <vector>
#include <random>
#include <boost/random/uniform_int_distribution.hpp>
static std::default_random_engine generator( time( NULL ) ^ getpid() );

using namespace std;

int uniform_int( int range ) {
  boost::random::uniform_int_distribution<> rand( 0, range );
  return rand( generator );
}

#include "weld.h"

// Value for the predicate to pass.
#define PASS 19940101.0
#define FAIL 19930101.0

// The generated input data.
class gen_data {
public:
  // Number of lineitems in the table.
  int64_t num_items_;
  int64_t num_columns_;
  // Probability that the branch in the query will be taken.
  float prob_;
  
  vector<vector<double>> items_;

  gen_data(int num_items, int num_cols, float prob) :
    num_items_( num_items ),
    num_columns_( num_cols ),
    prob_( prob ),
    items_() {
    for ( int i = 0; i < num_cols; i++ ) {
      items_.push_back( vector<double>() );
    }

    int pass_thres = (int)(prob * 1000000.0);
    srand(1);
    for (int i = 0; i < num_items; i++) {
      if (rand() % 1000000 <= pass_thres) {
        items_[0].push_back(PASS);
      } else {
        items_[0].push_back(FAIL);
      }
    }
    
    for (int j = 1; j < num_cols; j++) {
      for (int i = 0; i < num_items; i++) {
        items_[j].push_back(10.0);
      }
    }
  }
};

template <typename T>
struct weld_vector {
    T *data;
    int64_t length;
};

template <typename T>
weld_vector<T> make_weld_vector(T *data, int64_t length) {
    struct weld_vector<T> vector;
    vector.data = data;
    vector.length = length;
    return vector;
}

double run_query(int num_cond, struct gen_data *d) {
    double final_result = 0.0;

    for (int i = 0; i < d->num_items_; i++) {
      bool pass = true;
      if ( d->items_[0][i] >= 19940101 && d->items_[0][i] < 19950101 ) {
        for ( int j = 1; j < num_cond; j++ ) {
          if ( d->items_[j][i] < 9.0 ) {
            pass = false;              
          }
        }
      } else {
        pass = false;
      }

      if ( pass ) {
        for ( int j = 1; j < d->num_columns_; j++ ) {
          final_result += d->items_[j][i];
        }
      }
    }
    return final_result;
}

double run_query_weld(const char* fname, struct gen_data *d) {
    // Compile Weld module.
    weld_error_t e = weld_error_new();
    weld_conf_t conf = weld_conf_new();
    //weld_conf_set(conf, "weld.compile.traceExecution", "true");
      
    FILE *fptr = fopen(fname, "r");
    fseek(fptr, 0, SEEK_END);
    int string_size = ftell(fptr);
    rewind(fptr);
    char *program = (char *) malloc(sizeof(char) * (string_size + 1));
    fread(program, sizeof(char), string_size, fptr);
    program[string_size] = '\0';

    struct timeval start, end, diff;
    gettimeofday(&start, 0);
    weld_module_t m = weld_module_compile(program, conf, e);
    weld_conf_free(conf);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    //printf("Weld compile time: %ld.%06ld\n",
    //       (long) diff.tv_sec, (long) diff.tv_usec);

    if (weld_error_code(e)) {
        const char *err = weld_error_message(e);
        printf("Error message: %s\n", err);
        exit(1);
    }

   gettimeofday(&start, 0);

   vector<weld_vector<double>> args;
   for ( uint32_t i = 0; i < d->num_columns_; i++ ) {
     args.push_back(make_weld_vector<double>(d->items_[i].data(), d->num_items_));
   }

   weld_value_t weld_args = weld_value_new(args.data());

   // Run the module and get the result.
    conf = weld_conf_new();
    weld_value_t result = weld_module_run(m, conf, weld_args, e);
    if (weld_error_code(e)) {
        const char* err = weld_error_message(e);
        printf("Error message: %s\n", err);
        exit(1);
    }
    double* result_data = (double*) weld_value_data(result);
    double final_result = *result_data;

    // Free the values.
    weld_value_free(result);
    weld_value_free(weld_args);
    weld_conf_free(conf);

    weld_error_free(e);
    weld_module_free(m);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    // printf("Weld: %ld.%06ld (result=%.4f)\n",
    printf("%ld.%06ld\t%.4f\n", 
           (long) diff.tv_sec, (long) diff.tv_usec, final_result);

    return final_result;
}

int main(int argc, char **argv) {
    // Number of elements in array (should be >> cache size);
    int num_items = (1E8 / sizeof(int));
    // Approx. PASS probability.
    float prob = 0.01;
    // Total number of columns.
    int num_cols = 10;
    // Number of columns accessed in conditional.
    int num_cond = 10;
    // Weld code to run.
    std::string fname = "generated.weld";

    int ch;
    while ((ch = getopt(argc, argv, "n:c:m:p:f:")) != -1) {
      switch (ch) {
      case 'n':
        num_items = atoi(optarg);
        break;
      case 'c':
        num_cols = atoi(optarg);
        break;
      case 'm':
        num_cond = atoi(optarg);
        break;
      case 'p':
        prob = atof(optarg);
        break;
      case 'f':
        fname = optarg;
        break;
      case '?':
      default:
        fprintf(stderr, "invalid options");
        exit(1);
      }
    }
    
    // Check parameters.
    assert(num_items > 0);
    assert(num_cols > 0);
    assert(num_cond > 0);
    assert(prob >= 0.0 && prob <= 1.0);

    struct gen_data d(num_items, num_cols, prob);
    double result;
    struct timeval start, end, diff;

    // gettimeofday(&start, 0);
    // result = run_query(num_cond, &d);
    // gettimeofday(&end, 0);
    // timersub(&end, &start, &diff);
    // printf("Single-threaded C++: %ld.%06ld (result=%.4f)\n",
    //         (long) diff.tv_sec, (long) diff.tv_usec, result);

    d = gen_data(num_items, num_cols, prob);
    result = run_query_weld(fname.c_str(), &d);

    return 0;
}
