/*
 * Original version written by Gina Hortenbach.
 *
 * This code is intended to run on the Raspberry Pi with the WiringPi Library.
 *
 * The code is currently configured for a dump setup that uses shift
 * registers for address lines and Rapsberry Pi GPIO pins for input data.
 * See ParallelFLASHDumper_rpi.jpg
 */

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

/* define according to the size of your target chip */
#define MAX_ADDR        0xFFFFFF    /* 16 MB */

/*
 * I/O DATA pins
 *
 * On Pi 3 and 4 UART pins (15, 16) are used for bluetooth by default.
 */
#define DQ0         8
#define DQ1         9
#define DQ2         7
#define DQ3         0
#define DQ4         1
#define DQ5         2
#define DQ6         3
#define DQ7         4

#define OE          5
#define WE          12

#define ADDR_DATA   13
#define ADDR_CLOCK  6


/* LSB is shifted out first */
static void shiftAddrByte(unsigned char byte)
{
    int i;

    for (i = 0; i < 8; i++) {
        digitalWrite(ADDR_CLOCK, 0);
        delayMicroseconds(2);

        digitalWrite(ADDR_DATA, byte & 1);

        digitalWrite(ADDR_CLOCK, 1);
        delayMicroseconds(2);
        byte >>= 1;
    }
}

static void sendAddr(unsigned addr)
{
    int i;

    for (i = 0; i < 3; i++) {
        shiftAddrByte(addr & 0xFF);
        addr >>= 8;
    }

    /* LATCH/STROBE */
    delayMicroseconds(2);
}

static void setup(void)
{
    /* Set pin modes */
    pinMode(ADDR_DATA, OUTPUT);
    pinMode(ADDR_CLOCK, OUTPUT);

    pinMode(DQ0, INPUT);
    pinMode(DQ1, INPUT);
    pinMode(DQ2, INPUT);
    pinMode(DQ3, INPUT);
    pinMode(DQ4, INPUT);
    pinMode(DQ5, INPUT);
    pinMode(DQ6, INPUT);
    pinMode(DQ7, INPUT);

    pinMode(OE, OUTPUT);
}

static void output_test(void)
{
    int v;

    printf("ouput_test:\n");

    pinMode(DQ0, OUTPUT);
    pinMode(DQ1, OUTPUT);
    pinMode(DQ2, OUTPUT);
    pinMode(DQ3, OUTPUT);
    pinMode(DQ4, OUTPUT);
    pinMode(DQ5, OUTPUT);
    pinMode(DQ6, OUTPUT);
    pinMode(DQ7, OUTPUT);

    pinMode(OE, OUTPUT);
    pinMode(WE, OUTPUT);

    pinMode(ADDR_DATA, OUTPUT);
    pinMode(ADDR_CLOCK, OUTPUT);

    /* repeat simple patterns */
    for (v = 0; v < 16; v++) {
        int a;
        int b;

        if (v < 8) {    /* all on/off */
            a = b = v & 1;
        } else {        /* bit on/bit off */
            a = v & 1;
            b = !a;
        }

        digitalWrite(DQ0, a);
        digitalWrite(DQ1, b);
        digitalWrite(DQ2, a);
        digitalWrite(DQ3, b);

        digitalWrite(DQ4, a);
        digitalWrite(DQ5, b);
        digitalWrite(DQ6, a);
        digitalWrite(DQ7, b);

        digitalWrite(OE, a);
        digitalWrite(WE, b);
        digitalWrite(ADDR_DATA, a);
        digitalWrite(ADDR_CLOCK,b);
        delay(500);
    }

    /* test all values for each 4 outputs group */
    for (v = 0; v < 16; v++) {
        digitalWrite(DQ0, v & 1);
        digitalWrite(DQ1, v & 2);
        digitalWrite(DQ2, v & 4);
        digitalWrite(DQ3, v & 8);

        digitalWrite(DQ4, v & 1);
        digitalWrite(DQ5, v & 2);
        digitalWrite(DQ6, v & 4);
        digitalWrite(DQ7, v & 8);

        digitalWrite(OE, v & 1);
        digitalWrite(WE, v & 2);
        digitalWrite(ADDR_DATA, v & 4);
        digitalWrite(ADDR_CLOCK, v & 8);
        delay(500);
    }

    digitalWrite(DQ0, 0);
    digitalWrite(DQ1, 0);
    digitalWrite(DQ2, 0);
    digitalWrite(DQ3, 0);
    digitalWrite(DQ4, 0);
    digitalWrite(DQ5, 0);
    digitalWrite(DQ6, 0);
    digitalWrite(DQ7, 0);

    digitalWrite(OE, 0);
    digitalWrite(WE, 0);

    digitalWrite(ADDR_DATA, 0);
    digitalWrite(ADDR_CLOCK, 0);
}

static void usage(void)
{
    printf(
        "nordump [flags] <dump_file>\n"
        "flags:\n"
        "\t-o\toutput test\n"
    );
    exit(1);
}

int main(int argc, char const *argv[]) {
    const char *arg, *dump_file;
    FILE *f;
    unsigned char input;
    unsigned addr;

    /* Init wiringPi */
    if (wiringPiSetup() == -1) {
        printf("failed to initialize wiringPi\n");
        return 1;
    }

    /* Parse args */
    if (argc != 2)
        usage();

    arg = argv[1];
    if (arg[0] == '-') {
        if (arg[1] == 0 || arg[2] != 0)
            usage();

        switch (arg[1]) {
        case 'o':
            output_test();
            break;

        default:
            usage();
        }
        return 0;
    }

    dump_file = arg;

    /* TODO implement and test flash dump */
    printf("TODO\n");
    return 1;

    /* Open dump file */
    f = fopen(dump_file, "wb");
    if (f == NULL) {
        printf("failed to open %s\n", dump_file);
        return 1;
    }

    /* Setup pins */
    setup();

    for (addr = 0; addr <= MAX_ADDR; addr++) {
        sendAddr(addr);

        /* Read data */
        input  = digitalRead(DQ0);
        input |= digitalRead(DQ1) << 1;
        input |= digitalRead(DQ2) << 2;
        input |= digitalRead(DQ3) << 3;
        input |= digitalRead(DQ4) << 4;
        input |= digitalRead(DQ5) << 5;
        input |= digitalRead(DQ6) << 6;
        input |= digitalRead(DQ7) << 7;

        /* Save data */
        if (fputc(input, f) == EOF) {
            printf("error at addr 0x%x\n", addr);
            return 1;
        }
  }

  fclose(f);
  return 0;
}
