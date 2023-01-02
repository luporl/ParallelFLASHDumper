#ifndef NORDUMP_H
#define NORDUMP_H

#include "cfg.h"

#include <stdlib.h>


/* Macros */

#define BIT(n)          (1 << (n))

#if DEBUG
#define UNREACHABLE()                                                       \
    do {                                                                    \
        fprintf(stderr, "UNREACHABLE CODE at %s:%d\n", __FILE__, __LINE__); \
        abort();                                                            \
    } while (0)
#else
#define UNREACHABLE()   do { ; } while (0)
#endif


/* Functions */

void setup(void);
void setup_all(int mode);

void set_addr(unsigned addr);
void set_dq(int v);
int get_dq(void);

#endif
