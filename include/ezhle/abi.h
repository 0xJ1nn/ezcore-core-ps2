#ifndef EZHLE_ABI_H
#define EZHLE_ABI_H

#define EZHLE_ABI_VERSION "ezhle-2"

typedef enum {
    EZHLE_OK = 0,
    EZHLE_ERR_UNIMPL = 1,
    EZHLE_ERR_INVALID_ARG = 2
} ezhle_err_t;

#endif /* EZHLE_ABI_H */
