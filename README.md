# c64-pibridge

This is a simple program to connect my PC to a Commadore 64 using a pi pico as a bridge.
Uses the [pico sdk](https://github.com/raspberrypi/pico-sdk) , so grab a copy of that and set PICO_SDK_PATH to point to the correct path.

Very WIP, but does work for loading `.prg` files similar to a 1541 drive. Fastloaders won't work with this (yet).

## Build

```
git clone https://github.com/greghope667/c64-pibridge
cd c64-pibridge
mkdir build
cd build
export PICO_SDK_PATH=/path/to/sdk
cmake -DCMAKE_BUILD_TYPE:STRING=Release ..
make
```

## Communication

On the PC side, communication is handled as frames [frame.h](src/frame.h). In the [utils](utils) folder are some programs to perform translation.
