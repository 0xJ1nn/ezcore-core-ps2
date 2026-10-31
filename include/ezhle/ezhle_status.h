/* Result codes shared by this repository's modules.
 * EZHLE_ERR_UNIMPL is the contract for any behaviour that is not documented in
 * docs/SOURCES.md: report it, never guess it. */
#ifndef EZHLE_EZHLE_STATUS_H
#define EZHLE_EZHLE_STATUS_H

typedef enum {
    EZHLE_OK = 0,
    EZHLE_ERR_UNIMPL = -1,
    EZHLE_ERR_INVAL = -2
} ezhle_status;

#endif /* EZHLE_EZHLE_STATUS_H */
