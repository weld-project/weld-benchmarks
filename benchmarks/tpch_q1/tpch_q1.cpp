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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
// #include <omp.h>

#include "weld.h"

// Value for the predicate to pass.
#define PASS 19980901

#ifndef NUM_PARALLEL_THREADS
    #define NUM_PARALLEL_THREADS 4
#endif

// The generated input data.
struct gen_data {
    // Number of lineitems in the table.
    int64_t num_items;
    // Probability that the branch in the query will be taken.
    float prob;
    // The input data.
    struct lineitems *items;
};

// An input data item represented as in a row format.
struct lineitems {
    int8_t *return_flags;
    int8_t *line_statuses;
    float *quantities;
    float *extended_prices;
    float *discounts;
    int32_t *shipdates;
    float *taxes;
};

template <typename T>
struct weld_vector {
    T *data;
    int64_t length;
};

struct args {
    struct weld_vector<int8_t> return_flags;
    struct weld_vector<int8_t> line_statuses;
    struct weld_vector<float> quantities;
    struct weld_vector<float> extended_prices;
    struct weld_vector<float> discounts;
    struct weld_vector<int32_t> shipdates;
    struct weld_vector<float> taxes;
};

struct output {
    int8_t elem1;
    int8_t elem2;
    float elem3;
    float elem4;
    float elem5;
    float elem6;
    float elem7;
    float elem8;
    int32_t elem9;
};

template <typename T>
weld_vector<T> make_weld_vector(T *data, int64_t length) {
    struct weld_vector<T> vector;
    vector.data = data;
    vector.length = length;
    return vector;
}

void run_query_weld(struct gen_data *d) {
    // Compile Weld module.
    weld_error_t e = weld_error_new();
    weld_conf_t conf = weld_conf_new();

    FILE *fptr = fopen("tpch_q1.weld", "r");
    fseek(fptr, 0, SEEK_END);
    int string_size = ftell(fptr);
    rewind(fptr);
    char *program = (char *) malloc(sizeof(char) * (string_size + 1));
    fread(program, sizeof(char), string_size, fptr);
    program[string_size] = '\0';

    weld_module_t m = weld_module_compile(program, conf, e);
    weld_conf_free(conf);
    printf("Done compiling...\n");

    if (weld_error_code(e)) {
        const char *err = weld_error_message(e);
        printf("Error message: %s\n", err);
        exit(1);
    }

   struct args args;
   args.return_flags = make_weld_vector<int8_t>(d->items->return_flags, d->num_items);
   args.line_statuses = make_weld_vector<int8_t>(d->items->line_statuses, d->num_items);
   args.quantities = make_weld_vector<float>(d->items->quantities, d->num_items);
   args.extended_prices = make_weld_vector<float>(d->items->extended_prices, d->num_items);
   args.discounts = make_weld_vector<float>(d->items->discounts, d->num_items);
   args.shipdates = make_weld_vector<int32_t>(d->items->shipdates, d->num_items);
   args.taxes = make_weld_vector<float>(d->items->taxes, d->num_items);
   weld_value_t weld_args = weld_value_new(&args);

   // Run the module and get the result.
    conf = weld_conf_new();
    weld_value_t result = weld_module_run(m, conf, weld_args, e);
    if (weld_error_code(e)) {
        const char *err = weld_error_message(e);
        printf("Error message: %s\n", err);
        exit(1);
    }
    printf("Done running...\n");
    weld_vector<struct output> *result_data = (weld_vector<struct output> *) weld_value_data(result);
    printf("Calling weld_value_data on result: %u\n", result_data->data[0].elem1);

    // Free the values.
    weld_value_free(result);
    weld_value_free(weld_args);
    weld_conf_free(conf);

    weld_error_free(e);
    weld_module_free(m);
}

/** Generates input data.
 *
 * @param num_items the number of line items.
 * @param prob the selectivity of the branch.
 * @return the generated data in a structure.
 */
struct gen_data generate_data(int num_items, float prob) {
    struct gen_data d;

    d.num_items = num_items;
    d.prob = prob;

    d.items = (struct lineitems *)malloc(sizeof(struct lineitems));

    d.items->return_flags = (int8_t *) malloc(sizeof(int8_t) * num_items);
    d.items->line_statuses = (int8_t *) malloc(sizeof(int8_t) * num_items);
    d.items->quantities = (float *) malloc(sizeof(float) * num_items);
    d.items->extended_prices = (float *) malloc(sizeof(float) * num_items);
    d.items->discounts = (float *) malloc(sizeof(float) * num_items);
    d.items->shipdates = (int32_t *) malloc(sizeof(int32_t) * num_items);
    d.items->taxes = (float *) malloc(sizeof(float) * num_items);

    int pass_thres = (int)(prob * 1000000.0);
    srand(1);
    for (int i = 0; i < d.num_items; i++) {
        if (random() % 1000000 <= pass_thres) {
            d.items->shipdates[i] = PASS;
        } else {
            d.items->shipdates[i] = PASS + 1;
        }

        d.items->return_flags[i] = rand();
        d.items->line_statuses[i] = rand();
        d.items->quantities[i] = rand();
        d.items->extended_prices[i] = rand();
        d.items->discounts[i] = rand();
        d.items->taxes[i] = rand();
        // TODO: Figure out how to set return flag and linestatus to ensure that
        // num_buckets constraint is respected.
    }

    return d;
}

int main(int argc, char **argv) {
    // Number of elements in array (should be >> cache size);
    int num_items = (1E8 / sizeof(int));
    // Approx. PASS probability.
    float prob = 0.01;

    int ch;
    while ((ch = getopt(argc, argv, "b:n:p:")) != -1) {
        switch (ch) {
            case 'n':
                num_items = atoi(optarg);
                break;
            case 'p':
                prob = atof(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    // Check parameters.
    assert(num_items > 0);
    assert(prob >= 0.0 && prob <= 1.0);

    struct gen_data d = generate_data(num_items, prob);
    long sum;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    run_query_weld(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Weld: %ld.%06ld (result=0.0)\n",
            (long) diff.tv_sec, (long) diff.tv_usec);


    // TODO: Free d
    return 0;
}
