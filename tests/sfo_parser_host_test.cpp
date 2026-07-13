#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "../src/sfo_parser.h"

namespace {
std::string JoinPath(const std::string &base, const std::string &name) {
    return base + "/" + name;
}

void WriteBytes(const std::string &path, const std::vector<unsigned char> &bytes) {
    FILE *file = fopen(path.c_str(), "wb");
    assert(file != NULL);
    assert(fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size());
    fclose(file);
}

void AppendBytes(std::vector<unsigned char> *bytes, const void *data, size_t size) {
    const unsigned char *cursor = static_cast<const unsigned char*>(data);
    bytes->insert(bytes->end(), cursor, cursor + size);
}

std::vector<unsigned char> BuildSfo() {
    const char keyTable[] = "TITLE\0APP_VER\0";
    const size_t dataTitleOffset = 0;
    const size_t dataVersionOffset = 16;
    char dataTable[32];
    memset(dataTable, 0, sizeof(dataTable));
    strcpy(dataTable + dataTitleOffset, "Kylin Test");
    strcpy(dataTable + dataVersionOffset, "1.23");

    SfoHeader header;
    header.magic = 0x46535000;
    header.version = 0x00010100;
    header.key_table_off = sizeof(SfoHeader) + 2 * sizeof(SfoEntry);
    header.data_table_off = header.key_table_off + sizeof(keyTable);
    header.total_entries = 2;

    SfoEntry title;
    memset(&title, 0, sizeof(title));
    title.key_off = 0;
    title.data_len = 11;
    title.data_max_len = 16;
    title.data_off = dataTitleOffset;

    SfoEntry version;
    memset(&version, 0, sizeof(version));
    version.key_off = 6;
    version.data_len = 5;
    version.data_max_len = 16;
    version.data_off = dataVersionOffset;

    std::vector<unsigned char> bytes;
    AppendBytes(&bytes, &header, sizeof(header));
    AppendBytes(&bytes, &title, sizeof(title));
    AppendBytes(&bytes, &version, sizeof(version));
    AppendBytes(&bytes, keyTable, sizeof(keyTable));
    AppendBytes(&bytes, dataTable, sizeof(dataTable));
    return bytes;
}

std::vector<unsigned char> BuildPkg(const char *contentId) {
    std::vector<unsigned char> bytes(0x40 + 36, 0);
    bytes[0] = 0x7F;
    bytes[1] = 'C';
    bytes[2] = 'N';
    bytes[3] = 'T';
    const size_t contentIdLength = strlen(contentId);
    memcpy(&bytes[0x40], contentId, contentIdLength < 36 ? contentIdLength : 36);
    return bytes;
}
}

int main() {
    char rootTemplate[] = "/tmp/kylin-sfo-parser-XXXXXX";
    const std::string root = mkdtemp(rootTemplate);

    const std::string sfoPath = JoinPath(root, "param.sfo");
    WriteBytes(sfoPath, BuildSfo());
    std::string title;
    std::string version;
    assert(ParseSfo(sfoPath, title, version));
    assert(title == "Kylin Test");
    assert(version == "1.23");

    const std::string shortSfoPath = JoinPath(root, "short.sfo");
    std::vector<unsigned char> shortSfo(3, 0);
    WriteBytes(shortSfoPath, shortSfo);
    title = "unchanged";
    version = "unchanged";
    assert(!ParseSfo(shortSfoPath, title, version));

    const std::string badSfoPath = JoinPath(root, "bad.sfo");
    std::vector<unsigned char> badSfo = BuildSfo();
    badSfo[0] = 0xFF;
    WriteBytes(badSfoPath, badSfo);
    title.clear();
    version.clear();
    assert(!ParseSfo(badSfoPath, title, version));

    const std::string pkgPath = JoinPath(root, "sample.pkg");
    WriteBytes(pkgPath, BuildPkg("IV0000-KYLN00002_00-KYLINEXPLORER"));
    std::string contentId;
    std::string titleId;
    assert(ReadPkgMetadata(pkgPath, contentId, titleId));
    assert(contentId == "IV0000-KYLN00002_00-KYLINEXPLORER");
    assert(titleId == "KYLN00002");

    const std::string unknownTitlePkgPath = JoinPath(root, "unknown-title.pkg");
    WriteBytes(unknownTitlePkgPath, BuildPkg("CONTENT_WITHOUT_TITLE_FORMAT"));
    assert(ReadPkgMetadata(unknownTitlePkgPath, contentId, titleId));
    assert(contentId == "CONTENT_WITHOUT_TITLE_FORMAT");
    assert(titleId == "UNKNOWN");

    const std::string shortPkgPath = JoinPath(root, "short.pkg");
    std::vector<unsigned char> shortPkg(0x40 + 8, 0);
    shortPkg[0] = 0x7F;
    shortPkg[1] = 'C';
    shortPkg[2] = 'N';
    shortPkg[3] = 'T';
    WriteBytes(shortPkgPath, shortPkg);
    assert(!ReadPkgMetadata(shortPkgPath, contentId, titleId));

    const std::string badPkgPath = JoinPath(root, "bad.pkg");
    std::vector<unsigned char> badPkg = BuildPkg("IV0000-KYLN00002_00-KYLINEXPLORER");
    badPkg[0] = 0;
    WriteBytes(badPkgPath, badPkg);
    assert(!ReadPkgMetadata(badPkgPath, contentId, titleId));

    printf("sfo-parser-host-tests-ok\n");
    return 0;
}
