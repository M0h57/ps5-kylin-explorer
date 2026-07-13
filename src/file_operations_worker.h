#ifndef KYLIN_EXPLORER_FILE_OPERATIONS_WORKER_H
#define KYLIN_EXPLORER_FILE_OPERATIONS_WORKER_H

#include <stdint.h>
#include <dirent.h>
#include <time.h>
#include <string>
#include <vector>
#include <mutex>
#include <pthread.h>
#include "file_browser.h"
#include "file_operations.h"

class FileOperationsWorker {
public:
    // Constructor for Paste (Copy/Move)
    FileOperationsWorker(OperationKind kind, ClipboardAction action,
                         const FileEntry &clipboard, const std::vector<FileEntry> &clipboardEntries,
                         const std::string &destinationDirectory, ExistingTargetPolicy policy,
                         bool replacingTopLevelFile);
    // Constructor for Delete
    FileOperationsWorker(OperationKind kind, const std::vector<FileEntry> &roots);
    ~FileOperationsWorker();

    void Start();
    void Cancel();
    void Join();

    // Thread-safe accessors
    CopyState State() const;
    OperationTaskPhase Phase() const;
    int ProgressPercent() const;
    uint64_t CopiedBytes() const;
    uint64_t TotalBytes() const;
    uint64_t BytesPerSecond() const;
    std::string CurrentName() const;
    std::string Error() const;
    bool IsBusy() const;
    bool HasResult() const;
    bool ThreadFinished() const;

private:
    struct CopyJob {
        std::string sourcePath;
        std::string targetPath;
        uint64_t size;
    };

    struct CopyPlanFrame {
        std::string sourcePath;
        std::string targetPath;
        DIR *dir;
        bool opened;
    };

    struct DeletePlanFrame {
        std::string path;
        DIR *dir;
        bool opened;
    };

    static void* ThreadFunc(void *arg);
    void RunBackground();

    void RunCopying();
    void RunDeletion();

    void ProcessPlanning();
    void ProcessCopyPlanning();
    void ProcessDeletePlanning();
    bool AddFileJob(const std::string &sourcePath, const std::string &targetPath, uint64_t size);

    bool ExecuteCopyJob(const CopyJob &job);
    void CompleteOperation();
    void ClosePlanDirectories();
    void SyncParentDirectory(const std::string &path);
    void Rollback();
    void Fail(const std::string &message);

    static bool RemovePathRecursive(const std::string &path);
    static std::string JoinPath(const std::string &directory, const std::string &name);

    // Concurrency variables
    pthread_t thread_;
    bool threadRunning_;
    bool threadJoined_;
    bool threadFinished_;
    bool cancelRequested_;
    mutable std::recursive_mutex mutex_;

    // Operation configuration
    OperationKind operationKind_;
    ClipboardAction clipboardAction_;
    FileEntry clipboard_;
    std::vector<FileEntry> clipboardEntries_;
    std::string destinationDirectory_;
    ExistingTargetPolicy policy_;
    bool replacingTopLevelFile_;

    // Operation progress / state
    CopyState state_;
    OperationTaskPhase phase_;
    uint64_t copiedBytes_;
    uint64_t totalBytes_;
    time_t startedAt_;
    std::string error_;
    std::string currentName_;

    // Intermediate structures (only accessed by background thread)
    std::vector<CopyJob> jobs_;
    std::vector<CopyPlanFrame> copyPlanStack_;
    std::vector<DeletePlanFrame> deletePlanStack_;
    std::vector<std::string> createdFiles_;
    std::vector<std::string> createdDirectories_;
    std::vector<std::string> deletePaths_;
    size_t currentJobIndex_;
    size_t currentDeleteIndex_;
    std::string targetPath_;
    std::string temporaryPath_;
};

#endif
