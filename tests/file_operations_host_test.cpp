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

FileEntry StaleSizedEntry(const std::string &path, const std::string &name, uint64_t advertisedSize) {
    FileEntry entry = Entry(path, name, false);
    entry.size = advertisedSize;
    return entry;
}

FileEntry MissingEntry(const std::string &path, const std::string &name) {
    FileEntry entry;
    entry.path = path;
    entry.name = name;
    entry.displayName = DisplayCodec::DecodeBytes(name).text;
    entry.displayPath = DisplayCodec::DecodeBytes(path).text;
    entry.preview.clear();
    entry.size = 0;
    entry.modifiedTime = 0;
    entry.mode = 0;
    entry.isDirectory = false;
    entry.displayNameLossy = DisplayCodec::DecodeBytes(name).lossy;
    entry.displayPathLossy = DisplayCodec::DecodeBytes(path).lossy;
    entry.kind = FileKind::Regular;
    entry.type = FileType::Unknown;
    return entry;
}

void Complete(FileOperations &operations) {
    while (operations.IsBusy()) operations.Update();
}
}

int main() {
    char rootTemplate[] = "/tmp/kylin-file-operations-XXXXXX";
    const std::string root = mkdtemp(rootTemplate);
    const std::string source = JoinPath(root, "source");
    const std::string destination = JoinPath(root, "destination");
    assert(mkdir(source.c_str(), 0777) == 0);
    assert(mkdir(destination.c_str(), 0777) == 0);

    const std::string filename = "中文覆盖.txt";
    const std::string sourceFile = JoinPath(source, filename);
    const std::string destinationFile = JoinPath(destination, filename);
    WriteFile(sourceFile, "new-content");
    WriteFile(destinationFile, "old-content");

    FileOperations operations;
    const FileEntry sourceEntry = Entry(sourceFile, filename, false);
    assert(operations.CopyToClipboard(&sourceEntry));
    assert(operations.DestinationExists(destination));
    assert(operations.CanReplaceDestination(destination));
    assert(!operations.StartPaste(destination));
    assert(operations.State() == CopyState::Failed);
    assert(ReadFile(destinationFile) == "old-content");
    operations.Acknowledge();

    assert(operations.StartPaste(destination, ExistingTargetPolicy::Replace));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(ReadFile(destinationFile) == "new-content");
    operations.Acknowledge();

    const std::string sourceDirectory = JoinPath(source, "folder");
    const std::string destinationDirectory = JoinPath(destination, "folder");
    assert(mkdir(sourceDirectory.c_str(), 0777) == 0);
    assert(mkdir(destinationDirectory.c_str(), 0777) == 0);
    WriteFile(JoinPath(sourceDirectory, "nested.txt"), "nested-content");
    const FileEntry directoryEntry = Entry(sourceDirectory, "folder", true);
    assert(operations.CopyToClipboard(&directoryEntry));
    assert(operations.DestinationExists(destination));
    assert(!operations.CanReplaceDestination(destination));
    assert(!operations.StartPaste(destination, ExistingTargetPolicy::Replace));
    assert(operations.State() == CopyState::Failed);
    assert(access(JoinPath(destinationDirectory, "nested.txt").c_str(), F_OK) != 0);
    operations.Acknowledge();

    const std::string asyncDirectorySource = JoinPath(source, "async-folder");
    const std::string asyncDirectoryTarget = JoinPath(destination, "async-folder");
    assert(mkdir(asyncDirectorySource.c_str(), 0777) == 0);
    WriteFile(JoinPath(asyncDirectorySource, "payload.txt"), "payload");
    const FileEntry asyncDirectoryEntry = Entry(asyncDirectorySource, "async-folder", true);
    assert(operations.CopyToClipboard(&asyncDirectoryEntry));
    assert(operations.StartPaste(destination));
    assert(operations.IsBusy());
    assert(operations.Phase() == OperationTaskPhase::Planning);
    assert(access(asyncDirectoryTarget.c_str(), F_OK) != 0);
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(ReadFile(JoinPath(asyncDirectoryTarget, "payload.txt")) == "payload");
    operations.Acknowledge();

    const std::string moveFilename = "中文移动.txt";
    const std::string moveSource = JoinPath(source, moveFilename);
    const std::string moveTarget = JoinPath(destination, moveFilename);
    WriteFile(moveSource, "move-content");
    const FileEntry moveEntry = Entry(moveSource, moveFilename, false);
    assert(operations.CutToClipboard(&moveEntry));
    assert(operations.StartPaste(destination));
    assert(operations.IsMoveOperation());
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(moveSource.c_str(), F_OK) != 0);
    assert(ReadFile(moveTarget) == "move-content");
    operations.Acknowledge();

    const std::string replaceFilename = "中文移动覆盖.txt";
    const std::string replaceSource = JoinPath(source, replaceFilename);
    const std::string replaceTarget = JoinPath(destination, replaceFilename);
    WriteFile(replaceSource, "replacement");
    WriteFile(replaceTarget, "original");
    const FileEntry replaceEntry = Entry(replaceSource, replaceFilename, false);
    assert(operations.CutToClipboard(&replaceEntry));
    assert(!operations.StartPaste(destination));
    assert(ReadFile(replaceTarget) == "original");
    operations.Acknowledge();
    assert(operations.StartPaste(destination, ExistingTargetPolicy::Replace));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(replaceSource.c_str(), F_OK) != 0);
    assert(ReadFile(replaceTarget) == "replacement");
    operations.Acknowledge();

    const std::string sameFilename = "中文同路径.txt";
    const std::string sameSource = JoinPath(source, sameFilename);
    WriteFile(sameSource, "same-content");
    const FileEntry sameEntry = Entry(sameSource, sameFilename, false);
    assert(operations.CutToClipboard(&sameEntry));
    assert(!operations.StartPaste(source, ExistingTargetPolicy::Replace));
    assert(operations.State() == CopyState::Failed);
    assert(ReadFile(sameSource) == "same-content");
    operations.Acknowledge();

    const std::string moveDirectoryName = "中文移动目录";
    const std::string moveDirectorySource = JoinPath(source, moveDirectoryName);
    const std::string moveDirectoryTarget = JoinPath(destination, moveDirectoryName);
    assert(mkdir(moveDirectorySource.c_str(), 0777) == 0);
    WriteFile(JoinPath(moveDirectorySource, "内容.txt"), "directory-content");
    const FileEntry moveDirectoryEntry = Entry(moveDirectorySource, moveDirectoryName, true);
    assert(operations.CutToClipboard(&moveDirectoryEntry));
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(moveDirectorySource.c_str(), F_OK) != 0);
    assert(ReadFile(JoinPath(moveDirectoryTarget, "内容.txt")) == "directory-content");
    operations.Acknowledge();

    const std::string batchFileA = JoinPath(source, "批量A.txt");
    const std::string batchFileB = JoinPath(source, "批量B.txt");
    WriteFile(batchFileA, "batch-a");
    WriteFile(batchFileB, "batch-b");
    std::vector<FileEntry> batchCopy;
    batchCopy.push_back(Entry(batchFileA, "批量A.txt", false));
    batchCopy.push_back(Entry(batchFileB, "批量B.txt", false));
    assert(operations.CopyToClipboard(batchCopy));
    assert(operations.ClipboardCount() == 2);
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(ReadFile(JoinPath(destination, "批量A.txt")) == "batch-a");
    assert(ReadFile(JoinPath(destination, "批量B.txt")) == "batch-b");
    operations.Acknowledge();

    const std::string batchMoveA = JoinPath(source, "批量移动A.txt");
    const std::string batchMoveB = JoinPath(source, "批量移动B.txt");
    WriteFile(batchMoveA, "move-a");
    WriteFile(batchMoveB, "move-b");
    std::vector<FileEntry> batchMove;
    batchMove.push_back(Entry(batchMoveA, "批量移动A.txt", false));
    batchMove.push_back(Entry(batchMoveB, "批量移动B.txt", false));
    assert(operations.CutToClipboard(batchMove));
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(batchMoveA.c_str(), F_OK) != 0);
    assert(access(batchMoveB.c_str(), F_OK) != 0);
    assert(ReadFile(JoinPath(destination, "批量移动A.txt")) == "move-a");
    assert(ReadFile(JoinPath(destination, "批量移动B.txt")) == "move-b");
    operations.Acknowledge();

    const std::string deleteDirectory = JoinPath(destination, "中文递归删除");
    const std::string deleteNestedDirectory = JoinPath(deleteDirectory, "子目录");
    assert(mkdir(deleteDirectory.c_str(), 0777) == 0);
    assert(mkdir(deleteNestedDirectory.c_str(), 0777) == 0);
    WriteFile(JoinPath(deleteDirectory, "文件.txt"), "delete-content");
    WriteFile(JoinPath(deleteNestedDirectory, "深层文件.txt"), "nested-delete-content");
    const FileEntry deleteEntry = Entry(deleteDirectory, "中文递归删除", true);
    assert(operations.StartDelete(&deleteEntry));
    assert(operations.IsDeleteOperation());
    assert(!operations.CanCancel());
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(deleteDirectory.c_str(), F_OK) != 0);

    const std::string batchDeleteA = JoinPath(destination, "批量删除A.txt");
    const std::string batchDeleteB = JoinPath(destination, "批量删除B.txt");
    WriteFile(batchDeleteA, "delete-a");
    WriteFile(batchDeleteB, "delete-b");
    std::vector<FileEntry> batchDelete;
    batchDelete.push_back(Entry(batchDeleteA, "批量删除A.txt", false));
    batchDelete.push_back(Entry(batchDeleteB, "批量删除B.txt", false));
    operations.Acknowledge();
    assert(operations.StartDelete(batchDelete));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(batchDeleteA.c_str(), F_OK) != 0);
    assert(access(batchDeleteB.c_str(), F_OK) != 0);

    operations.Acknowledge();
    const std::string staleSource = JoinPath(source, "stale-size.bin");
    const std::string staleTarget = JoinPath(destination, "stale-size.bin");
    FILE *large = fopen(staleSource.c_str(), "wb");
    assert(large != NULL);
    for (int index = 0; index < 300000; ++index) fputc('x', large);
    fclose(large);
    const FileEntry staleEntry = StaleSizedEntry(staleSource, "stale-size.bin", 1);
    assert(operations.CopyToClipboard(&staleEntry));
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(operations.ProgressPercent() == 100);
    assert(operations.CopiedBytes() == operations.TotalBytes());
    assert(access(staleTarget.c_str(), F_OK) == 0);

    operations.Acknowledge();
    const std::string outsideDirectory = JoinPath(root, "outside");
    const std::string symlinkDeleteDirectory = JoinPath(destination, "symlink-delete");
    assert(mkdir(outsideDirectory.c_str(), 0777) == 0);
    assert(mkdir(symlinkDeleteDirectory.c_str(), 0777) == 0);
    WriteFile(JoinPath(outsideDirectory, "keep.txt"), "keep");
    const std::string linkPath = JoinPath(symlinkDeleteDirectory, "outside-link");
    assert(symlink(outsideDirectory.c_str(), linkPath.c_str()) == 0);
    FileEntry symlinkDeleteEntry = Entry(symlinkDeleteDirectory, "symlink-delete", true);
    assert(operations.StartDelete(&symlinkDeleteEntry));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(access(symlinkDeleteDirectory.c_str(), F_OK) != 0);
    assert(ReadFile(JoinPath(outsideDirectory, "keep.txt")) == "keep");

    operations.Acknowledge();
    const std::string zeroFile = JoinPath(source, "zero.bin");
    const std::string zeroTarget = JoinPath(destination, "zero.bin");
    WriteFile(zeroFile, "");
    const FileEntry zeroEntry = Entry(zeroFile, "zero.bin", false);
    assert(operations.CopyToClipboard(&zeroEntry));
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(operations.ProgressPercent() == 100);
    assert(access(zeroTarget.c_str(), F_OK) == 0);

    operations.Acknowledge();
    const std::string invalidNameSource = JoinPath(source, "invalid-name-source.txt");
    WriteFile(invalidNameSource, "invalid-name");
    FileEntry invalidNameEntry = Entry(invalidNameSource, "bad/name.txt", false);
    assert(operations.CopyToClipboard(&invalidNameEntry));
    assert(!operations.StartPaste(destination));
    assert(operations.State() == CopyState::Failed);
    assert(operations.Error() == "Invalid source name");

    operations.Acknowledge();
    FileEntry missingEntry = MissingEntry(JoinPath(source, "missing-source.txt"), "missing-source.txt");
    assert(operations.CopyToClipboard(&missingEntry));
    assert(!operations.StartPaste(destination));
    assert(operations.State() == CopyState::Failed);
    assert(operations.Error() == "Unable to inspect source item");

    operations.Acknowledge();
    const std::string linkTargetFile = JoinPath(source, "link-target.txt");
    const std::string linkSource = JoinPath(source, "copy-link");
    WriteFile(linkTargetFile, "link-target");
    assert(symlink(linkTargetFile.c_str(), linkSource.c_str()) == 0);
    const FileEntry linkEntry = Entry(linkSource, "copy-link", false);
    assert(operations.CopyToClipboard(&linkEntry));
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Failed);
    assert(operations.Error() == "Special file copy is not supported");
    assert(access(JoinPath(destination, "copy-link").c_str(), F_OK) != 0);

    operations.Acknowledge();
    const std::string tempConflictSource = JoinPath(source, "temp-conflict.bin");
    const std::string tempConflictExisting = JoinPath(destination, "temp-conflict.bin.kylin-copy.tmp");
    WriteFile(tempConflictSource, "temp-conflict");
    WriteFile(tempConflictExisting, "existing-temp");
    const FileEntry tempConflictEntry = Entry(tempConflictSource, "temp-conflict.bin", false);
    assert(operations.CopyToClipboard(&tempConflictEntry));
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Failed);
    assert(operations.Error() == "Destination already exists");
    assert(ReadFile(tempConflictExisting) == "existing-temp");

    operations.Acknowledge();
    const std::string cancelSource = JoinPath(source, "cancel-folder");
    const std::string cancelTarget = JoinPath(destination, "cancel-folder");
    assert(mkdir(cancelSource.c_str(), 0777) == 0);
    WriteFile(JoinPath(cancelSource, "cancel.txt"), "cancel-content");
    const FileEntry cancelEntry = Entry(cancelSource, "cancel-folder", true);
    assert(operations.CopyToClipboard(&cancelEntry));
    assert(operations.StartPaste(destination));
    operations.Update();
    assert(operations.IsBusy());
    assert(operations.CanCancel());
    operations.Cancel();
    assert(operations.State() == CopyState::Canceled);
    assert(access(cancelTarget.c_str(), F_OK) != 0);
    assert(ReadFile(JoinPath(cancelSource, "cancel.txt")) == "cancel-content");

    operations.Acknowledge();
    const std::string deleteClipboardPath = JoinPath(source, "delete-clears-clipboard.txt");
    WriteFile(deleteClipboardPath, "delete-clears");
    const FileEntry deleteClipboardEntry = Entry(deleteClipboardPath, "delete-clears-clipboard.txt", false);
    assert(operations.CopyToClipboard(&deleteClipboardEntry));
    assert(operations.HasClipboard());
    assert(operations.StartDelete(&deleteClipboardEntry));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(!operations.HasClipboard());
    assert(access(deleteClipboardPath.c_str(), F_OK) != 0);

    operations.Acknowledge();
    const std::string nestedRoot = JoinPath(source, "nested-root");
    const std::string nestedChild = JoinPath(nestedRoot, "nested-child.txt");
    assert(mkdir(nestedRoot.c_str(), 0777) == 0);
    WriteFile(nestedChild, "nested-child");
    std::vector<FileEntry> nestedSelection;
    nestedSelection.push_back(Entry(nestedChild, "nested-child.txt", false));
    nestedSelection.push_back(Entry(nestedRoot, "nested-root", true));
    assert(operations.CopyToClipboard(nestedSelection));
    assert(operations.ClipboardCount() == 1);
    assert(operations.StartPaste(destination));
    Complete(operations);
    assert(operations.State() == CopyState::Completed);
    assert(ReadFile(JoinPath(JoinPath(destination, "nested-root"), "nested-child.txt")) == "nested-child");

    printf("file-operations-host-tests-ok\n");
    return 0;
}
