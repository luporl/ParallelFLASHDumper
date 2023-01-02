#ifndef CFG_H
#define CFG_H

/* Enable debug */
#define DEBUG               1
/* Enable test commands */
#ifndef TEST
#define TEST                1
#endif
/*
 * Drive address pins (up to A12) directly through RPi's GPIOs, instead of
 * using shift registers.
 * The other address pins must be tied to ground to get meaningful reads.
 * (only tested with flash dump command)
 */
#define TEST_ADDR_PINS      0


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

#endif
