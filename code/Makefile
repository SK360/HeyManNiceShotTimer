sketch      :=code.ino
CORE        :=m5stack:esp32
# Flashing on  an "ESP32 Dev Module" board
boardconfig :="${CORE}:m5stack_stickc_plus2"

ARDUINO_CLI ?= arduino-cli
MKSPIFFS=~/.arduino15/packages/m5stack/tools/mklittlefs/3.0.0-gnu12-dc7f933/mklittlefs
BC          ?= bc

PARTITION_TABLE=~/.arduino15/packages/m5stack/hardware/esp32/2.1.4/tools/partitions/default_8MB.csv

DEVICE :=/dev/cu.usbserial-5B212384801

.PHONY: build
build:
	$(ARDUINO_CLI) compile --fqbn $(boardconfig) $(sketch)

.PHONY: flash
flash:
	$(ARDUINO_CLI) upload -p ${DEVICE} --fqbn ${boardconfig} ${sketch} 

.PHONY: filesystem.bin
.ONESHELL:
filesystem.bin:
	PROPS=$$($(ARDUINO_CLI) compile --fqbn $(boardconfig) --show-properties)
	BUILD_SPIFFS_BLOCKSIZE=4096
	BUILD_SPIFFS_PAGESIZE=256
	BUILD_SPIFFS_START_HEX=$$(cat ${PARTITION_TABLE} | grep "^spiffs"|cut -d, -f4 | xargs)
	BUILD_SPIFFS_START=$$(echo "ibase=16;$${BUILD_SPIFFS_START_HEX:2}"|bc -q)
	echo "BUILD_SPIFFS_START $$BUILD_SPIFFS_START_HEX ($$BUILD_SPIFFS_START)"
	BUILD_SPIFFS_SIZE_HEX=$$(cat ${PARTITION_TABLE} | grep "^spiffs"|cut -d, -f5 | xargs)
	BUILD_SPIFFS_SIZE=$$(echo "ibase=16;$${BUILD_SPIFFS_SIZE_HEX:2}"|bc -q)
	echo "BUILD_SPIFFS_SIZE  $$BUILD_SPIFFS_SIZE_HEX ($$BUILD_SPIFFS_SIZE)"
	$(MKSPIFFS) -c data -p $$BUILD_SPIFFS_PAGESIZE -b $$BUILD_SPIFFS_BLOCKSIZE -s $$BUILD_SPIFFS_SIZE $@

.PHONY: flash-fs
.ONESHELL:
flash-fs: filesystem.bin
	BUILD_SPIFFS_START_HEX=$$(cat ${PARTITION_TABLE} | grep "^spiffs"|cut -d, -f4 | xargs)
	python ~/.arduino15/packages/m5stack/tools/esptool_py/4.5.1/esptool.py --chip esp32 \
	  --port ${DEVICE} \
	  --baud 1500000 \
	  --before default_reset \
	  --after hard_reset \
	  write_flash -z --flash_mode dio \
	  --flash_freq 80m --flash_size 8MB \
	  $${BUILD_SPIFFS_START_HEX} filesystem.bin


.PHONY: clean
clean:
	rm -rf build
	rm -f filesystem.bin

.PHONY: monitor
monitor:
	 arduino-cli monitor --fqbn ${boardconfig} -p ${DEVICE} --config 115200

.PHONY: all-flash
all-flash: build flash filesystem.bin flash-fs clean monitor

.PHONY: all
all: build flash clean monitor


