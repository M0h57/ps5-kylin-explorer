#ifndef KYLIN_EXPLORER_FILE_SYSTEM_H
#define KYLIN_EXPLORER_FILE_SYSTEM_H

#include <stdint.h>
#include <string>
#include <vector>

struct DisplayName {
    std::string text;
    bool lossy;

    DisplayName() : lossy(false) {}
    DisplayName(const std::string &textValue, bool lossyValue)
        : text(textValue), lossy(lossyValue) {}
};

class DisplayCodec {
public:
    static DisplayName DecodeBytes(const std::string &bytes);
};

class FsName {
public:
    FsName();
    explicit FsName(const std::string &bytes);

    static bool IsValidBytes(const std::string &bytes);

    const std::string &Bytes() const;
    const DisplayName &Display() const;

private:
    std::string bytes_;
    DisplayName display_;
};

class FsPath {
public:
    FsPath();
    explicit FsPath(const std::string &bytes);

    const std::string &Bytes() const;
    const DisplayName &Display() const;
    FsPath Join(const FsName &name) const;

private:
    std::string bytes_;
    DisplayName display_;
};

enum class FileKind {
    Regular,
    Directory,
    Symlink,
    Fifo,
    Socket,
    Device,
    Unknown,
};

struct FsMetadata {
    FileKind kind;
    uint64_t size;
    uint64_t modifiedTime;
    uint32_t mode;
    uint64_t device;
    uint64_t inode;

    FsMetadata();
};

struct FsDirectoryEntry {
    FsName name;
    FsPath path;
    FsMetadata metadata;
};

class PosixFileSystem {
public:
    bool Lstat(const FsPath &path, FsMetadata *metadata, std::string *error) const;
    bool ListDirectory(const FsPath &path, std::vector<FsDirectoryEntry> *entries,
                       std::string *error) const;
};

bool IsRegularFile(FileKind kind);
bool IsDirectory(FileKind kind);
bool IsSpecialFile(FileKind kind);

#endif
