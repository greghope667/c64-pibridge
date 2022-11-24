default: build

# Change these as necessary
SRC_DIR=${CURDIR}
export PICO_SDK_PATH = ${CURDIR}/../../pico/pico-sdk

BUILD_DIR=${SRC_DIR}/build
TARGET=${BUILD_DIR}/pibridge

PICOTOOL=${HOME}/code/pico/picotool/build/picotool

clean:
	rm -r "${BUILD_DIR}"

configure:
	strat ubuntu cmake \
		-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
		-DCMAKE_BUILD_TYPE:STRING=Release \
		-S "${SRC_DIR}" \
		-B "${BUILD_DIR}" \
		-GNinja \

${TARGET}.elf:
	cd "${BUILD_DIR}" && ninja

${TARGET}.uf2: ${TARGET}.elf
	cd "${BUILD_DIR}" && ninja

build: ${TARGET}.elf ${TARGET}.uf2

flash: ${TARGET}.uf2
	sudo ${PICOTOOL} load "${TARGET}.uf2" -f

# Other utilities
utils/c64pb-read: utils/c64pb-read.c
	cc -O2 -Wall -Wextra $< -lserialport -o $@

utils/c64pb-split: utils/c64pb-split.c
	cc -O2 -Wall -Wextra $< -o $@

utils: utils/c64pb-read utils/c64pb-split