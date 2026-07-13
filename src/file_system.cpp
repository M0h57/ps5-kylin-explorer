#include "file_system.h"

#include "logger.h"
#include "utils.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

namespace {
bool IsContinuation(unsigned char value) {
    return (value & 0xC0) == 0x80;
}

bool DecodeStrictUtf8(const std::string &bytes, size_t offset, size_t *nextOffset) {
    const unsigned char first = static_cast<unsigned char>(bytes[offset]);
    if (first <= 0x7F) {
        *nextOffset = offset + 1;
        return true;
    }

    const size_t remaining = bytes.size() - offset;
    if (first >= 0xC2 && first <= 0xDF) {
        if (remaining < 2) return false;
        if (!IsContinuation(static_cast<unsigned char>(bytes[offset + 1]))) return false;
        *nextOffset = offset + 2;
        return true;
    }

    if (first == 0xE0) {
        if (remaining < 3) return false;
        const unsigned char second = static_cast<unsigned char>(bytes[offset + 1]);
        const unsigned char third = static_cast<unsigned char>(bytes[offset + 2]);
        if (second < 0xA0 || second > 0xBF || !IsContinuation(third)) return false;
        *nextOffset = offset + 3;
        return true;
    }
    if ((first >= 0xE1 && first <= 0xEC) || (first >= 0xEE && first <= 0xEF)) {
        if (remaining < 3) return false;
        if (!IsContinuation(static_cast<unsigned char>(bytes[offset + 1])) ||
            !IsContinuation(static_cast<unsigned char>(bytes[offset + 2]))) {
            return false;
        }
        *nextOffset = offset + 3;
        return true;
    }
    if (first == 0xED) {
        if (remaining < 3) return false;
        const unsigned char second = static_cast<unsigned char>(bytes[offset + 1]);
        const unsigned char third = static_cast<unsigned char>(bytes[offset + 2]);
        if (second < 0x80 || second > 0x9F || !IsContinuation(third)) return false;
        *nextOffset = offset + 3;
        return true;
    }

    if (first == 0xF0) {
        if (remaining < 4) return false;
        const unsigned char second = static_cast<unsigned char>(bytes[offset + 1]);
        if (second < 0x90 || second > 0xBF) return false;
        if (!IsContinuation(static_cast<unsigned char>(bytes[offset + 2])) ||
            !IsContinuation(static_cast<unsigned char>(bytes[offset + 3]))) {
            return false;
        }
        *nextOffset = offset + 4;
        return true;
    }
    if (first >= 0xF1 && first <= 0xF3) {
        if (remaining < 4) return false;
        if (!IsContinuation(static_cast<unsigned char>(bytes[offset + 1])) ||
            !IsContinuation(static_cast<unsigned char>(bytes[offset + 2])) ||
            !IsContinuation(static_cast<unsigned char>(bytes[offset + 3]))) {
            return false;
        }
        *nextOffset = offset + 4;
        return true;
    }
    if (first == 0xF4) {
        if (remaining < 4) return false;
        const unsigned char second = static_cast<unsigned char>(bytes[offset + 1]);
        if (second < 0x80 || second > 0x8F) return false;
        if (!IsContinuation(static_cast<unsigned char>(bytes[offset + 2])) ||
            !IsContinuation(static_cast<unsigned char>(bytes[offset + 3]))) {
            return false;
        }
        *nextOffset = offset + 4;
        return true;
    }

    return false;
}

void AppendEscapedByte(std::string *output, unsigned char value) {
    static const char kHex[] = "0123456789ABCDEF";
    output->push_back('\\');
    output->push_back('x');
    output->push_back(kHex[value >> 4]);
    output->push_back(kHex[value & 0x0F]);
}

FileKind KindFromMode(mode_t mode) {
    if (S_ISREG(mode)) return FileKind::Regular;
    if (S_ISDIR(mode)) return FileKind::Directory;
    if (S_ISLNK(mode)) return FileKind::Symlink;
    if (S_ISFIFO(mode)) return FileKind::Fifo;
    if (S_ISSOCK(mode)) return FileKind::Socket;
    if (S_ISCHR(mode) || S_ISBLK(mode)) return FileKind::Device;
    return FileKind::Unknown;
}

void MetadataFromStat(const struct stat &st, FsMetadata *metadata) {
    metadata->kind = KindFromMode(st.st_mode);
    const unsigned char *raw = reinterpret_cast<const unsigned char *>(&st);
    uint64_t size = static_cast<uint64_t>(st.st_size);
    // PS5 PS4-compat layer puts st_size at offset 72 (not the compiler's st.st_size offset)
    // and always fills st_blocks with a constant 65536. Detect this and read from offset 72.
    if (st.st_blocks == 65536) {
        uint64_t sizeAt72;
        memcpy(&sizeAt72, raw + 72, sizeof(sizeAt72));
        if (sizeAt72 != size) size = sizeAt72;
    }
    metadata->size = size;
    metadata->modifiedTime = static_cast<uint64_t>(st.st_mtime);
    metadata->mode = static_cast<uint32_t>(st.st_mode);
    metadata->device = static_cast<uint64_t>(st.st_dev);
    metadata->inode = static_cast<uint64_t>(st.st_ino);
}
}

DisplayName DisplayCodec::DecodeBytes(const std::string &bytes) {
    std::string output;
    bool lossy = false;
    size_t offset = 0;
    while (offset < bytes.size()) {
        size_t nextOffset = offset + 1;
        const unsigned char first = static_cast<unsigned char>(bytes[offset]);
        if (DecodeStrictUtf8(bytes, offset, &nextOffset)) {
            if (first < 0x20 || first == 0x7F) {
                AppendEscapedByte(&output, first);
                lossy = true;
            } else {
                output.append(bytes.data() + offset, nextOffset - offset);
            }
            offset = nextOffset;
        } else {
            AppendEscapedByte(&output, first);
            lossy = true;
            ++offset;
        }
    }
    return DisplayName(output, lossy);
}

