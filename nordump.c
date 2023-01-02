/*
 * Original version written by Gina Hortenbach.
 *
 * This code is intended to run on a Raspberry Pi 3 Model B,
 * with the WiringPi Library.
 *
 * The code is currently configured for a dump setup that uses shift
 * registers for address lines and Rapsberry Pi GPIO pins for input data.
 */

#include "nordump.h"
#include "test.h"

#include <stdio.h>
#include <wiringPi.h>


/* Delay time to use in nor read/write functions */
#define NRW_DELAY_US    100


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

void setup_all(int mode)
{
    setup_dq(mode);

    pinMode(OE, mode);
    pinMode(WE, mode);

    pinMode(ADDR_DATA, mode);
    pinMode(ADDR_CLOCK, mode);
}

#if !TEST_ADDR_PINS

void setup(void)
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

    /* Wait for stabilization of signals */
    delayMicroseconds(10);
}

/* MSB is shifted out first */
void set_addr(unsigned addr)
{
    int i;

    /* NOTE an extra pulse is needed to latch the last shifted bit */
    for (i = 0; i < ADDR_BITS + 1; i++) {
        digitalWrite(ADDR_CLOCK, 0);
        digitalWrite(ADDR_DATA, addr & BIT(ADDR_BITS - 1));
        delayMicroseconds(ADDR_PULSE_US / 2);

        digitalWrite(ADDR_CLOCK, 1);
        delayMicroseconds(ADDR_PULSE_US / 2);
        addr <<= 1;
    }

    digitalWrite(ADDR_CLOCK, 0);
}

#endif

void set_dq(int v)
{
    int i;

    for (i = 0; i < DQN; i++)
        digitalWrite(dq(i), v & BIT(i));
}

int get_dq(void)
{
    int i, v;

    v = 0;
    for (i = 0; i < DQN; i++)
        v  |= digitalRead(dq(i)) << i;
    return v;
}

static unsigned nor_read(unsigned addr)
{
    unsigned v;

    setup_dq(INPUT);

    /* Disable DQ output */
    digitalWrite(WE, 1);
    digitalWrite(OE, 1);
    delayMicroseconds(NRW_DELAY_US);

    /* Set addr */
    set_addr(addr);
    delayMicroseconds(NRW_DELAY_US);

    /* Perform read */
    digitalWrite(OE, 0);
    delayMicroseconds(NRW_DELAY_US);
    v = get_dq();
    digitalWrite(OE, 1);
    delayMicroseconds(NRW_DELAY_US);

    return v;
}

static void nor_write(unsigned addr, unsigned data)
{
    /* Disable DQ output */
    digitalWrite(WE, 1);
    digitalWrite(OE, 1);
    delayMicroseconds(NRW_DELAY_US);

    /* Set addr/data */
    setup_dq(OUTPUT);
    set_addr(addr);
    set_dq(data);
    delayMicroseconds(NRW_DELAY_US);

    /* Perform write */
    digitalWrite(WE, 0);                /* latch addr */
    delayMicroseconds(NRW_DELAY_US);
    digitalWrite(WE, 1);                /* latch data */
    delayMicroseconds(NRW_DELAY_US);
}

static void get_ids(void)
{
    printf("get_ids:\n");

    setup();

    /* Enter Autoselect mode */
    nor_write(0xaaa, 0xaa);
    nor_write(0x555, 0x55);
    nor_write(0xaaa, 0x90);

    /* Get manufacturer id */
    printf("Manufacturer ID: 0x%02x\n", nor_read(0x00));

    /* Get device id */
    printf("Device ID:");
    printf(" 0x%02x", nor_read(0x02));
    printf(" 0x%02x", nor_read(0x1c));
    printf(" 0x%02x\n", nor_read(0x1e));

    /* Reset to read mode */
    nor_write(0x00, 0xf0);

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
        "\t-I\tsetup all used pins as inputs\n"
#if TEST
#if !TEST_ADDR_PINS
        "\t-a\taddress test\n"
#endif
        "\t-e\tOE# test\n"
        "\t-i\tinput test\n"
        "\t-o\toutput test\n"
        "\t-y\tinput/output test\n"
#endif
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
#if !TEST_ADDR_PINS
        case 'a':
#endif
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
#if !TEST_ADDR_PINS
    case 'a':
        addr_test();
        break;
#endif

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
