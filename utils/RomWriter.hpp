#ifndef ROM_WRITER_HPP
#define ROM_WRITER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

enum RomFormat { ROM_HEX, ROM_UINT, ROM_INT, ROM_BINARY };

class RomWriter {
private:
    static const int ROM_SIZE = 256;
    std::string filename;
    std::vector<uint16_t> data;
    RomFormat format;

    void createDirectories(const std::string& filepath) {
        std::string path = filepath;
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash == std::string::npos) return;
        path = path.substr(0, lastSlash);
        #ifdef _WIN32
        char separator = '\\';
        #else
        char separator = '/';
        #endif
        std::string current;
        for (size_t i = 0; i < path.length(); ++i) {
            if (path[i] == '/' || path[i] == '\\') {
                if (!current.empty()) MKDIR(current.c_str());
                current += separator;
            } else current += path[i];
        }
        if (!current.empty()) MKDIR(current.c_str());
    }

    std::string formatToString(RomFormat fmt) const {
        switch (fmt) {
            case ROM_HEX: return "hex";
            case ROM_UINT: return "uint";
            case ROM_INT: return "int";
            case ROM_BINARY: return "binary";
            default: return "unknown";
        }
    }

public:
    RomWriter(const std::string& filename, RomFormat format = ROM_HEX)
        : filename(filename), format(format) {
        data.resize(ROM_SIZE, 0);
    }

    RomWriter(const std::string& filename, const std::string& formatStr)
        : filename(filename) {
        data.resize(ROM_SIZE, 0);
        if (formatStr == "hex") format = ROM_HEX;
        else if (formatStr == "uint") format = ROM_UINT;
        else if (formatStr == "int") format = ROM_INT;
        else if (formatStr == "binary") format = ROM_BINARY;
        else format = ROM_HEX;
    }

    void set(uint8_t address, uint16_t value) { data[address] = value; }
    uint16_t get(uint8_t address) const { return data[address]; }

    bool writeToFile() {
        createDirectories(filename);
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "Error: Cannot write to '" << filename << "'\n";
            return false;
        }
        for (int i = 0; i < ROM_SIZE; ++i) {
            switch (format) {
                case ROM_HEX:
                    out << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << data[i] << "\n";
                    break;
                case ROM_UINT:
                    out << std::dec << data[i] << "\n";
                    break;
                case ROM_INT:
                    out << std::dec << (int16_t)data[i] << "\n";
                    break;
                case ROM_BINARY:
                    for (int b = 15; b >= 0; --b)
                        out << ((data[i] >> b) & 1 ? '1' : '0');
                    out << "\n";
                    break;
            }
        }
        out.close();
        std::cout << "Wrote ROM: " << filename << " (" << ROM_SIZE << " entries, " << formatToString(format) << " format)\n";
        return true;
    }

    std::string getFilename() const { return filename; }
    int getSize() const { return ROM_SIZE; }
    RomFormat getFormat() const { return format; }
};

#endif
