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
#define PASS 19980901
#define NUM_BUCKETS 6

// A bucket entry.
struct bucket_entry {
    float sum_qty;
    float sum_base_price;
    float sum_disc_price;
    float sum_charge;
    float sum_discount;
    int32_t count;

    // Pad to 32 bytes.
    int8_t _pad[8];
};

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
    float elem1;
    float elem2;
    float elem3;
    float elem4;
    float elem5;
    int32_t elem6;
};

template <typename T>
weld_vector<T> make_weld_vector(T *data, int64_t length) {
    struct weld_vector<T> vector;
    vector.data = data;
    vector.length = length;
    return vector;
}

int32_t run_query(struct gen_data *d) {
    for (int i = 0; i < d->num_items; i++) {
        struct lineitems *items = d->items;
        if (items->shipdates[i] <= PASS) {
            int bucket = (2 * items->return_flags[i]) + items->line_statuses[i];
            struct bucket_entry *e = &d->buckets[bucket];
            e->sum_qty += items->quantities[i];
            e->sum_base_price += items->extended_prices[i];
            float disc_price = (items->extended_prices[i] * (1 - items->discounts[i]));
            e->sum_disc_price += disc_price;
            e->sum_charge +=
                (disc_price * (1 + items->taxes[i]));
            e->sum_discount += items->discounts[i];
            e->count++;
        }
    }
    return d->buckets[0].count + (int32_t) d->buckets[0].sum_discount;
}

int32_t run_query_weld(struct gen_data *d) {
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
    weld_vector<struct output> *result_data = (weld_vector<struct output> *) weld_value_data(result);
    int32_t final_result = result_data->data[0].elem6 + (int32_t) result_data->data[0].elem5;

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
    d.buckets = (struct bucket_entry *)malloc(sizeof(struct bucket_entry) * NUM_BUCKETS);

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
        if (rand() % 1000000 <= pass_thres) {
            d.items->shipdates[i] = PASS;
        } else {
            d.items->shipdates[i] = PASS + 1;
        }

        d.items->return_flags[i] = rand() % 2;
        d.items->line_statuses[i] = rand() % 3;
        d.items->quantities[i] = rand();
        d.items->extended_prices[i] = rand();
        d.items->discounts[i] = rand();
        d.items->taxes[i] = rand();
        // TODO: Figure out how to set return flag and linestatus to ensure that
        // num_buckets constraint is respected.
    }
    memset(d.buckets, 0, sizeof(struct bucket_entry) * NUM_BUCKETS);

    return d;
}

void free_generated_data(struct gen_data *d) {
    free(d->items->return_flags);
    free(d->items->line_statuses);
    free(d->items->quantities);
    free(d->items->extended_prices);
    free(d->items->discounts);
    free(d->items->shipdates);
    free(d->items->taxes);

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
    int32_t result;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    result = run_query_weld(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Weld: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, result);
    free_generated_data(&d);

    d = generate_data(num_items, prob);
    gettimeofday(&start, 0);
    result = run_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Single-threaded C++: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, result);
    free_generated_data(&d);

    return 0;
}
