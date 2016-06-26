# An ARM-based LZ4 compression demonstration

## Description

This demo shows how the LZ4 compression library can be used on embedded
microcontrollers.  Using a preprocessor switch, the program will either perform
a simple compression similar to the example included with the LZ4 library, or it
will launch a single-producer, single-consumer [FreeRTOS](http://www.freertos.org)-based program
where compressed data is transferred between two threads via a shared buffer
and then decompressed.

## Notes

Currently the target architecture is ARM Cortex-M4, and the included demo is
configured for TI's [TM4C1294 Launchpad](http://www.ti.com/tool/ek-tm4c1294xl).

The LZ4 library currently used is a [fork](https://github.com/Cyan4973/lz4/pull/211) by @vvromanov which hopefully
will be mainlined in the near future.  Previously, a mostly-stock version of
[r123](https://github.com/Cyan4973/lz4/tree/r123) had been used.
