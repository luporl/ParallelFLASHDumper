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
 * On Pi 3 and 4 UART pins (15, 16) are used for bluetooth by default.
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
#define ADDR_MAX    0xFFFFFF    /* 16 MB */
#define ADDR_BITS   24


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

static void clear_outputs(void)
{
    set_dq(0);

    digitalWrite(OE, 0);
    digitalWrite(WE, 0);

    digitalWrite(ADDR_DATA, 0);
    digitalWrite(ADDR_CLOCK, 0);
}

static void input_test(void)
{
    printf("input_test:\n");

    setup(INPUT);
}

static void output_test(void)
{
    int i, pat;

    printf("ouput_test:\n");

    setup(OUTPUT);

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

static void dump(const char *dump_file)
{
    FILE *f;
    unsigned char input;
    unsigned addr;

    /* TODO implement and test flash dump */
    printf("Dumping NOR Flash from address 0 to 0x%08x...\n", ADDR_MAX);
    printf("TODO\n");
    exit(1);

    /* Open dump file */
    f = fopen(dump_file, "wb");
    if (f == NULL) {
        printf("failed to open %s\n", dump_file);
        exit(1);
    }

    /* Setup pins */
    setup(INPUT);

    digitalWrite(WE, 1);
    digitalWrite(OE, 1);

    digitalWrite(ADDR_DATA, 0);
    digitalWrite(ADDR_CLOCK, 0);

    delayMicroseconds(10);

    /* dump */
    for (addr = 0; addr <= ADDR_MAX; addr++) {
        set_addr(addr);
        /* NOTE set_addr already waits for at least 1 us */

        /* Read data */
        digitalWrite(OE, 0);
        delayMicroseconds(ADDR_PULSE_US / 2);

        input  = digitalRead(DQ0);
        input |= digitalRead(DQ1) << 1;
        input |= digitalRead(DQ2) << 2;
        input |= digitalRead(DQ3) << 3;
        input |= digitalRead(DQ4) << 4;
        input |= digitalRead(DQ5) << 5;
        input |= digitalRead(DQ6) << 6;
        input |= digitalRead(DQ7) << 7;

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
        "\t-i\tinput test\n"
        "\t-o\toutput test\n"
        "\t-s\tsetup pins\n"
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
        case 'i':
        case 'o':
        case 's':
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
    case 'i':
        input_test();
        break;

    case 'o':
        output_test();
        break;

    case 's':
        setup(INPUT);
        break;

    default:
        dump(arg);
    }

  return 0;
}
