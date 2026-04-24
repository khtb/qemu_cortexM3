#######################################                                                      
#        _   __  _   _   _____   ____  
#       | | / / | | | | (_   _) |  _ \ 
#       | |/ /  | |_| |   | |   | |_) )
#       |   <   |  _  |   | |   |  _ ( 
#       | |\ \  | | | |   | |   | |_) )
#       |_| \_\ |_| |_|   |_|   |____/ 
#                                      
#                                      
########################################
# Makefile
# Author: khattab


SEP="============================================="
PROJECT = app
SRCDIR = src freertos freertos/portable/ARM_CM3 \
		 src/ethernet \
		 src/ethernet/lwip_port \
		 src/uart    \
		 src/shell \
		 lib/FreeRTOS-Plus-CLI \
		 lib/lwip/src/core \
		 lib/lwip/src/core/ipv4 \
		 lib/lwip/src/api

# Specific files instead of whole directories if needed
LWIP_NETIF_FILES = lib/lwip/src/netif/ethernet.c

OUTDIR = out
INCDIR = freertos/portable/ARM_CM3 \
		 freertos/include \
		 lib/FreeRTOS-Plus-CLI \
		 lib/lwip/src/include \
		 src/ethernet/lwip_port \
		 $(SRCDIR)

CC = arm-none-eabi-gcc
LD      = $(CC)
OBJCOPY = arm-none-eabi-objcopy
# CFLAGS  = -mcpu=cortex-m4 -mthumb -O0 -g -ffreestanding -fno-builtin -nostdlib -mfpu=fpv4-sp-d16 -mfloat-abi=hard
# LDFLAGS = -T linker.ld -nostartfiles -Wl,--gc-sections,-Map=$(OUTDIR)/$(PROJECT).map
LDSCRIPT = linker.ld
MCU = -mcpu=cortex-m3
CFLAGS = $(MCU) -mthumb $(C_INCS) -O0 -Wall -g
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -lc -lm -lnosys -Wl,-Map=$(OUTDIR)/$(PROJECT).map


SRC     := $(foreach d, $(SRCDIR),$(wildcard $(d)/*.c)) $(LWIP_NETIF_FILES)
OBJ     := $(patsubst %.c,$(OUTDIR)/%.o,$(SRC))
# OBJ     := $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%.o,$(SRC))
# OBJ       :=$(foreach d,$(SRCDIR),$(wildcard $(d)/*.c))
INC_FILES     := $(wildcard $(INCDIR)/*.h)
INC           := $(addprefix -I, $(INCDIR))
ELF     := $(OUTDIR)/$(PROJECT).elf

all: $(ELF)

$(OBJ): $(OUTDIR)/%.o: %.c | $(OUTDIR)
	echo "Compiline" $< 
	mkdir -p  $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@	

$(ELF): $(OBJ) linker.ld
	$(LD) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@ 

$(OUTDIR):
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)

run: $(ELF)
# 	qemu-system-arm -M mps2-an386 -cpu cortex-m4 -nographic -kernel $(ELF) -d int,cpu_reset
	qemu-system-arm -M lm3s6965evb -m 16 -cpu cortex-m3 -nographic -kernel $(ELF) -net nic,macaddr=00:11:22:33:44:55 -net user,hostfwd=tcp::12345-:7,hostfwd=tcp::2323-:23,hostfwd=udp::5000-:5000

# 	qemu-system-arm -M lm3s6965evb -m 16 -cpu cortex-m3 -nographic -kernel $(ELF) -net nic -net user,hostfwd=tcp::55007-:7 \

debug: $(ELF)
	qemu-system-arm -M lm3s6965evb -m 16 -cpu cortex-m3 -nographic -kernel $(ELF) -net nic,macaddr=00:11:22:33:44:55 -net user,hostfwd=tcp::12345-:7,hostfwd=tcp::2323-:23,hostfwd=udp::5000-:5000 \
	-S -gdb tcp::1234 

# Capture traffic to qemu_net.pcap (open in Wireshark)
# Capture traffic to qemu_net.pcap (open in Wireshark)
debug_pcap: $(ELF)
	qemu-system-arm -M lm3s6965evb -m 16 -cpu cortex-m3 -nographic -kernel $(ELF) \
	-net nic,macaddr=00:11:22:33:44:55,netdev=n1 \
	-netdev user,id=n1,hostfwd=tcp::12345-:7,hostfwd=tcp::2323-:23 \
	-object filter-dump,id=f1,netdev=n1,file=qemu_net.pcap

# Run with logger support (sends L2 frames to UDP 12345)
# Note: Standard networking (ping google) might not work in this mode as it replaces 'user' backend
run_logger: $(ELF)
	qemu-system-arm -M lm3s6965evb -m 16 -cpu cortex-m3 -nographic -kernel $(ELF) \
	-net nic,macaddr=00:11:22:33:44:55,netdev=n1 \
	-netdev socket,id=n1,udp=127.0.0.1:12345,localaddr=127.0.0.1:12348

diss: 
	arm-none-eabi-objdump -d -C $(ELF) > $(OUTDIR)/$(PROJECT).diss


test:
	echo $(OBJ)
	echo $(SEP)
	echo $(SRC)