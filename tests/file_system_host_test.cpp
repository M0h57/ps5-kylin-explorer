#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/file_system.h"

namespace {
std::string JoinPath(const std::string &base, const std::string &name) {
    return base + "/" + name;
}

void WriteFile(const std::string &path, const char *content) {
    FILE *file = fopen(path.c_str(), "wb");
    assert(file != NULL);
    fputs(content, file);
    fclose(file);
}
}

int main() {
    DisplayName chinese = DisplayCodec::DecodeBytes("中文.txt");
    assert(!chinese.lossy);
    assert(chinese.text == "中文.txt");

    std::string invalid = "bad-";
    invalid.push_back(static_cast<char>(0xC0));
    invalid.push_back(static_cast<char>(0xAF));
    invalid += "-name";
    DisplayName invalidDisplay = DisplayCodec::DecodeBytes(invalid);
    assert(invalidDisplay.lossy);
    assert(invalidDisplay.text.find("\\xC0\\xAF") != std::string::npos);

    std::string control = "line";
    control.push_back('\n');
    control += "break";
    DisplayName controlDisplay = DisplayCodec::DecodeBytes(control);
    assert(controlDisplay.lossy);
    assert(controlDisplay.text == "line\\x0Abreak");

    std::string delName = "del";
    delName.push_back(static_cast<char>(0x7F));
    DisplayName delDisplay = DisplayCodec::DecodeBytes(delName);
    assert(delDisplay.lossy);
    assert(delDisplay.text == "del\\x7F");

    std::string fourByteUtf8;
    fourByteUtf8.push_back(static_cast<char>(0xF0));
    fourByteUtf8.push_back(static_cast<char>(0x9F));
    fourByteUtf8.push_back(static_cast<char>(0x98));
    fourByteUtf8.push_back(static_cast<char>(0x80));
    DisplayName fourByteDisplay = DisplayCodec::DecodeBytes(fourByteUtf8);
    assert(!fourByteDisplay.lossy);
    assert(fourByteDisplay.text == fourByteUtf8);

    std::string surrogate;
    surrogate.push_back(static_cast<char>(0xED));
    surrogate.push_back(static_cast<char>(0xA0));
    surrogate.push_back(static_cast<char>(0x80));
    DisplayName surrogateDisplay = DisplayCodec::DecodeBytes(surrogate);
    assert(surrogateDisplay.lossy);
    assert(surrogateDisplay.text == "\\xED\\xA0\\x80");

    assert(FsName::IsValidBytes("valid"));
    assert(!FsName::IsValidBytes(""));
    assert(!FsName::IsValidBytes("."));
    assert(!FsName::IsValidBytes(".."));
    assert(!FsName::IsValidBytes("a/b"));
    std::string nulName = "a";
    nulName.push_back('\0');
    nulName += "b";
    assert(!FsName::IsValidBytes(nulName));

    FsPath root("/");
    assert(FsPath("").Bytes() == "/");
    FsName name("child");
    assert(!name.Display().lossy);
    assert(name.Display().text == "child");
    assert(root.Join(name).Bytes() == "/child");
    assert(FsPath("/tmp").Join(name).Bytes() == "/tmp/child");

    assert(IsRegularFile(FileKind::Regular));
    assert(!IsRegularFile(FileKind::Directory));
    assert(IsDirectory(FileKind::Directory));
    assert(!IsDirectory(FileKind::Symlink));
    assert(IsSpecialFile(FileKind::Symlink));
    assert(IsSpecialFile(FileKind::Fifo));
    assert(IsSpecialFile(FileKind::Socket));
    assert(IsSpecialFile(FileKind::Device));
    assert(IsSpecialFile(FileKind::Unknown));

    char rootTemplate[] = "/tmp/kylin-file-system-XXXXXX";
    const std::string tempRoot = mkdtemp(rootTemplate);
    const std::string targetDirectory = JoinPath(tempRoot, "target");
    const std::string linkPath = JoinPath(tempRoot, "link");
    assert(mkdir(targetDirectory.c_str(), 0777) == 0);
    WriteFile(JoinPath(targetDirectory, "keep.txt"), "keep");
    assert(symlink(targetDirectory.c_str(), linkPath.c_str()) == 0);

    PosixFileSystem fileSystem;
    FsMetadata linkMetadata;
    std::string error;
    assert(fileSystem.Lstat(FsPath(linkPath), &linkMetadata, &error));
    assert(linkMetadata.kind == FileKind::Symlink);
    assert(IsSpecialFile(linkMetadata.kind));

    const std::string fifoPath = JoinPath(tempRoot, "fifo");
    assert(mkfifo(fifoPath.c_str(), 0600) == 0);
    FsMetadata fifoMetadata;
    assert(fileSystem.Lstat(FsPath(fifoPath), &fifoMetadata, &error));
    assert(fifoMetadata.kind == FileKind::Fifo);
    assert(IsSpecialFile(fifoMetadata.kind));

    assert(!fileSystem.Lstat(FsPath(JoinPath(tempRoot, "missing")), &fifoMetadata, &error));
    assert(!error.empty());

    std::vector<FsDirectoryEntry> entries;
    assert(fileSystem.ListDirectory(FsPath(tempRoot), &entries, &error));
    bool sawLink = false;
    bool sawDirectory = false;
    for (size_t index = 0; index < entries.size(); ++index) {
        if (entries[index].name.Bytes() == "link") {
            sawLink = true;
            assert(entries[index].metadata.kind == FileKind::Directory);
        }
        if (entries[index].name.Bytes() == "target") {
            sawDirectory = true;
            assert(entries[index].metadata.kind == FileKind::Directory);
        }
    }
    assert(sawLink);
    assert(sawDirectory);

    entries.clear();
    assert(!fileSystem.ListDirectory(FsPath(JoinPath(tempRoot, "missing-dir")), &entries, &error));
    assert(entries.empty());

    printf("file-system-host-tests-ok\n");
    return 0;
}
