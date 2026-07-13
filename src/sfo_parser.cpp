#include "sfo_parser.h"
#include <stdio.h>
#include <string.h>

bool ParseSfo(const std::string &path, std::string &title, std::string &version) {
    FILE *file = fopen(path.c_str(), "rb");
    if (file == NULL) return false;

    SfoHeader header;
    if (fread(&header, 1, sizeof(SfoHeader), file) != sizeof(SfoHeader)) {
        fclose(file);
        return false;
    }

    if (header.magic != 0x46535000) { // "PSF\0"
        fclose(file);
        return false;
    }

    std::vector<SfoEntry> entries(header.total_entries);
    if (fread(&entries[0], sizeof(SfoEntry), header.total_entries, file) != header.total_entries) {
        fclose(file);
        return false;
    }

    // Read Key table
    long keyTableSize = static_cast<long>(header.data_table_off - header.key_table_off);
    if (keyTableSize <= 0) {
        fclose(file);
        return false;
    }
    std::vector<char> keyTable(keyTableSize);
    fseek(file, header.key_table_off, SEEK_SET);
    if (fread(&keyTable[0], 1, keyTableSize, file) != static_cast<size_t>(keyTableSize)) {
        fclose(file);
        return false;
    }

    // Read Data table
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    long dataTableSize = fileSize - static_cast<long>(header.data_table_off);
    if (dataTableSize <= 0) {
        fclose(file);
        return false;
    }
    std::vector<char> dataTable(dataTableSize);
    fseek(file, header.data_table_off, SEEK_SET);
    if (fread(&dataTable[0], 1, dataTableSize, file) != static_cast<size_t>(dataTableSize)) {
        fclose(file);
        return false;
    }

    fclose(file);

    for (uint32_t i = 0; i < header.total_entries; ++i) {
        // Safe key lookup
        uint16_t keyOff = entries[i].key_off;
        if (keyOff >= keyTableSize) continue;
        const char *key = &keyTable[keyOff];

        // Safe data lookup
        uint32_t dataOff = entries[i].data_off;
        if (dataOff >= static_cast<uint32_t>(dataTableSize)) continue;
        const char *value = &dataTable[dataOff];

        if (strcmp(key, "TITLE") == 0) {
            title = value;
        } else if (strcmp(key, "APP_VER") == 0 || strcmp(key, "VERSION") == 0) {
            version = value;
        }
    }

    return true;
}

bool ReadPkgMetadata(const std::string &path, std::string &contentId, std::string &titleId) {
    FILE *file = fopen(path.c_str(), "rb");
    if (file == NULL) return false;

    unsigned char magic[4];
    if (fread(magic, 1, 4, file) != 4) {
        fclose(file);
        return false;
    }

    // PS4 PKG magic is 0x7F434E54 ("\x7FCNT")
    if (magic[0] != 0x7F || magic[1] != 0x43 || magic[2] != 0x4E || magic[3] != 0x54) {
        fclose(file);
        return false;
    }

    // Content ID is at offset 0x40 and is 36 bytes long
    if (fseek(file, 0x40, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    char contentBuffer[37];
    memset(contentBuffer, 0, sizeof(contentBuffer));
    if (fread(contentBuffer, 1, 36, file) != 36) {
        fclose(file);
        return false;
    }

    fclose(file);

    contentId = contentBuffer;

    // Extract Title ID from Content ID (typically between first hyphen and first underscore, e.g. UP0001-CUSA12345_00)
    size_t hyphen = contentId.find('-');
    size_t underscore = contentId.find('_');
    if (hyphen != std::string::npos && underscore != std::string::npos && underscore > hyphen + 1) {
        titleId = contentId.substr(hyphen + 1, underscore - hyphen - 1);
    } else {
        // Fallback or partial format
        titleId = "UNKNOWN";
    }

    return true;
}
