sketch      :=code.ino
CORE        :=m5stack:esp32
boardconfig :="${CORE}:m5stack_stickc_plus2"

ARDUINO_CLI ?= arduino-cli

DEVICE :=/dev/cu.usbserial-5B212384801

.PHONY: build
build:
	$(ARDUINO_CLI) compile -e --fqbn $(boardconfig) $(sketch)

.PHONY: flash
flash:
	$(ARDUINO_CLI) upload -p ${DEVICE} --fqbn ${boardconfig} ${sketch}

.PHONY: merge
merge:
	~/Library/Arduino15/packages/m5stack/tools/esptool_py/4.5.1/esptool --chip esp32 merge_bin --target-offset 0 -o merged.bin \
	  0x1000 build/m5stack.esp32.m5stack_stickc_plus2/code.ino.bootloader.bin \
	  0x8000 build/m5stack.esp32.m5stack_stickc_plus2/code.ino.partitions.bin \
	  0x10000 build/m5stack.esp32.m5stack_stickc_plus2/code.ino.bin

.PHONY: clean
clean:
	rm -rf build
	rm -f merged.bin

.PHONY: monitor
monitor:
	arduino-cli monitor --fqbn ${boardconfig} -p ${DEVICE} --config 115200

.PHONY: all
all: build flash clean monitor

.PHONY: release
release: clean build merge
