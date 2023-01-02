/*
 * Original version written by Gina Hortenbach.
 *
 * This code is intended to run on a Raspberry Pi 3 Model B,
 * with the WiringPi Library.
 *
 * The code is currently configured for a dump setup that uses shift
 * registers for address lines and Rapsberry Pi GPIO pins for input data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>


/* Macros */

/* Enable debug */
#define DEBUG               1
/* Enable test commands */
#define TEST                1
#define TEST_ADDR_PINS      0

#if DEBUG
#define UNREACHABLE()                                                       \
    do {                                                                    \
        fprintf(stderr, "UNREACHABLE CODE at %s:%d\n", __FILE__, __LINE__); \
        abort();                                                            \
    } while (0)
#else
#define UNREACHABLE()   do { ; } while (0)
#endif

#define BIT(n)          (1 << (n))

/*
 * I/O Data pins
 *
 * NOTE1 Not sure if it's safe to use UART pins (15, 16) on Pi 3.
 * NOTE2 Pins 8 and 9 (I2C SDA/SCL) have a fixed internal 1.8k ohm pull-up.
 * NOTE3 Pins 30 and 31 are reserved for EEPROM I2C.
 */
#define DQ0             8
#define DQ1             9
#define DQ2             7
#define DQ3             0
#define DQ4             1
#define DQ5             2
#define DQ6             3
#define DQ7             4
/* Number of DQ pins */
#define DQN             8

/* Control pins */
#define OE              5
#define WE              12

/* Address pins (shift registers) */
#define ADDR_DATA       13
#define ADDR_CLOCK      6
/*
 * Address clock pulse, in micro seconds
 *
 * 2 us -> 500 KHz
 * Address shift requires 25 clock pulses = 50 us
 * With 1 extra pulse to read data from flash, the dump period/freq should be:
 * 52 us -> 19.230 KHz
 * Expected time to read 16 MB = 872 s = 14.5 mins
 */
#define ADDR_PULSE_US   2

/*
 * Define according to the size of your target chip
 *
 * NOTE code must be adjusted if MAX_ADDR requires more than 24 address pins.
 */
#if !TEST_ADDR_PINS
#define ADDR_MAX    0xFFFFFF    /* 16 MB */
#else
#define ADDR_MAX    0x000FFF    /* 4 KB */
#endif
#define ADDR_BITS   24


/* Protos */

static void setup(void);
static void set_addr(unsigned addr);
#if TEST
static void input_test(void);
static void output_test(void);
static void io_test(void);
static void oe_test(void);
#endif


/* Functions */

static int dq(int n)
{
    switch (n) {
    case 0:     return DQ0;
    case 1:     return DQ1;
    case 2:     return DQ2;
    case 3:     return DQ3;
    case 4:     return DQ4;
    case 5:     return DQ5;
    case 6:     return DQ6;
    case 7:     return DQ7;
    }

    UNREACHABLE();
    return -1;
}

static void setup_dq(int mode)
{
    int i;

    for (i = 0; i < DQN; i++)
        pinMode(dq(i), mode);
}

static void setup_all(int mode)
{
    setup_dq(mode);

    pinMode(OE, mode);
    pinMode(WE, mode);

    pinMode(ADDR_DATA, mode);
    pinMode(ADDR_CLOCK, mode);
}

#if !TEST_ADDR_PINS

static void setup(void)
{
    setup_dq(INPUT);

    pinMode(WE, OUTPUT);
    digitalWrite(WE, 1);
    pinMode(OE, OUTPUT);
    digitalWrite(OE, 1);

    pinMode(ADDR_DATA, OUTPUT);
    digitalWrite(ADDR_DATA, 0);
    pinMode(ADDR_CLOCK, OUTPUT);
    digitalWrite(ADDR_CLOCK, 0);
}

#endif

#if TEST

static void set_dq(int v)
{
    int i;

    for (i = 0; i < DQN; i++)
        digitalWrite(dq(i), v & BIT(i));
}

#endif

static int get_dq(void)
{
    int i, v;

    v = 0;
    for (i = 0; i < DQN; i++)
        v  |= digitalRead(dq(i)) << i;
    return v;
}

