//https://github.com/wolkykim/qlibc
#ifndef TOKENBUCKET_H
#define TOKENBUCKET_H

#include <stdbool.h>

/* types */
typedef struct qtokenbucket_s qtokenbucket_t;

/* public functions */
extern void qtokenbucket_init(qtokenbucket_t *bucket, int init_tokens,
                              int max_tokens, int tokens_per_sec);
extern bool qtokenbucket_consume(qtokenbucket_t *bucket, int tokens);
extern long qtokenbucket_waittime(qtokenbucket_t *bucket, int tokens);

/**
 * qtokenbucket internal data structure
 */
struct qtokenbucket_s {
    double tokens; /*!< current number of tokens. */
    int max_tokens; /*!< maximum number of tokens. */
    int tokens_per_sec; /*!< fill rate per second. */
    long last_fill; /*!< last refill time in Millisecond. */
};


#endif /* TOKENBUCKET_H */