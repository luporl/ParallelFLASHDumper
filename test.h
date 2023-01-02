#ifndef TEST_H
#define TEST_H

#include "cfg.h"

#if TEST
void input_test(void);
void output_test(void);
void io_test(void);
void oe_test(void);
#if !TEST_ADDR_PINS
void addr_test(void);
#endif
#endif

#endif
