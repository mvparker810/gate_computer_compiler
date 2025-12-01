#include "../utils/rom_writer.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

// Create nested directories
static void create_directory(const char* path) {
#ifdef _WIN32
    char temp[512];
    strncpy(temp, path, 511);
    temp[511] = '\0';
    for (char* p = temp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            _mkdir(temp);
            *p = '/';
        }
    }
    _mkdir(temp);
#else
    char temp[512];
    strncpy(temp, path, 511);
    temp[511] = '\0';
    for (char* p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp, 0755);
            *p = '/';
        }
    }
    mkdir(temp, 0755);
#endif
}

int write_rom_file(const char* filename, uint16_t* data, int size, rom_format format) {
    if (!filename || !data || size <= 0) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }

    // Create directories if needed
    char dir_path[512];
    strncpy(dir_path, filename, 511);
    dir_path[511] = '\0';

    char* last_slash = strrchr(dir_path, '/');
    if (!last_slash) last_slash = strrchr(dir_path, '\\');

    if (last_slash) {
        *last_slash = '\0';
        create_directory(dir_path);
    }

    // Open file
    FILE* out = fopen(filename, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot write to '%s'\n", filename);
        return -1;
    }

    // Write data in specified format
    for (int i = 0; i < size; i++) {
        switch (format) {
            case ROM_HEX:
                fprintf(out, "%04X\n", data[i]);
                break;
            case ROM_UINT:
                fprintf(out, "%u\n", data[i]);
                break;
            case ROM_INT:
                fprintf(out, "%d\n", (int16_t)data[i]);
                break;
            case ROM_BINARY:
                for (int b = 15; b >= 0; b--) {
                    fputc((data[i] >> b) & 1 ? '1' : '0', out);
                }
                fputc('\n', out);
                break;
        }
    }

    fclose(out);
    return 0;
}
