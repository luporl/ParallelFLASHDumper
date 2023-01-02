#include "test.h"
#include "nordump.h"

#include <stdio.h>
#include <wiringPi.h>


/* Macros */

#if TEST_ADDR_PINS
#define AM1         13
#define A0          6
#define A1          14
#define A2          10
#define A3          11
#define A4          21
#define A5          22
#define A6          26
#define A7          23
#define A8          24
#define A9          27
#define A10         25
#define A11         28
#define A12         29
/* past last address pin */
#define AN          13
#endif


/* Functions */

#if TEST_ADDR_PINS

static int a(int n)
{
    switch (n) {
    case -1:    return AM1;
    case 0:     return A0;
    case 1:     return A1;
    case 2:     return A2;
    case 3:     return A3;
    case 4:     return A4;
    case 5:     return A5;
    case 6:     return A6;
    case 7:     return A7;
    case 8:     return A8;
    case 9:     return A9;
    case 10:    return A10;
    case 11:    return A11;
    case 12:    return A12;
    }

    UNREACHABLE();
    return -1;
}

void setup(void)
{
    int i;

    setup_all(INPUT);

    pinMode(WE, OUTPUT);
    digitalWrite(WE, 1);
    pinMode(OE, OUTPUT);
    digitalWrite(OE, 1);

    for (i = -1; i < AN; i++)
        pinMode(a(i), OUTPUT);

    /* Wait for stabilization of signals */
    delayMicroseconds(10);
}

void set_addr(unsigned addr)
{
    int i;

    for (i = -1; i < AN; i++)
        digitalWrite(a(i), addr & BIT(i + 1));

    delayMicroseconds(ADDR_PULSE_US);
}

#endif  /* TEST_ADDR_PINS */

static void clear_outputs(void)
{
    set_dq(0);

    digitalWrite(OE, 0);
    digitalWrite(WE, 0);

    digitalWrite(ADDR_DATA, 0);
    digitalWrite(ADDR_CLOCK, 0);
}

/*
 * Test data input in DQ0-DQ7.
 *
 * They must be connected to push buttons or something similar.
 */
void input_test(void)
{
    int i, v;

    printf("input_test:\n");

    setup_all(INPUT);

    for (;;) {
        v = get_dq();

        for (i = 0; i < 8; i++)
            printf("%d", v & BIT(i) ? 1 : 0);
        printf("\t0x%02x\n", v);

        delay(1000);
    }
}

/*
 * Test data ouput in DQ0-DQ7, OE, WE, ADDR_DATA and ADDR_CLOCK.
 *
 * They must be connected to LEDs (with proper resistors to limit current) or
 * something similar.
 */
void output_test(void)
{
    int i, pat;

    printf("ouput_test:\n");

    setup_all(OUTPUT);

    for (i = 0; i < 32; i++) {
        /* test all values for each 4 outputs group */
        if (i < 16)
            pat = i | i << 4;
        /* all on, all off */
        else if (i < 24)
            pat = i & 1 ? 0xff : 0;
        /* bit on, bit off */
        else
            pat = i & 1 ? 0xaa : 0x55;

        set_dq(pat);

        digitalWrite(OE, pat & BIT(0));
        digitalWrite(WE, pat & BIT(1));
        digitalWrite(ADDR_DATA, pat & BIT(2));
        digitalWrite(ADDR_CLOCK, pat & BIT(3));
        delay(500);
    }

    clear_outputs();
}

/*
 * Test input and output in DQ0-DQ7.
 *
 * This happens in 3 stages:
 * 1- A short output test is performed
 * 2- Pins are reconfigured as inputs and the following values must be
 *    entered (high 4 bits are currently ignored):
 *    - 0x00
 *    - 0x05
 *    - 0x0a
 * 3- Pins are reconfigured as outputs and leds blink twice to signal the end
 *    of the test.
 *
 * Each tested pin must be connected to a SPDT (single pole double throw)
 * button, where each pole is connected to a resistor to ground or 3.3V, and
 * to an LED that is connected to ground through a resistor (resistor values
 * such as 4k7 doesn't work well, probably because of the internal pull-ups of
 * some pins, but using 300 ohms for all resistors works fine).
 *
 * This is useful to make sure everything is working before trying to get the
 * chip IDs, that needs to send a command (write) and then read the contents
 * back.
 */
