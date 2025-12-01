#ifndef ROM_WRITER_H
#define ROM_WRITER_H

#include <stdint.h>

typedef enum {
    ROM_HEX,
    ROM_UINT,
    ROM_INT,
    ROM_BINARY
} rom_format;

/*
 * Write ROM data to file
 *
 * filename: path to output file (directories created automatically)
 * data: array of 16-bit values
 * size: number of entries in data array
 * format: output format (ROM_HEX, ROM_UINT, ROM_INT, ROM_BINARY)
 *
 * Returns: 0 on success, -1 on failure
 */
int write_rom_file(const char* filename, uint16_t* data, int size, rom_format format);

#endif
