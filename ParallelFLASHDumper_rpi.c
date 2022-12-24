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
#include <wiringPi.h>

#define ADDR_STROBE     25  /* latch - orange */
#define ADDR_DATAOUT    29  /* green */
#define ADDR_CLOCK      28  /* yellow */

/* define according to the size of your target chip */
#define MAX_ADDR        0xFFFFFF    /* 16 MB */

/* INPUT DATA pins */
#define IN_DQ00         9
#define IN_DQ01         7
#define IN_DQ02         1
#define IN_DQ03         2
#define IN_DQ04         3
#define IN_DQ05         5
#define IN_DQ06         6
#define IN_DQ07         14

#define OE              8
#define RST             23


/* LSB is shifted out first */
static void shiftAddrByte(unsigned char byte)
{
    int i;

    for (i = 0; i < 8; i++) {
        digitalWrite(ADDR_CLOCK, 0);
        delayMicroseconds(2);

        digitalWrite(ADDR_DATAOUT, byte & 1);

        digitalWrite(ADDR_CLOCK, 1);
        delayMicroseconds(2);
        byte >>= 1;
    }
}

static void sendAddr(unsigned addr)
{
    int i;

    digitalWrite(ADDR_STROBE, LOW);

    for (i = 0; i < 3; i++) {
        shiftAddrByte(addr & 0xFF);
        addr >>= 8;
    }

    /* LATCH/STROBE */
    digitalWrite(ADDR_STROBE, HIGH);
    delayMicroseconds(2);
    digitalWrite(ADDR_STROBE, LOW);
    delayMicroseconds(2);
}

static void usage(void)
{
    printf("nordump <dump_file>\n");
}

static void setup(void)
{
    /* Set pin modes */
    pinMode(ADDR_STROBE, OUTPUT);
    pinMode(ADDR_DATAOUT, OUTPUT);
    pinMode(ADDR_CLOCK, OUTPUT);

    pinMode(IN_DQ00, INPUT);
    pinMode(IN_DQ01, INPUT);
    pinMode(IN_DQ02, INPUT);
    pinMode(IN_DQ03, INPUT);
    pinMode(IN_DQ04, INPUT);
    pinMode(IN_DQ05, INPUT);
    pinMode(IN_DQ06, INPUT);
    pinMode(IN_DQ07, INPUT);

    pinMode(OE, OUTPUT);
    pinMode(RST, OUTPUT);
}

static void reset(void)
{
    /* TODO reset shift registers */

    /* Reset flash */
    digitalWrite(RST, HIGH);
    digitalWrite(OE, HIGH);
    delay(1);
    digitalWrite(RST, LOW);
    delay(1);
    digitalWrite(RST, HIGH);
    digitalWrite(OE, LOW);
    delay(1);
}

int main(int argc, char const *argv[]) {
    const char *dump_file;
    FILE *f;
    unsigned char input;
    unsigned addr;

    if (argc != 2) {
        usage();
        return 1;
    }

    /* Open dump file */
    dump_file = argv[1];
    f = fopen(dump_file, "wb");
    if (f == NULL) {
        printf("failed to open %s\n", dump_file);
        return 1;
    }

    /* Init wiringPi */
    if (wiringPiSetup() == -1) {
        printf("failed to initialize wiringPi\n");
        return 1;
    }

    /* Setup pins, reset chips */
    setup();
    reset();

    for (addr = 0; addr <= MAX_ADDR; addr++) {
        sendAddr(addr);

        /* Read data */
        input  = digitalRead(IN_DQ00);
        input |= digitalRead(IN_DQ01) << 1;
        input |= digitalRead(IN_DQ02) << 2;
        input |= digitalRead(IN_DQ03) << 3;
        input |= digitalRead(IN_DQ04) << 4;
        input |= digitalRead(IN_DQ05) << 5;
        input |= digitalRead(IN_DQ06) << 6;
        input |= digitalRead(IN_DQ07) << 7;

        /* Save data */
        if (fputc(input, f) == EOF) {
            printf("error at addr 0x%x\n", addr);
            return 1;
        }
  }

  fclose(f);
  return 0;
}
