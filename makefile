### USAGE ###

# see programs for PROGRAM options
# to compile: make PROGRAM 
# to flash latest compile via USB: make flash
# to flash latest compile via cloud: make flash device=DEVICE
# to start serial monitor: make monitor
# to compile & flash: make PROGRAM flash
# to compile, flash & monitor: make PROGRAM flash monitor

### PARAMS ###

# platform and version
PLATFORM?=photon
VERSION?=2.0.0-rc.4
device?=

# default bin is the latest compiled
BIN:=$(shell ls -Art *.bin | tail -n 1)

### PROGRAMS ###

MODULES:=
debug/blink: MODULES=
debug/i2c_scanner: MODULES=
debug/lcd: MODULES=
debug/logger: MODULES=modules/logger
ministat: MODULES=device_module stepper_module

### HELPERS ###

# list available devices
list:
	@echo "\nINFO: querying list of available devices..."
	@particle list

# start serial monitor
monitor:
	@echo "\nINFO: connecting to serial monitor..."
	@trap "exit" INT; while :; do particle serial monitor; done

### COMPILE & FLASH ###

# compile binary
%: 
	@echo "\nINFO: compiling $@ in the cloud for $(PLATFORM) $(VERSION)...."
	@cd src && particle compile $(PLATFORM) $(MODULES) $@ $@/project.properties --target $(VERSION) --saveTo ../$(subst /,_,$@)-$(VERSION).bin

# flash (via cloud if device is set, via usb if none provided)
# by the default the latest bin, unless BIN otherwise specified
flash:
ifeq ($(device),)
	@$(MAKE) usb_flash BIN=$(shell ls -Art *.bin | tail -n 1)
else
	@$(MAKE) cloud_flash BIN=$(shell ls -Art *.bin | tail -n 1)
endif

# flash via the cloud
cloud_flash: 
ifeq ($(device),)
	@echo "ERROR: no device provided, specify the name to flash via make ... device=???."
else
	@echo "\nINFO: flashing $(BIN) to $(device) via the cloud..."
	@particle flash $(device) $(BIN)
endif

# usb flash
usb_flash: 
	@echo "INFO: putting device into DFU mode"
	@particle usb dfu
	@echo "INFO: flashing $(BIN) over USB (requires device in DFU mode = yellow blinking)..."
	@particle flash --usb  $(BIN)

# cleaning
clean:
	@echo "INFO: removing all .bin files..."
	@rm -f ./*.bin
