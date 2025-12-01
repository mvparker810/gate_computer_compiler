/*

ASCII Font Atlas to ROM Formatter

Parses an 8x8 font atlas BMP and converts each character to parallel ROM data.
- Characters: ASCII 32-127 (96 total)
- Each character: 8x8 pixels = 8 rows × 8 bits
- Output: 4 parallel ROM units (256 addresses × 16-bit each, direct ASCII mapping)
  ROM_ALPHA: Rows 0-1, ROM_BRAVO: Rows 2-3, ROM_CHARLIE: Rows 4-5, ROM_DELTA: Rows 6-7

Compilation (from gate_computer_compiler/ root):
$ gcc v2/text/ascii_formatter.c v2/rom_writer.c -o out/ascii_formatter

Usage:
$ ./ascii_formatter <input_bmp> [-f FORMAT]

Options:
- -f <FORMAT>: Output format: hex, uint, int, binary (default: hex)

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../utils/rom_writer.h"

#define CHAR_START 32
#define CHAR_END 127
#define CHAR_COUNT (CHAR_END - CHAR_START + 1)
#define CHAR_SIZE 8
#define CHAR_PIXELS (CHAR_SIZE * CHAR_SIZE)
#define ROM_SIZE 256
#define ROM_COUNT 4

// BMP structures
typedef struct {
    uint16_t magic;
    uint32_t file_size;
    uint32_t reserved;
    uint32_t data_offset;
} BMPFileHeader;

typedef struct {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
} BMPInfoHeader;

// Read BMP file and extract character data
uint16_t* read_bmp_font(const char* filename, int* out_width, int* out_height) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open '%s'\n", filename);
        return NULL;
    }

    // Read file header
    BMPFileHeader fh;
    fread(&fh.magic, sizeof(uint16_t), 1, f);
    fread(&fh.file_size, sizeof(uint32_t), 1, f);
    fread(&fh.reserved, sizeof(uint32_t), 1, f);
    fread(&fh.data_offset, sizeof(uint32_t), 1, f);

    if (fh.magic != 0x4D42) { // "BM"
        fprintf(stderr, "Error: Not a valid BMP file\n");
        fclose(f);
        return NULL;
    }

    // Read info header
    BMPInfoHeader ih;
    fread(&ih.header_size, sizeof(uint32_t), 1, f);
    fread(&ih.width, sizeof(int32_t), 1, f);
    fread(&ih.height, sizeof(int32_t), 1, f);
    fread(&ih.planes, sizeof(uint16_t), 1, f);
    fread(&ih.bits_per_pixel, sizeof(uint16_t), 1, f);
    fread(&ih.compression, sizeof(uint32_t), 1, f);

    *out_width = ih.width;
    *out_height = ih.height;

    printf("BMP Info: %dx%d, %d bits per pixel\n", ih.width, ih.height, ih.bits_per_pixel);

    if (ih.bits_per_pixel != 1 && ih.bits_per_pixel != 4 && ih.bits_per_pixel != 8 &&
        ih.bits_per_pixel != 24 && ih.bits_per_pixel != 32) {
        fprintf(stderr, "Error: Only 1-bit, 4-bit, 8-bit, 24-bit and 32-bit BMPs supported\n");
        fclose(f);
        return NULL;
    }

    int bytes_per_pixel = (ih.bits_per_pixel + 7) / 8;
    if (ih.bits_per_pixel == 1 || ih.bits_per_pixel == 4) bytes_per_pixel = 0; // Special case

    // Read palette if present (for 1-bit, 4-bit, 8-bit)
    uint8_t palette[256 * 4] = {0}; // Max 256 colors, 4 bytes each (BGRA)
    if (ih.bits_per_pixel <= 8) {
        int num_colors = 1 << ih.bits_per_pixel; // 2^bits_per_pixel
        fseek(f, 54, SEEK_SET); // Standard palette location
        fread(palette, 4, num_colors, f);
    }

    // Read pixel data
    fseek(f, fh.data_offset, SEEK_SET);
    int stride = ((ih.width * ih.bits_per_pixel + 31) / 32) * 4;
    uint8_t* pixels = malloc(stride * ih.height);
    fread(pixels, 1, stride * ih.height, f);
    fclose(f);

    // Extract 8-bit font data in parallel ROM layout
    // 4 ROMs × 256 addresses = 1024 entries (each 16-bit)
    // Each ROM stores 2 rows packed into one 16-bit word per address
    // Valid ASCII codes 32-127 are populated; others remain 0x0000
    uint16_t* font_data = calloc(ROM_COUNT * ROM_SIZE, sizeof(uint16_t));

    for (int char_idx = 0; char_idx < CHAR_COUNT; char_idx++) {
        int col = (char_idx % 16) * CHAR_SIZE;
        int row = (char_idx / 16) * CHAR_SIZE;

        for (int y = 0; y < CHAR_SIZE; y++) {
            uint8_t line = 0;

            for (int x = 0; x < CHAR_SIZE; x++) {
                int bmp_x = col + x;
                int bmp_y = ih.height - 1 - (row + y);  // BMP is bottom-up, flip to top-down
                int pixel_idx = bmp_y * stride + bmp_x * bytes_per_pixel;
                uint8_t pixel_val = 0;

                if (ih.bits_per_pixel == 1) {
                    // 1-bit: each bit is a pixel
                    int byte_idx = bmp_y * stride + (bmp_x / 8);
                    int bit_idx = 7 - (bmp_x % 8);
                    int color_idx = (pixels[byte_idx] >> bit_idx) & 1;
                    uint8_t b = palette[color_idx * 4];
                    uint8_t g = palette[color_idx * 4 + 1];
                    uint8_t r = palette[color_idx * 4 + 2];
                    pixel_val = (r > 200 && g > 200 && b > 200) ? 255 : 0;
                } else if (ih.bits_per_pixel == 4) {
                    // 4-bit: 2 pixels per byte
                    int byte_idx = bmp_y * stride + (bmp_x / 2);
                    int nibble_idx = 1 - (bmp_x % 2);
                    int color_idx = (pixels[byte_idx] >> (nibble_idx * 4)) & 0xF;
                    uint8_t b = palette[color_idx * 4];
                    uint8_t g = palette[color_idx * 4 + 1];
                    uint8_t r = palette[color_idx * 4 + 2];
                    pixel_val = (r > 200 && g > 200 && b > 200) ? 255 : 0;
                } else if (ih.bits_per_pixel == 8) {
                    // 8-bit: palette index
                    int color_idx = pixels[pixel_idx];
                    uint8_t b = palette[color_idx * 4];
                    uint8_t g = palette[color_idx * 4 + 1];
                    uint8_t r = palette[color_idx * 4 + 2];
                    pixel_val = (r > 200 && g > 200 && b > 200) ? 255 : 0;
                } else if (ih.bits_per_pixel == 24) {
                    // 24-bit: RGB
                    uint8_t r = pixels[pixel_idx];
                    uint8_t g = pixels[pixel_idx + 1];
                    uint8_t b = pixels[pixel_idx + 2];
                    pixel_val = (r > 200 && g > 200 && b > 200) ? 255 : 0;
                } else if (ih.bits_per_pixel == 32) {
                    // 32-bit: RGBA or ARGB
                    uint8_t r = pixels[pixel_idx];
                    uint8_t g = pixels[pixel_idx + 1];
                    uint8_t b = pixels[pixel_idx + 2];
                    pixel_val = (r > 200 && g > 200 && b > 200) ? 255 : 0;
                }

                // White/bright pixel is 1, dark is 0
                if (pixel_val > 127) {
                    line |= (1 << x);
                }
            }

            // Pack 2 rows into one 16-bit word per ROM
            // ALPHA (rom_idx=0): rows 0-1 (displayed as 1-2), upper byte = row 0, lower byte = row 1
            int rom_idx = y / 2;
            int row_in_rom = y % 2;
            int ascii_code = CHAR_START + char_idx;  // 32-127
            int addr = rom_idx * ROM_SIZE + ascii_code;  // Use ASCII code as address

            // Invert byte order: first row goes to upper byte, second row to lower byte
            font_data[addr] |= ((uint16_t)line & 0xFF) << ((1 - row_in_rom) * 8);
        }
    }

    free(pixels);
    return font_data;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_bmp> [-f FORMAT]\n", argv[0]);
        fprintf(stderr, "Formats: hex, uint, int, binary (default: hex)\n");
        return -1;
    }

    const char* input_file = argv[1];
    rom_format fmt = ROM_HEX;

    // Parse optional format flag
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            const char* fmt_str = argv[++i];
            if (strcmp(fmt_str, "hex") == 0) fmt = ROM_HEX;
            else if (strcmp(fmt_str, "uint") == 0) fmt = ROM_UINT;
            else if (strcmp(fmt_str, "int") == 0) fmt = ROM_INT;
            else if (strcmp(fmt_str, "binary") == 0) fmt = ROM_BINARY;
            else {
                fprintf(stderr, "Error: Unknown format '%s'\n", fmt_str);
                return -1;
            }
        }
    }

    int width, height;
    uint16_t* font_data = read_bmp_font(input_file, &width, &height);
    if (!font_data) return -1;

    printf("Read font atlas: %dx%d\n", width, height);
    printf("Extracted %d characters, mapped to addresses 0-255\n", CHAR_COUNT);
    printf("Allocated %d 16-bit words (%d per ROM × %d ROMs)\n",
           ROM_COUNT * ROM_SIZE, ROM_SIZE, ROM_COUNT);

    // Extract individual ROM arrays and write them
    const char* rom_names[] = {"ROM_ALPHA", "ROM_BRAVO", "ROM_CHARLIE", "ROM_DELTA"};
    char filename[256];

    printf("Writing %d parallel ROM files to v2/text/out/...\n", ROM_COUNT);

    for (int rom_idx = 0; rom_idx < ROM_COUNT; rom_idx++) {
        // Extract this ROM's data from the combined array
        uint16_t* rom_data = malloc(ROM_SIZE * sizeof(uint16_t));
        int start_addr = rom_idx * ROM_SIZE;
        for (int i = 0; i < ROM_SIZE; i++) {
            rom_data[i] = font_data[start_addr + i];
        }

        sprintf(filename, "v2/text/out/%s.out", rom_names[rom_idx]);
        if (write_rom_file(filename, rom_data, ROM_SIZE, fmt) < 0) {
            fprintf(stderr, "Error: Failed to write %s\n", filename);
            free(rom_data);
            free(font_data);
            return -1;
        }

        printf("Wrote %s\n", filename);
        free(rom_data);
    }

    free(font_data);
    printf("Done!\n");
    return 0;
}