cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(my_project)

pico_sdk_init()

add_compile_options(
	-Wall
	-Wextra
	-std=c++20
)

add_executable(pibridge 
	src/main.cpp
	src/stream.cpp
	src/iec.cpp
	)

target_link_libraries(pibridge
	pico_stdlib
	pico_multicore
	)

# USB on, UART off
pico_enable_stdio_usb(pibridge 1)
pico_enable_stdio_uart(pibridge 0)

pico_add_extra_outputs(pibridge)
