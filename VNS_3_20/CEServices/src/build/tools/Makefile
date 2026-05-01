BINMERGE := ../../eCos/host/webstax_binmerge/webstax_binmerge.c


compile:
	gcc -g -m32 -Wall $(BINMERGE) -o webstax_binmerge

compile_256:
	gcc -g -m32 -Wall $(BINMERGE) -DALLOWED_MAXIMUM_SECTOR_SIZE=256 -o webstax_binmerge_256

dbg:
	ddd webstax_binmerge

# If not given on the command line, use this one:
REDBOOT_PATH ?= ../obj/redboot.bin
FIRMWARE_PATH ?= ../obj/*.gz

FIRMWARE_PATH_VNS6    ?= ../obj/VNS-6.gz
FIRMWARE_PATH_IES6    ?= ../obj/iES-6.gz
FIRMWARE_PATH_IES8    ?= ../obj/iES-8.gz
FIRMWARE_PATH_VNS12   ?= ../obj/VNS-12.gz
FIRMWARE_PATH_IES12   ?= ../obj/iES-12.gz
FIRMWARE_PATH_IES_2_0 ?= ../obj/iES-12_2_0.gz
FIRMWARE_PATH_IES_16_2_0 ?= ../obj/iES-16_2_0.gz
FIRMWARE_PATH_NCGU ?= ../obj/NCGU_SWITCH.gz
FIRMWARE_PATH_IGU ?= ../obj/IGU_SWITCH.gz
FIRMWARE_PATH_MITR ?= ../obj/MITR_SWITCH.gz
FIRMWARE_PATH_VESM ?= ../obj/VESM_SWITCH.gz
FIRMWARE_PATH_ESIR ?= ../obj/eSIR.gz
# Example #1:
# Create master file from redboot and runtime code
# with the following features:
#  - Dual Image Support
#  - Flash sector is 256 KBytes, size is 16MB, for example: M26P128 (the one on Vitesse EVB)
#  - Verbose output
#  - MAC Address = 00:01:c1:01:02:03
#  - Board ID = 1234
#  - Default bootscript
flash16_256: compile_256
	./webstax_binmerge_256 -v -sb 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o master_flash16_256.bin -a "MAC=00:01:c1:01:02:03" -a "BOARDID=1234"

# Example #2:
# Create master file from redboot and runtime code
# with the following features:
#  - Dual Image Support
#  - Flash sector is 64 KBytes, size is 16MB, for example: MX25L128
#  - Verbose output
#  - MAC Address = 00:01:c1:01:02:03
#  - Board ID = 1234
#  - Default bootscript
flash16_64: compile
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o master_flash16_64.bin -a "MAC=00:01:c1:01:02:03" -a "BOARDID=1234"

# Example #3:
# Create master file from redboot and runtime code 
# with the following features:
#  - Dual Image Support
#  - Flash sector is 128 KBytes, size is 16MB.
#  - Verbose output
#  - MAC Address = 00:01:c1:01:02:03
#  - Board ID = 1234
#  - Default bootscript
flash16_128: compile
	./webstax_binmerge -v -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o master_flash16_128.bin -a "MAC=00:01:c1:01:02:03" -a "BOARDID=1234"

flash_vns: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH) -o vns_6_flash_01.bin -a "MAC=30:2d:e8:00:00:01" -a "BOARDID=1234" 

flash_vns6: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_VNS6) -o vns_6_flash_01.bin -a "MAC=30:2d:e8:12:00:00" -a "BOARDID=1" 

flash_ies6: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_IES6) -o ies_6_flash_r1.bin -a "MAC=30:2d:e8:12:00:00" -a "BOARDID=1" 

flash_ies8: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_IES8) -o ies_8_flash_r1.bin -a "MAC=30:2d:e8:1d:00:00" -a "BOARDID=1" 

flash_vns12: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_VNS12) -o vns_12_flash_r1.bin -a "MAC=30:2d:e8:11:00:00" -a "BOARDID=1"

flash_ies12: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_IES12) -o ies_12_flash_r1.bin -a "MAC=30:2d:e8:11:00:00" -a "BOARDID=1"

flash_ies_2_0: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_IES_2_0) -o ies_2_0_flash_.bin -a "MAC=30:2d:e8:11:00:00" -a "BOARDID=1"

flash_ies_16_2_0: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_IES_16_2_0) -o ies_16_2_0_flash_.bin -a "MAC=30:2d:e8:1E:00:00" -a "BOARDID=1"

flash_epe: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_IES12) -o epe_flash_r1.bin -a "MAC=30:2d:e8:11:00:00" -a "BOARDID=00010001" -a "BOARDTYPE=0"

flash_ncgu: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_NCGU) -o ncgu_flash.bin -a "MAC=30:2d:e8:17:00:00" -a "BOARDID=1"  
	
flash_igu: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_IGU) -o igu_flash.bin -a "MAC=30:2d:e8:17:00:00" -a "BOARDID=1" 
	
flash_mitr: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_MITR) -o mitr_flash.bin -a "MAC=30:2d:e8:19:00:00" -a "BOARDID=1"
	
flash_vesm: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_VESM) -o vesm_flash.bin -a "MAC=30:2d:e8:24:00:00" -a "BOARDID=1"
	
flash_esir: compile 
	./webstax_binmerge -v -sb 64 -sc 256 -r $(REDBOOT_PATH) -w $(FIRMWARE_PATH_ESIR) -o esir_flash.bin -a "MAC=30:2d:e8:1A:00:00" -a "BOARDID=1"