void io_test(void)
{
    int i, pat, state, v;

    printf("io_test:\n");

    /* Output test */
    setup_all(OUTPUT);

    for (i = 0; i < 16; i++) {
        /* all on, all off */
        if (i < 8)
            pat = i & 1 ? 0xff : 0;
        /* bit on, bit off */
        else
            pat = i & 1 ? 0xaa : 0x55;

        set_dq(pat);
        delay(500);
    }

    clear_outputs();

    /* Input test */
    delay(500);
    setup_all(INPUT);
    delay(500);

    state = 0;
    while (state < 3) {
        v = get_dq();

        for (i = 0; i < 8; i++)
            printf("%d", v & BIT(i) ? 1 : 0);
        printf("\t0x%02x\n", v);

        if (state == 0 && (v & 0xf) == 0x0)
            state++;
        if (state == 1 && (v & 0xf) == 0x5)
            state++;
        if (state == 2 && (v & 0xf) == 0xa)
            state++;

        delay(1000);
    }

    /* Final output */
    delay(500);
    setup_all(OUTPUT);
    delay(500);

    for (i = 0; i < 2; i++) {
        set_dq(0xff);
        delay(500);
        set_dq(0x00);
        delay(500);
    }
}

/*
 * OE (output enable) test.
 *
 * Test if driving FLASH's OE to HIGH really inhibits output.
 * This is essential to avoid damaging the hardware with get_ids().
 *
 * Any of DQ0-DQ7 must be connected to 2 LEDs, one to ground and another to
 * 3V3. OE is working if one of the LEDs stays on for half a second and both
 * LEDs stay off for another half second.
 */
void oe_test(void)
{
    printf("oe_test\n");

    setup();

    digitalWrite(OE, 0);
    delay(500);
    digitalWrite(OE, 1);
    delay(500);
}

#if !TEST_ADDR_PINS

static void addr_test_check(unsigned i, unsigned exp, unsigned got)
{
    if (got != exp) {
        printf("Test #%02d failed: expected 0x%02x, got 0x%02x\n",
            i, exp, got);
        exit(1);
    }
}

static void addr_shift(unsigned *addr, unsigned bit)
{
    digitalWrite(ADDR_CLOCK, 0);
    digitalWrite(ADDR_DATA, bit);
    delayMicroseconds(ADDR_PULSE_US / 2);

    digitalWrite(ADDR_CLOCK, 1);
    delayMicroseconds(ADDR_PULSE_US / 2);
    *addr <<= 1;

    digitalWrite(ADDR_CLOCK, 0);
}

/*
 * Address shift registers test.
 *
 * To test one address shift register, connect its 8 output pins to DQ0-DQ7.
 */
void addr_test(void)
{
    unsigned addr, i, pat, v;
    static unsigned tvals[8] = {
        0x0f0f0f,
        0x111111,
        0x121212,
        0x747474,
        0x898989,
        0xa5a5a5,
        0xc3c3c3,
        0xf1f1f1
    };

    printf("addr_test\n");

    setup();

    /* Patterns test */
    for (i = 0; i < 32; i++) {
        /* test all values for each 4 outputs group */
        if (i < 16)
            pat = i | i << 4;
        /* all on, all off */
        else if (i < 24)
            pat = i & 1 ? 0xff : 0;
        /* bit on, bit off */
        else
            pat = i & 1 ? 0xaa : 0x55;

        addr = pat | pat << 8 | pat << 16;

        printf("0x%06x\n", addr);
        set_addr(addr);
        delayMicroseconds(ADDR_PULSE_US / 2);

        v = get_dq();
        addr_test_check(i, pat, v);
    }

    /* Bit shift test */
    addr = 0x010101;
    set_addr(addr);
    for (i = 0; i < 8; i++) {
        printf("0x%06x\n", addr);
        delayMicroseconds(ADDR_PULSE_US / 2);
        v = get_dq();
        addr_test_check(i, addr & 0xff, v);
        addr_shift(&addr, 0);
    }

    /* Random values */
    for (i = 0; i < 8; i++) {
        addr = tvals[i];
        printf("0x%06x\n", addr);
        set_addr(addr);
        delayMicroseconds(ADDR_PULSE_US / 2);
        v = get_dq();
        addr_test_check(i, addr & 0xff, v);
    }
}

#endif
