nordump
-------

This program is based on cyphunk's ParallelFLASHDumper.
It has been modified/stripped to fit my needs.
It has been tested with MX29GL128FHT2I-90G NOR FLASH only.

setup
-----

1. Connect "parallel out" shift registers to address pins of target
2. Connect "parallel out" pins to Raspberry Pi (see nordump.c for the mapping)
3. Connect RPi GPIO's to data out pins of target (see nordump.c for the mapping)
4. Connect RPi GPIO's to OE#/WE# pins of target (see nordump.c for the mapping)
5. Define MAX_ADDR. Code will dump address 0 to MAX_ADDR

usage
-----

Run "./nordump <dumpfile>" to dump the contents of the FLASH chip to
<dumpfile>.

The other options are mostly for test, depend on external circuitry and may
damage your RPi or FLASH chip. That's why they are disabled in code.