FsName::FsName() : bytes_(), display_() {}

FsName::FsName(const std::string &bytes)
    : bytes_(bytes), display_(DisplayCodec::DecodeBytes(bytes)) {}

bool FsName::IsValidBytes(const std::string &bytes) {
    if (bytes.empty() || bytes == "." || bytes == "..") return false;
    for (size_t index = 0; index < bytes.size(); ++index) {
        if (bytes[index] == '\0' || bytes[index] == '/') return false;
    }
    return true;
}

const std::string &FsName::Bytes() const { return bytes_; }
const DisplayName &FsName::Display() const { return display_; }

FsPath::FsPath() : bytes_("/"), display_(DisplayCodec::DecodeBytes(bytes_)) {}

FsPath::FsPath(const std::string &bytes)
    : bytes_(bytes.empty() ? "/" : bytes), display_(DisplayCodec::DecodeBytes(bytes_)) {}

const std::string &FsPath::Bytes() const { return bytes_; }
const DisplayName &FsPath::Display() const { return display_; }

FsPath FsPath::Join(const FsName &name) const {
    return FsPath(JoinPath(bytes_, name.Bytes()));
}

FsMetadata::FsMetadata()
    : kind(FileKind::Unknown), size(0), modifiedTime(0), mode(0), device(0), inode(0) {}

bool PosixFileSystem::Lstat(const FsPath &path, FsMetadata *metadata, std::string *error) const {
    struct stat st;
    if (lstat(path.Bytes().c_str(), &st) != 0) {
        if (errno == ENOSYS) {
            if (stat(path.Bytes().c_str(), &st) != 0) {
                if (error != NULL) *error = strerror(errno);
                return false;
            }
        } else {
            if (error != NULL) *error = strerror(errno);
            return false;
        }
    }
    if (metadata != NULL) MetadataFromStat(st, metadata);
    return true;
}

bool PosixFileSystem::ListDirectory(const FsPath &path, std::vector<FsDirectoryEntry> *entries,
                                    std::string *error) const {
    if (entries == NULL) return false;
    entries->clear();

    DIR *dir = opendir(path.Bytes().c_str());
    if (dir == NULL) {
        if (error != NULL) *error = strerror(errno);
        return false;
    }

    int diagCount = 0;
    struct dirent *item;
    while ((item = readdir(dir)) != NULL) {
        const std::string nameBytes = item->d_name;
        if (nameBytes == "." || nameBytes == "..") continue;

        FsName name(nameBytes);
        FsDirectoryEntry entry;
        entry.name = name;
        entry.path = path.Join(name);
        {
            struct stat st;
            if (stat(entry.path.Bytes().c_str(), &st) == 0) {
                MetadataFromStat(st, &entry.metadata);
                if (diagCount < 5) {
                    char line[512];
                    const unsigned char *raw = reinterpret_cast<const unsigned char *>(&st);
                    int pos = snprintf(line, sizeof(line),
                        "[DIAG] %s size=%llu(0x%llx) mode=0x%x dev=0x%llx ino=0x%llx nlink=%u uid=%u gid=%u rdev=0x%x blocks=%lld blksize=%u",
                        nameBytes.c_str(),
                        (unsigned long long)st.st_size,
                        (unsigned long long)st.st_size,
                        (unsigned int)st.st_mode,
                        (unsigned long long)st.st_dev,
                        (unsigned long long)st.st_ino,
                        (unsigned int)st.st_nlink,
                        (unsigned int)st.st_uid,
                        (unsigned int)st.st_gid,
                        (unsigned int)st.st_rdev,
                        (long long)st.st_blocks,
                        (unsigned int)st.st_blksize);
                    pos += snprintf(line + pos, sizeof(line) - pos, " raw=");
                    for (int bi = 0; bi < 96 && bi < (int)sizeof(st); ++bi) {
                        pos += snprintf(line + pos, sizeof(line) - pos, "%02x", raw[bi]);
                    }
                    LOG_INFO("%s", line);
                    {
                        uint64_t sizeAt72;
                        memcpy(&sizeAt72, raw + 72, sizeof(sizeAt72));
                        if (sizeAt72 != static_cast<uint64_t>(st.st_size)) {
                            char line2[128];
                            snprintf(line2, sizeof(line2),
                                "[DIAG] %s corrected=%llu(0x%llx)",
                                nameBytes.c_str(),
                                (unsigned long long)sizeAt72,
                                (unsigned long long)sizeAt72);
                            LOG_INFO("%s", line2);
                        }
                    }
                    ++diagCount;
                }
            } else {
                if (diagCount < 5) {
                    char line[128];
                    snprintf(line, sizeof(line),
                        "[DIAG] %s stat FAILED: %s",
                        nameBytes.c_str(), strerror(errno));
                    LOG_INFO("%s", line);
                    ++diagCount;
                }
            }
        }
        entries->push_back(entry);
    }
    closedir(dir);
    return true;
}

bool IsRegularFile(FileKind kind) {
    return kind == FileKind::Regular;
}

bool IsDirectory(FileKind kind) {
    return kind == FileKind::Directory;
}

bool IsSpecialFile(FileKind kind) {
    return kind == FileKind::Symlink || kind == FileKind::Fifo ||
           kind == FileKind::Socket || kind == FileKind::Device ||
           kind == FileKind::Unknown;
}
