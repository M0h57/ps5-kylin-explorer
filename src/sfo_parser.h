#ifndef KYLIN_EXPLORER_SFO_PARSER_H
#define KYLIN_EXPLORER_SFO_PARSER_H

#include <stdint.h>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct SfoHeader {
    uint32_t magic;         // "PSF\0" (0x46535000)
    uint32_t version;       // Typically 0x00010100
    uint32_t key_table_off; // Offset to the key table
    uint32_t data_table_off;// Offset to the data table
    uint32_t total_entries; // Number of entries
};

struct SfoEntry {
    uint16_t key_off;       // Offset into the key table (name of the field)
    uint16_t align;         // Data format/alignment
    uint32_t data_len;      // Length of the data
    uint32_t data_max_len;  // Maximum length of the data
    uint32_t data_off;      // Offset into the data table (value of the field)
};
#pragma pack(pop)

struct InstalledApp {
    std::string title;
    std::string titleId;
    std::string version;
    std::string path;
};

// Parses a standalone SFO file (e.g. /sce_sys/param.sfo)
bool ParseSfo(const std::string &path, std::string &title, std::string &version);

// Reads Content ID and extracts Title ID from a PS4/PS5 PKG header at 0x40
bool ReadPkgMetadata(const std::string &path, std::string &contentId, std::string &titleId);

#endif