#if !TEST_ADDR_PINS

/* LSB is shifted out first */
static void set_addr(unsigned addr)
{
    int i;

    /* NOTE an extra pulse is needed to latch the last shifted bit */
    for (i = 0; i < ADDR_BITS + 1; i++) {
        digitalWrite(ADDR_CLOCK, 0);
        digitalWrite(ADDR_DATA, addr & 1);
        delayMicroseconds(ADDR_PULSE_US / 2);

        digitalWrite(ADDR_CLOCK, 1);
        delayMicroseconds(ADDR_PULSE_US / 2);
        addr >>= 1;
    }

    digitalWrite(ADDR_CLOCK, 0);
}

#endif

static void get_ids(void)
{
    printf("get_ids:\n");

    setup_all(INPUT);

    /* TODO Get manufacturer id */

    /* TODO Get device id */

    /* Done, setup all pins as inputs, for safety */
    setup_all(INPUT);
}

static void dump(const char *dump_file)
{
    FILE *f;
    int input;
    unsigned addr;

    printf("Dumping NOR Flash from address 0 to 0x%08x...\n", ADDR_MAX);

    /* Open dump file */
    f = fopen(dump_file, "wb");
    if (f == NULL) {
        printf("failed to open %s\n", dump_file);
        exit(1);
    }

    /* Setup pins */
    setup();
    /* Wait for stabilization of signals */
    delayMicroseconds(10);

    /* dump */
    for (addr = 0; addr <= ADDR_MAX; addr++) {
        set_addr(addr);
        /* NOTE set_addr already waits for at least 1 us */

        /* Read data */
        digitalWrite(OE, 0);
        delayMicroseconds(ADDR_PULSE_US / 2);

        input = get_dq();

        digitalWrite(OE, 1);
        delayMicroseconds(ADDR_PULSE_US / 2);

        /* Save data */
        if (fputc(input, f) == EOF) {
            printf("error at addr 0x%x\n", addr);
            exit(1);
        }
    }

    fclose(f);
}

static void usage(void)
{
    printf(
        "nordump ([flag] | <dump_file>)\n"
        "flags:\n"
        "\t-d\tget manufacturer and device ids\n"
#if TEST
        "\t-e\tOE# test\n"
        "\t-i\tinput test\n"
        "\t-o\toutput test\n"
        "\t-y\tinput/output test\n"
#endif
        "\t-I\tsetup all used pins as inputs\n"
    );
    exit(1);
}

int main(int argc, char const *argv[])
{
    char action;
    const char *arg;

    /* Parse args */
    if (argc != 2)
        usage();

    action = 0;
    arg = argv[1];
    if (arg[0] == '-') {
        if (arg[2] != 0)
            usage();

        action = arg[1];
        switch (action) {
        case 'd':
#if TEST
        case 'e':
        case 'i':
        case 'o':
        case 'y':
#endif
        case 'I':
            break;

        default:
            usage();
        }
    }

    /* Init wiringPi */
    if (wiringPiSetup() == -1) {
        printf("failed to initialize wiringPi\n");
        return 1;
    }

    /* Execute selected action */
    switch (action) {
    case 'd':
        get_ids();
        break;

#if TEST
    case 'e':
        oe_test();
        break;

    case 'i':
        input_test();
        break;

    case 'o':
        output_test();
        break;

    case 'y':
        io_test();
        break;
#endif

    case 'I':
        setup_all(INPUT);
        break;

    default:
        dump(arg);
    }

  return 0;
}


/* Test stuff */

#if TEST


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

static void setup(void)
{
    int i;

    setup_all(INPUT);

    for (i = -1; i < AN; i++)
        pinMode(a(i), OUTPUT);
}

static void set_addr(unsigned addr)
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
static void input_test(void)
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
static void output_test(void)
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
static void io_test(void)
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
static void oe_test(void)
{
    printf("oe_test\n");

    setup();

    digitalWrite(OE, 0);
    delay(500);
    digitalWrite(OE, 1);
    delay(500);
}

#endif  /* TEST */
