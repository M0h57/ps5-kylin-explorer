#ifndef KYLIN_EXPLORER_FILE_OPERATIONS_H
#define KYLIN_EXPLORER_FILE_OPERATIONS_H

#include <stdint.h>
#include <time.h>
#include <string>
#include <vector>
#include "file_browser.h"

enum class CopyState {
    Idle,
    Copying,
    Completed,
    Failed,
    Canceled,
};

enum class ExistingTargetPolicy {
    Reject,
    Replace,
};

enum class ClipboardAction {
    Copy,
    Move,
};

enum class OperationKind {
    Copy,
    Move,
    Delete,
};

enum class OperationTaskPhase {
    Idle,
    Planning,
    Running,
    Finalizing,
};

// Forward declaration of the worker class encapsulating I/O and threading details.
class FileOperationsWorker;

class FileOperations {
public:
    FileOperations();
    ~FileOperations();
    bool CopyToClipboard(const FileEntry *entry);
    bool CopyToClipboard(const std::vector<FileEntry> &entries);
    bool CutToClipboard(const FileEntry *entry);
    bool CutToClipboard(const std::vector<FileEntry> &entries);
    bool StartPaste(const std::string &destinationDirectory,
                    ExistingTargetPolicy policy = ExistingTargetPolicy::Reject);
    bool StartDelete(const FileEntry *entry);
    bool StartDelete(const std::vector<FileEntry> &entries);
    void Update();
    void Cancel();
    void Acknowledge();

    bool HasClipboard() const;
    int ClipboardCount() const;
    bool IsMoveOperation() const;
    bool IsDeleteOperation() const;
    bool CanCancel() const;
    bool DestinationExists(const std::string &destinationDirectory) const;
    bool CanReplaceDestination(const std::string &destinationDirectory) const;
    bool IsBusy() const;
    bool HasResult() const;
    CopyState State() const;
    OperationTaskPhase Phase() const;
    int ProgressPercent() const;
    uint64_t CopiedBytes() const;
    uint64_t TotalBytes() const;
    uint64_t BytesPerSecond() const;
    const std::string &CurrentName() const;
    const std::string &Error() const;

private:
    static std::string JoinPath(const std::string &directory, const std::string &name);
    static std::vector<FileEntry> NormalizeEntryRoots(const std::vector<FileEntry> &entries);

    // Clipboard state (retained in the UI/Controller class)
    FileEntry clipboard_;
    std::vector<FileEntry> clipboardEntries_;
    ClipboardAction clipboardAction_;
    OperationKind operationKind_;
    bool hasClipboard_;
    bool replacingTopLevelFile_;
    std::string targetPath_;
    std::string emptyString_;

    // Pointer to active background I/O worker
    FileOperationsWorker *worker_;

    // Local state (used for instant fast-path operations)
    CopyState localState_;
    OperationTaskPhase localPhase_;
    uint64_t localCopiedBytes_;
    uint64_t localTotalBytes_;
    mutable std::string localError_;
    mutable std::string localCurrentName_;
};

#endif
