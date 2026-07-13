#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/file_operations.h"

namespace {
std::string JoinPath(const std::string &base, const std::string &name) {
    return base + "/" + name;
}

void WriteFile(const std::string &path, const char *content) {
    FILE *file = fopen(path.c_str(), "wb");
    assert(file != NULL);
    assert(fwrite(content, 1, strlen(content), file) == strlen(content));
    fclose(file);
}

std::string ReadFile(const std::string &path) {
    FILE *file = fopen(path.c_str(), "rb");
    assert(file != NULL);
    char buffer[128];
    const size_t size = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);
    return std::string(buffer, size);
}

FileEntry Entry(const std::string &path, const std::string &name, bool directory) {
    FileEntry entry;
    entry.path = path;
    entry.name = name;
    entry.displayName = DisplayCodec::DecodeBytes(name).text;
    entry.displayPath = DisplayCodec::DecodeBytes(path).text;
    entry.preview.clear();
    entry.size = directory ? 0 : ReadFile(path).size();
    entry.modifiedTime = 0;
    entry.mode = 0;
    entry.isDirectory = directory;
    entry.displayNameLossy = DisplayCodec::DecodeBytes(name).lossy;
    entry.displayPathLossy = DisplayCodec::DecodeBytes(path).lossy;
    entry.kind = directory ? FileKind::Directory : FileKind::Regular;
    entry.type = directory ? FileType::Directory : FileType::Text;
    return entry;
}

void Complete(FileOperations &operations) {
    while (operations.IsBusy()) operations.Update();
}
}

int main() {
    char rootTemplate[] = "/tmp/kylin-forced-copy-move-XXXXXX";
    const std::string root = mkdtemp(rootTemplate);
    const std::string source = JoinPath(root, "source");
    const std::string destination = JoinPath(root, "destination");
    assert(mkdir(source.c_str(), 0777) == 0);
    assert(mkdir(destination.c_str(), 0777) == 0);

    FileOperations operations;

    const std::string fileSource = JoinPath(source, "cross-device-file.txt");
    const std::string fileTarget = JoinPath(destination, "cross-device-file.txt");
    WriteFile(fileSource, "cross-device-file");
    const FileEntry fileEntry = Entry(fileSource, "cross-device-file.txt", false);
    assert(operations.CutToClipboard(&fileEntry));
    assert(operations.StartPaste(destination));
    assert(operations.IsMoveOperation());
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(fileSource.c_str(), F_OK) != 0);
    assert(ReadFile(fileTarget) == "cross-device-file");
    operations.Acknowledge();

    const std::string directorySource = JoinPath(source, "cross-device-folder");
    const std::string directoryTarget = JoinPath(destination, "cross-device-folder");
    assert(mkdir(directorySource.c_str(), 0777) == 0);
    WriteFile(JoinPath(directorySource, "nested.txt"), "nested-move");
    const FileEntry directoryEntry = Entry(directorySource, "cross-device-folder", true);
    assert(operations.CutToClipboard(&directoryEntry));
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(directorySource.c_str(), F_OK) != 0);
    assert(ReadFile(JoinPath(directoryTarget, "nested.txt")) == "nested-move");

    printf("file-operations-forced-copy-move-host-tests-ok\n");
    return 0;
}
