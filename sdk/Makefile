# sdk/Makefile - Makefile compartido para agonv-utils en ESP32-MOS
#
# NO invocar directamente. Cada comando lo incluye:
#   include ../sdk/Makefile
#
# Variables que el Makefile del comando debe definir ANTES del include:
#   NAME    - nombre del binario (sin extensión)
#   SRCS    - archivos .c fuente (con ruta completa o relativa al CMD_DIR)
#   CMD_DIR - directorio del comando (normalmente $(CURDIR))

# ── Target selection ─────────────────────────────────────────────────────
TARGET ?= esp32p4

# ── Directorios ──────────────────────────────────────────────────────────
SDK_DIR  := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
INC_DIR  := $(SDK_DIR)
MOS_INC  := $(SDK_DIR)../../agon-lite-v/components/mos_api/include

# ── Toolchain & arch flags per target ────────────────────────────────────
ifeq ($(TARGET),esp32p4)
  # ESP32-P4: RISC-V RV32IMAFC
  CC      = riscv32-esp-elf-gcc
  AS      = riscv32-esp-elf-gcc
  OBJCOPY = riscv32-esp-elf-objcopy
  OBJDUMP = riscv32-esp-elf-objdump
  ARCH_FLAGS = -march=rv32imafc -mabi=ilp32f -DCONFIG_IDF_TARGET_ESP32P4=1
  # ESP32-P4: unified PSRAM address space (DBUS == IBUS).
  # s_exec_arena is at 0x48000000 (from build/esp32-mos.map .ext_ram.bss)
  MOS_EXEC_BASE ?= 0x48000000
  MOS_DBUS_BASE ?= 0x48000000
  LIBGCC_DIR := $(dir $(shell $(CC) $(ARCH_FLAGS) -print-libgcc-file-name))
  LIBC_DIR   := $(dir $(shell $(CC) $(ARCH_FLAGS) -print-file-name=libc.a))
else
  # ESP32-S3: Xtensa LX7 (default)
  CC      = xtensa-esp-elf-gcc
  AS      = xtensa-esp-elf-gcc
  OBJCOPY = xtensa-esp-elf-objcopy
  OBJDUMP = xtensa-esp-elf-objdump
  XTENSA_DYNCONFIG_SO := xtensa_esp32s3.so
  XTENSA_LIB_DIR      := $(dir $(shell which xtensa-esp-elf-gcc))../lib
  ARCH_FLAGS = -mdynconfig=$(XTENSA_DYNCONFIG_SO) -mlongcalls -mtext-section-literals
  # ESP32-S3: separate DBUS/IBUS windows
  # s_exec_arena DBUS (data window):        0x3C0E0000
  # s_exec_arena IBUS (instruction window): 0x420E0000  (= DBUS + 0x6000000)
  MOS_EXEC_BASE ?= 0x420E0000
  MOS_DBUS_BASE ?= 0x3C0E0000
  LIBGCC_DIR := $(dir $(shell $(CC) -mdynconfig=$(XTENSA_DYNCONFIG_SO) -print-libgcc-file-name))
  # Export DYLD_LIBRARY_PATH so the compiler can dlopen xtensa_esp32s3.so
  export DYLD_LIBRARY_PATH := $(XTENSA_LIB_DIR)$(if $(DYLD_LIBRARY_PATH),:$(DYLD_LIBRARY_PATH))
endif

# ── Flags de compilación ─────────────────────────────────────────────────
CFLAGS = \
    $(ARCH_FLAGS) \
    -Os \
    -std=c11 \
    -nostdlib \
    -ffreestanding \
    -fno-builtin \
    -Wall \
    -Wextra \
    -I$(INC_DIR) \
    -I$(MOS_INC)

ASFLAGS = \
    $(ARCH_FLAGS) \
    -nostdlib

LDFLAGS = \
    -T$(SDK_DIR)mos_user.ld \
    -Wl,--defsym,MOS_EXEC_BASE=$(MOS_EXEC_BASE) \
    -Wl,--defsym,MOS_DBUS_BASE=$(MOS_DBUS_BASE) \
    -nostdlib \
    -Wl,--gc-sections

# libgcc provides software float routines (__divsf3, __mulsf3, etc.)
# libc provides memcpy/memset that RISC-V GCC emits implicitly for struct copies.
# Both have no OS dependencies so safe to link into freestanding programs.
ifeq ($(TARGET),esp32p4)
LIBGCC_FLAGS = -L$(LIBGCC_DIR) -L$(LIBC_DIR) -lc -lgcc
else
LIBGCC_FLAGS = -L$(LIBGCC_DIR) -lgcc
endif

# ── Shared objects (linked into every program) ────────────────────────────
COMMON_OBJ := $(SDK_DIR)mos_vector.$(TARGET).o

# ── Rutas de salida por comando ───────────────────────────────────────────
CMD_DIR  ?= $(CURDIR)
OUT_ELF  := $(CMD_DIR)/bin/$(NAME).elf
OUT_BIN  := $(CMD_DIR)/bin/$(NAME).bin
OUT_MAP  := $(CMD_DIR)/bin/$(NAME).map

.PHONY: all clean disasm

all: $(OUT_BIN)

# Compilar mos_vector.S (objeto compartido, sufijo por target)
$(SDK_DIR)mos_vector.$(TARGET).o: $(SDK_DIR)mos_vector.S
	$(AS) $(ASFLAGS) -c -o $@ $<

# Compilar .c → .elf  (mos_vector.o primero para que .text.vector quede en offset 0)
$(OUT_ELF): $(SRCS) $(SDK_DIR)mos_user.ld $(COMMON_OBJ)
	@mkdir -p $(CMD_DIR)/bin
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$(OUT_MAP) \
	    -o $@ $(COMMON_OBJ) $(SRCS) $(LIBGCC_FLAGS)

# ELF → binario plano
$(OUT_BIN): $(OUT_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "  → $@ ($$(wc -c < $@) bytes)"

# Desensamblar para depuración
disasm: $(OUT_ELF)
	$(OBJDUMP) -d $< > $(CMD_DIR)/bin/$(NAME).asm
	@echo "  → $(CMD_DIR)/bin/$(NAME).asm"

clean:
	rm -f $(OUT_ELF) $(OUT_BIN) $(OUT_MAP) $(CMD_DIR)/bin/$(NAME).asm
