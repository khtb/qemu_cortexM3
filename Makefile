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
SRCDIR = src freertos freertos/portable/ARM_CM3

OUTDIR = out
INCDIR = freertos/portable/ARM_CM3 \
		 freertos/include

CC = arm-none-eabi-gcc
LD      = $(CC)
OBJCOPY = arm-none-eabi-objcopy
# CFLAGS  = -mcpu=cortex-m4 -mthumb -O0 -g -ffreestanding -fno-builtin -nostdlib -mfpu=fpv4-sp-d16 -mfloat-abi=hard
# LDFLAGS = -T linker.ld -nostartfiles -Wl,--gc-sections,-Map=$(OUTDIR)/$(PROJECT).map
LDSCRIPT = linker.ld
MCU = -mcpu=cortex-m3
CFLAGS = $(MCU) -mthumb $(C_INCS) -O0 -Wall -g
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -lc -lm -lnosys -Wl,-Map=$(OUTDIR)/$(PROJECT).map


SRC     := $(foreach d, $(SRCDIR),$(wildcard $(d)/*.c))
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
	qemu-system-arm -M lm3s6965evb -m 16 -cpu cortex-m3 -nographic -kernel $(ELF)

debug: $(ELF)
	qemu-system-arm -M lm3s6965evb -m 16 -cpu cortex-m3 -nographic -kernel $(ELF) -S -gdb tcp::1234

diss: 
	arm-none-eabi-objdump -d -C $(ELF) > $(OUTDIR)/$(PROJECT).diss


test:
	echo $(OBJ)
	echo $(SEP)
	echo $(SRC)