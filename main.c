#define _POSIX_C_SOURCE 200112L

#include <stdio.h>          
#include <stdlib.h>         // atoll, exit
#include <pthread.h>        
#include <string.h>         // strcmp
#include <sys/sysinfo.h>    // get_nprocs

//
// GOAL
//  - Sum up the first N natural numbers as fast as possible.
//

typedef unsigned long long u64;

u64 total = 0;
pthread_mutex_t total_mutex;

// Half open interval: [begin, end)
typedef struct Range {
    u64 begin;
    u64 end;
} Range;

typedef u64 (*ThreadingScheme)(u64);

u64 get_bound_from_args(int, char **);
ThreadingScheme get_threading_scheme(int, char **);

u64 single_thread(u64); // ThreadingScheme
u64 cpu_share(u64);     // ThreadingScheme

const struct {
    const char *name;
    ThreadingScheme scheme;
} threading_schemes[] = {
    {.name="single", .scheme=single_thread},
    {.name="cpus",  .scheme=cpu_share},
};
const size_t threading_schemes_len =
    sizeof(threading_schemes) / sizeof(*threading_schemes);

void *worker(void *);
u64 sum_over(Range *);

int main(int argc, char *argv[])
{
    u64 N = get_bound_from_args(argc, argv);
    ThreadingScheme scheme = get_threading_scheme(argc, argv);

    u64 expected = N * (N + 1) / 2;
    u64 calculated = scheme(N);

    printf("Expected:   %lld\n", expected);
    printf("Calculated: %lld\n", calculated);
}

u64 get_bound_from_args(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(
            stderr,
            "\n"
            "USAGE:\n"
            "    %s n [--method|-m method]\n"
            "WHERE:\n"
            "    n         - the number to sum up to\n"
            "    method, m - the threading method to use.\n"
            "                Options include:\n"
            "        single: computation happens on a single\n"
            "            thread.\n"
            "        cpus: computation is divided equally\n"
            "            among k threads where k is the number\n"
            "            of CPUs available to the process.\n"
            "\n",
            argv[0]
        );
        exit(1);
    }

    long long n = atoll(argv[1]);

    if (n <= 0) {
        fprintf(stderr, "\nERROR: Please supply a positive number.\n\n");
        exit(1);
    }

    return (u64) n;
}

// For convenience.
#define streq(s1, s2) (strcmp(s1, s2) == 0)

ThreadingScheme get_threading_scheme(int argc, char *argv[])
{
    ThreadingScheme scheme = (ThreadingScheme) NULL;
                
    for (int i = 1; i < argc; ++i) {
        if (streq(argv[i], "--method") || streq(argv[i], "-m")) {
            if (++i < argc) {
                for (size_t j = 0; j < threading_schemes_len; ++j) {
                    __auto_type entry = &threading_schemes[j];
                    if (streq(entry->name, argv[i])) {
                       scheme = entry->scheme;
                       break;
                    }
                }

                if (!scheme) {
                    fprintf(
                        stderr,
                        "\nERROR: '%s' is not a recognized threading "
                        "scheme. Try 'single', or 'cpus'.\n\n",
                        argv[i]
                    );
                    exit(1);
                }
                return scheme;
            }
            else {
                fprintf(
                    stderr,
                    "\nERROR: No threading scheme specified after "
                    "%s flag!\n\n",
                    argv[i-1]
                );
                exit(1);
            }
        }
    }

    __auto_type entry = &threading_schemes[0];
    fprintf(
        stderr,
        "\nINFO: Default threading scheme '%s' will be used.\n\n",
        entry->name
    );
    return entry->scheme;
}

u64 single_thread(u64 N)
{
    pthread_t tid;
    Range range = {.begin=0, .end=N+1};
    pthread_create(&tid, NULL, worker, (void *) &range);

    fprintf(stderr, "\nINFO: Spawning 1 thread...\n\n");

    int *retval;
    pthread_join(tid, (void **) &retval);
    fprintf(
        stderr,
        "INFO: Thread exited with status %d.\n\n",
        *retval & 0x7F
    );
    
    return total;
}

// Generates the `i`th subinterval of `*interval` when divided
// into `n` subintervals. Stores the result in `*out`.
void subinterval(Range *interval, Range *out, int n, int i)
{
    u64 delta = (interval->end - interval->begin) / n;
    u64 remainder = (interval->end - interval->begin) % n;
    out->begin = interval->begin + delta * i;
    out->end = out->begin + delta + ((i == n-1) ? remainder : 0);
}

u64 cpu_share(u64 N)
{
    int n_cpus = get_nprocs();
    fprintf(stderr, "\nINFO: Spawning %d threads...\n\n", n_cpus);

    pthread_t tid[n_cpus];
    Range range[n_cpus];
    Range interval = {.begin=1, .end=N+1};

    for (int i = 0; i < n_cpus; ++i) {
        subinterval(&interval, &range[i], n_cpus, i);
        fprintf(
            stderr,
            "INFO: Thread %d gets [%llu, %llu).\n",
            i, range[i].begin, range[i].end
        );
        pthread_create(&tid[i], NULL, worker, (void *) &range[i]);
    }

    printf("\n");

    int *retval;
    for (int i = 0; i < n_cpus; ++i) {
        pthread_join(tid[i], (void **) &retval);
        fprintf(
            stderr,
            "INFO: Thread %d exited with status %d.\n",
            i, *retval & 0x7F
        );
    }

    printf("\n");

    return total;
}

// Thread worker. Loosely wraps the `sum_over` function, but
// handles threading details.
void *worker(void *arg)
{
    Range *range = (Range *) arg;
    u64 acc = sum_over(range);

    pthread_mutex_lock(&total_mutex);
    total += acc;
    pthread_mutex_unlock(&total_mutex);
    
    *(int *) arg = 0;
    pthread_exit(arg);
}

// Computes the sum of the integers in [range->begin, range->end)
// using a for loop.
u64 sum_over(Range *range)
{
    u64 acc = 0;
    for (u64 i = range->begin; i < range->end; ++i)
        acc += i;
    return acc;
}

