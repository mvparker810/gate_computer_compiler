# Detect platform
ifeq ($(OS),Windows_NT)
    RM = del /Q
    MKDIR = mkdir
    EXE = .exe
    SEP = \\
else
    RM = rm -f
    MKDIR = mkdir -p
    EXE =
    SEP = /
endif

CC = gcc
CFLAGS = -Wall -std=c99
OUT_DIR = .$(SEP)out

# ROM writer sources
ROM_WRITERS = ROM_WRITERS/opcode_flags_ROM.c \
              ROM_WRITERS/branchcond_ROM.c \
			  ROM_WRITERS/fp16_digit_masks.c\
              ROM_WRITERS/hex_display_ROM.c \
              ROM_WRITERS/ascii_fontdata_ROM.c \
			  

# Generate executable names
ROM_EXES = $(addprefix $(OUT_DIR)/,opcode_flags branchcond_rom fp16_digit_masks hex_display_rom ascii_fontdata_rom)
ROM_EXES_WITH_EXT = $(addsuffix $(EXE),$(ROM_EXES))

.PHONY: all assembler roms opcode_flags branchcond_rom fp16_digit_masks hex_display_rom ascii_fontdata_rom test test_fib clean help

$(OUT_DIR):
	$(MKDIR) $@

all: assembler roms

assembler: $(OUT_DIR)/lsasm_v2$(EXE)

roms: $(ROM_EXES_WITH_EXT)

$(OUT_DIR)/lsasm_v2$(EXE): $(OUT_DIR) machinecode/lsasm_v2.c utils/rom_writer.c utils/rom_writer.h
	$(CC) $(CFLAGS) -o $@ machinecode/lsasm_v2.c utils/rom_writer.c

$(OUT_DIR)/opcode_flags$(EXE): $(OUT_DIR) ROM_WRITERS/opcode_flags_ROM.c utils/rom_writer.c utils/rom_writer.h
	$(CC) $(CFLAGS) -o $@ ROM_WRITERS/opcode_flags_ROM.c utils/rom_writer.c

$(OUT_DIR)/branchcond_rom$(EXE): $(OUT_DIR) ROM_WRITERS/branchcond_ROM.c utils/rom_writer.c utils/rom_writer.h
	$(CC) $(CFLAGS) -o $@ ROM_WRITERS/branchcond_ROM.c utils/rom_writer.c

$(OUT_DIR)/hex_display_rom$(EXE): $(OUT_DIR) ROM_WRITERS/hex_display_ROM.c utils/rom_writer.c utils/rom_writer.h
	$(CC) $(CFLAGS) -o $@ ROM_WRITERS/hex_display_ROM.c utils/rom_writer.c

$(OUT_DIR)/ascii_fontdata_rom$(EXE): $(OUT_DIR) ROM_WRITERS/ascii_fontdata_ROM.c utils/rom_writer.c utils/rom_writer.h
	$(CC) $(CFLAGS) -o $@ ROM_WRITERS/ascii_fontdata_ROM.c utils/rom_writer.c

$(OUT_DIR)/fp16_digit_masks$(EXE): $(OUT_DIR) ROM_WRITERS/fp16_digit_masks.c utils/rom_writer.c utils/rom_writer.h
	$(CC) $(CFLAGS) -o $@ ROM_WRITERS/fp16_digit_masks.c utils/rom_writer.c


# Alias targets for convenience
opcode_flags: $(OUT_DIR)/opcode_flags$(EXE)
branchcond_rom: $(OUT_DIR)/branchcond_rom$(EXE)
hex_display_rom: $(OUT_DIR)/hex_display_rom$(EXE)
ascii_fontdata_rom: $(OUT_DIR)/ascii_fontdata_rom$(EXE)

test: assembler
	$(OUT_DIR)/lsasm_v2$(EXE) -f hex scripts/sample.txt sample_test
	@echo Test complete. Check sample_test_ALPHA.hex and sample_test_BETA.hex

test_fib: assembler
	$(OUT_DIR)/lsasm_v2$(EXE) -f hex scripts/FIBONACCI.txt fib_test
	@echo Fibonacci test complete.

clean:
	-$(RM) $(OUT_DIR)$(SEP)*$(EXE) $(OUT_DIR)$(SEP)*.hex $(OUT_DIR)$(SEP)*.out 2>nul || echo Cleaned

help:
	@echo Available targets:
	@echo   make all              - Build assembler and all ROM generators
	@echo   make assembler        - Build assembler only
	@echo   make roms             - Build all ROM generators
	@echo   make opcode_flags     - Build opcode flags ROM generator
	@echo   make branchcond_rom   - Build branch condition ROM generator
	@echo   make hex_display_rom  - Build hex display ROM generator
	@echo   make ascii_fontdata_rom - Build ASCII font data ROM generator
	@echo   make test             - Assemble sample.txt
	@echo   make test_fib         - Assemble FIBONACCI.txt
	@echo   make clean            - Remove built files
	@echo   make help             - Show this help message
