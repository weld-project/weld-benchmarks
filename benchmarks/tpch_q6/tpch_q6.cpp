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
#include <omp.h>

#include "weld.h"

// Value for the predicate to pass.
#define PASS 19940101
#define FAIL 19930101

// The generated input data.
struct gen_data {
    // Number of lineitems in the table.
    int64_t num_items;
    // Probability that the branch in the query will be taken.
    float prob;
    // The input data.
    struct lineitems *items;
    // The hash table.
    struct bucket_entry *buckets;
};

// An input data item represented as in a row format.
struct lineitems {
    int32_t *shipdates;
    float *discounts;
    float *quantities;
    float *extended_prices;
};

template <typename T>
struct weld_vector {
    T *data;
    int64_t length;
};

struct args {
    struct weld_vector<int32_t> shipdates;
    struct weld_vector<float> discounts;
    struct weld_vector<float> quantities;
    struct weld_vector<float> extended_prices;
};

template <typename T>
weld_vector<T> make_weld_vector(T *data, int64_t length) {
    struct weld_vector<T> vector;
    vector.data = data;
    vector.length = length;
    return vector;
}

float run_query(struct gen_data *d) {
    float final_result = 0.0;
    for (int i = 0; i < d->num_items; i++) {
        struct lineitems *items = d->items;
        if (items->shipdates[i] >= 19940101 && items->shipdates[i] < 19950101 &&
            items->discounts[i] >= 5.0 && items->discounts[i] <= 7.0 && items->quantities[i] < 24.0) {
            final_result += (items->discounts[i] * items->extended_prices[i]);
        }
    }
    return final_result;
}

float run_query_weld(struct gen_data *d) {
    // Compile Weld module.
    weld_error_t e = weld_error_new();
    weld_conf_t conf = weld_conf_new();

    FILE *fptr = fopen("tpch_q6.weld", "r");
    fseek(fptr, 0, SEEK_END);
    int string_size = ftell(fptr);
    rewind(fptr);
    char *program = (char *) malloc(sizeof(char) * (string_size + 1));
    fread(program, sizeof(char), string_size, fptr);
    program[string_size] = '\0';

    weld_module_t m = weld_module_compile(program, conf, e);
    weld_conf_free(conf);

    if (weld_error_code(e)) {
        const char *err = weld_error_message(e);
        printf("Error message: %s\n", err);
        exit(1);
    }

   struct args args;
   args.shipdates = make_weld_vector<int32_t>(d->items->shipdates, d->num_items);
   args.discounts = make_weld_vector<float>(d->items->discounts, d->num_items);
   args.quantities = make_weld_vector<float>(d->items->quantities, d->num_items);
   args.extended_prices = make_weld_vector<float>(d->items->extended_prices, d->num_items);

   weld_value_t weld_args = weld_value_new(&args);

   // Run the module and get the result.
    conf = weld_conf_new();
    weld_value_t result = weld_module_run(m, conf, weld_args, e);
    if (weld_error_code(e)) {
        const char *err = weld_error_message(e);
        printf("Error message: %s\n", err);
        exit(1);
    }
    float *result_data = (float *) weld_value_data(result);
    float final_result = *result_data;

    // Free the values.
    weld_value_free(result);
    weld_value_free(weld_args);
    weld_conf_free(conf);

    weld_error_free(e);
    weld_module_free(m);

    return final_result;
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

    d.items->shipdates = (int32_t *) malloc(sizeof(int32_t) * num_items);
    d.items->discounts = (float *) malloc(sizeof(float) * num_items);
    d.items->quantities = (float *) malloc(sizeof(float) * num_items);
    d.items->extended_prices = (float *) malloc(sizeof(float) * num_items);

    int pass_thres = (int)(prob * 1000000.0);
    srand(1);
    for (int i = 0; i < d.num_items; i++) {
        if (rand() % 1000000 <= pass_thres) {
            d.items->shipdates[i] = PASS;
        } else {
            d.items->shipdates[i] = FAIL;
        }

        d.items->discounts[i] = 6.0;
        d.items->quantities[i] = 12.0;
        d.items->extended_prices[i] = rand() % 100;
        // TODO: Figure out how to set return flag and linestatus to ensure that
        // num_buckets constraint is respected.
    }

    return d;
}

void free_generated_data(struct gen_data *d) {
    free(d->items->shipdates);
    free(d->items->discounts);
    free(d->items->quantities);
    free(d->items->extended_prices);

    free(d->items);
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
    float result;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    result = run_query_weld(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Weld: %ld.%06ld (result=%.4f)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, result);
    free_generated_data(&d);

    d = generate_data(num_items, prob);
    gettimeofday(&start, 0);
    result = run_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Single-threaded C++: %ld.%06ld (result=%.4f)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, result);
    free_generated_data(&d);

    return 0;
}
