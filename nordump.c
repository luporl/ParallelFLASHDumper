/*
 * Original version written by Gina Hortenbach.
 *
 * This code is intended to run a Raspberry Pi 3 Model B,
 * with the WiringPi Library.
 *
 * The code is currently configured for a dump setup that uses shift
 * registers for address lines and Rapsberry Pi GPIO pins for input data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>


/* Macros */

#define BIT(n)          (1 << (n))

/*
 * I/O Data pins
 *
 * NOTE1 On Pi 3 and 4 UART pins (15, 16) are used for bluetooth by default.
 *
 * NOTE2 Pins 8 and 9 (I2C SDA/SCL) have internal 1.8k ohm pull-up to 3v3.
 */
#define DQ0             8
#define DQ1             9
#define DQ2             7
#define DQ3             0
#define DQ4             1
#define DQ5             2
#define DQ6             3
#define DQ7             4

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
#if 0
#define ADDR_MAX    0xFFFFFF    /* 16 MB */
#endif
#define ADDR_MAX    0x000FFF    /* 4 KB */
#define ADDR_BITS   24

#define AM1         13
#define A0          6
#define A1          14
#define A2          10
#define A3          11
#define A4          30
#define A5          31
#define A6          21
#define A7          22
#define A8          26
#define A9          23
#define A10         24


/* Functions */

static void setup_dq(int mode)
{
    pinMode(DQ0, mode);
    pinMode(DQ1, mode);
    pinMode(DQ2, mode);
    pinMode(DQ3, mode);
    pinMode(DQ4, mode);
    pinMode(DQ5, mode);
    pinMode(DQ6, mode);
    pinMode(DQ7, mode);
}

static void setup(int dq_mode)
{
    /* Set pin modes */
    setup_dq(dq_mode);

    pinMode(OE, OUTPUT);
    pinMode(WE, OUTPUT);

    pinMode(ADDR_DATA, OUTPUT);
    pinMode(ADDR_CLOCK, OUTPUT);
}

static void setup_all(int mode)
{
    setup_dq(mode);

    pinMode(OE, mode);
    pinMode(WE, mode);

    pinMode(ADDR_DATA, mode);
    pinMode(ADDR_CLOCK, mode);

    if (mode == INPUT) {
        pinMode(AM1, mode);
        pinMode(A0, mode);
        pinMode(A1, mode);
        pinMode(A2, mode);
        pinMode(A3, mode);
        pinMode(A4, mode);
        pinMode(A5, mode);
        pinMode(A6, mode);
        pinMode(A7, mode);
        pinMode(A8, mode);
        pinMode(A9, mode);
        pinMode(A10, mode);
    }
}

static void set_dq(int v)
{
    digitalWrite(DQ0, v & BIT(0));
    digitalWrite(DQ1, v & BIT(1));
    digitalWrite(DQ2, v & BIT(2));
    digitalWrite(DQ3, v & BIT(3));
    digitalWrite(DQ4, v & BIT(4));
    digitalWrite(DQ5, v & BIT(5));
    digitalWrite(DQ6, v & BIT(6));
    digitalWrite(DQ7, v & BIT(7));
}

static int get_dq(void)
{
    int v;

    v  = digitalRead(DQ0);
    v |= digitalRead(DQ1) << 1;
    v |= digitalRead(DQ2) << 2;
    v |= digitalRead(DQ3) << 3;
    v |= digitalRead(DQ4) << 4;
    v |= digitalRead(DQ5) << 5;
    v |= digitalRead(DQ6) << 6;
    v |= digitalRead(DQ7) << 7;

    return v;
}

static void clear_outputs(void)
{
    set_dq(0);

    digitalWrite(OE, 0);
    digitalWrite(WE, 0);

    digitalWrite(ADDR_DATA, 0);
    digitalWrite(ADDR_CLOCK, 0);
}

/* LSB is shifted out first */
static void set_addr(unsigned addr)
{
#if 0
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
#else
    digitalWrite(AM1, addr & BIT(0));
    digitalWrite(A0, addr & BIT(1));
    digitalWrite(A1, addr & BIT(2));
    digitalWrite(A2, addr & BIT(3));
    digitalWrite(A3, addr & BIT(4));
    digitalWrite(A4, addr & BIT(5));
    digitalWrite(A5, addr & BIT(6));
    digitalWrite(A6, addr & BIT(7));
    digitalWrite(A7, addr & BIT(8));
    digitalWrite(A8, addr & BIT(9));
    digitalWrite(A9, addr & BIT(10));
    digitalWrite(A10, addr & BIT(11));

    delayMicroseconds(ADDR_PULSE_US);
#endif
}

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

static void get_ids(void)
{
    printf("get_ids:\n");

    setup_all(INPUT);

    /* Done, setup all pins as inputs, for safety */
    setup_all(INPUT);
}

static void dump(const char *dump_file)
{
    FILE *f;
    int input, mode;
    unsigned addr;

    /* TODO implement and test flash dump */
    printf("Dumping NOR Flash from address 0 to 0x%08x...\n", ADDR_MAX);

    /* Open dump file */
    f = fopen(dump_file, "wb");
    if (f == NULL) {
        printf("failed to open %s\n", dump_file);
        exit(1);
    }

    /* Setup pins */
    /* setup(INPUT); */
    setup_all(INPUT);

    /* mode = INPUT; */
    mode = OUTPUT;
    pinMode(AM1, mode);
    pinMode(A0, mode);
    pinMode(A1, mode);
    pinMode(A2, mode);
    pinMode(A3, mode);
    pinMode(A4, mode);
    pinMode(A5, mode);
    pinMode(A6, mode);
    pinMode(A7, mode);
    pinMode(A8, mode);
    pinMode(A9, mode);
    pinMode(A10, mode);

    pinMode(WE, OUTPUT);
    digitalWrite(WE, 1);
    delayMicroseconds(10);

    pinMode(OE, OUTPUT);
    digitalWrite(OE, 1);
    delayMicroseconds(10);

#if 0
    /* OE test */
    digitalWrite(OE, 0);
    delay(500);
    digitalWrite(OE, 1);
    delay(500);
#endif

    /*
    digitalWrite(ADDR_DATA, 0);
    digitalWrite(ADDR_CLOCK, 0);
    */

    delayMicroseconds(10);

    /*
    printf("TODO\n");
    exit(1);
    */

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
        "\t-i\tinput test\n"
        "\t-o\toutput test\n"
        "\t-s\tsetup pins\n"
        "\t-y\tinput/output test\n"
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
        case 'i':
        case 'o':
        case 's':
        case 'y':
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

    case 'i':
        input_test();
        break;

    case 'o':
        output_test();
        break;

    case 's':
        setup(INPUT);
        break;

    case 'y':
        io_test();
        break;

    case 'I':
        setup_all(INPUT);
        break;

    default:
        dump(arg);
    }

  return 0;
}
