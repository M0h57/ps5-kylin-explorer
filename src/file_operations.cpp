#include "file_operations.h"
#include "file_operations_worker.h"
#include "file_system.h"
#include "logger.h"
#include "utils.h"

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
int RenameForMove(const std::string &source, const std::string &target) {
#ifdef KYLIN_TEST_FORCE_COPY_MOVE
    (void)source;
    (void)target;
    errno = EXDEV;
    return -1;
#else
    return rename(source.c_str(), target.c_str());
#endif
}

bool LstatPath(const std::string &path, FsMetadata *metadata) {
    PosixFileSystem fileSystem;
    std::string error;
    if (!fileSystem.Lstat(FsPath(path), metadata, &error)) {
        LOG_INFO("LstatPath failed for '%s': %s", path.c_str(), error.c_str());
        return false;
    }
    return true;
}

bool PathExists(const std::string &path, FsMetadata *metadata) {
    FsMetadata details;
    if (!LstatPath(path, &details)) return false;
    if (metadata != NULL) *metadata = details;
    return true;
}
}

FileOperations::FileOperations()
    : clipboardAction_(ClipboardAction::Copy), operationKind_(OperationKind::Copy),
      hasClipboard_(false), replacingTopLevelFile_(false), worker_(NULL),
      localState_(CopyState::Idle), localPhase_(OperationTaskPhase::Idle),
      localCopiedBytes_(0), localTotalBytes_(0) {
}

FileOperations::~FileOperations() {
    if (worker_ != NULL) {
        delete worker_;
        worker_ = NULL;
    }
}

bool FileOperations::CopyToClipboard(const FileEntry *entry) {
    if (entry == NULL || IsBusy()) return false;
    std::vector<FileEntry> entries;
    entries.push_back(*entry);
    return CopyToClipboard(entries);
}

bool FileOperations::CopyToClipboard(const std::vector<FileEntry> &entries) {
    if (entries.empty() || IsBusy()) return false;
    clipboardEntries_ = NormalizeEntryRoots(entries);
    if (clipboardEntries_.empty()) return false;
    clipboard_ = clipboardEntries_[0];
    clipboardAction_ = ClipboardAction::Copy;
    hasClipboard_ = true;
    return true;
}

bool FileOperations::CutToClipboard(const FileEntry *entry) {
    if (entry == NULL || IsBusy()) return false;
    std::vector<FileEntry> entries;
    entries.push_back(*entry);
    return CutToClipboard(entries);
}

bool FileOperations::CutToClipboard(const std::vector<FileEntry> &entries) {
    if (entries.empty() || IsBusy()) return false;
    clipboardEntries_ = NormalizeEntryRoots(entries);
    if (clipboardEntries_.empty()) return false;
    clipboard_ = clipboardEntries_[0];
    clipboardAction_ = ClipboardAction::Move;
    hasClipboard_ = true;
    return true;
}

bool FileOperations::StartPaste(const std::string &destinationDirectory, ExistingTargetPolicy policy) {
    if (!hasClipboard_ || IsBusy()) return false;

    operationKind_ = clipboardAction_ == ClipboardAction::Move ? OperationKind::Move : OperationKind::Copy;
    const bool singleClipboardItem = clipboardEntries_.size() == 1;

    std::vector<FsMetadata> sourceMetadata;
    sourceMetadata.reserve(clipboardEntries_.size());
    for (size_t index = 0; index < clipboardEntries_.size(); ++index) {
        const FileEntry &entry = clipboardEntries_[index];
        if (!FsName::IsValidBytes(entry.name)) {
            localError_ = "Invalid source name";
            localState_ = CopyState::Failed;
            return false;
        }

        FsMetadata sourceDetails;
        if (!LstatPath(entry.path, &sourceDetails)) {
            localError_ = "Unable to inspect source item";
            localState_ = CopyState::Failed;
            return false;
        }
        sourceMetadata.push_back(sourceDetails);

        const std::string destinationPath = JoinPath(destinationDirectory, entry.name);
        if (sourceDetails.kind == FileKind::Directory && IsPathWithin(destinationPath, entry.path)) {
            localError_ = "Cannot paste a directory into itself";
            localState_ = CopyState::Failed;
            return false;
        }

        FsMetadata existing;
        if (PathExists(destinationPath, &existing)) {
            if (policy != ExistingTargetPolicy::Replace || !singleClipboardItem) {
                localError_ = "Destination already exists";
                localState_ = CopyState::Failed;
                return false;
            }
            if (sourceDetails.kind != FileKind::Regular || existing.kind != FileKind::Regular) {
                localError_ = "Only regular file overwrite is supported";
                localState_ = CopyState::Failed;
                return false;
            }
        }
        if (clipboardAction_ == ClipboardAction::Move && destinationPath == entry.path) {
            localError_ = "Source and destination are the same";
            localState_ = CopyState::Failed;
            return false;
        }
    }

    if (worker_ != NULL) {
        delete worker_;
        worker_ = NULL;
    }

    localCopiedBytes_ = 0;
    localTotalBytes_ = 0;
    localError_.clear();
    localCurrentName_.clear();
    targetPath_ = JoinPath(destinationDirectory, clipboard_.name);
    replacingTopLevelFile_ = policy == ExistingTargetPolicy::Replace && singleClipboardItem;

    // Fast-path: Move single file/folder on same filesystem using rename
    if (clipboardAction_ == ClipboardAction::Move && singleClipboardItem) {
        if (RenameForMove(clipboard_.path, targetPath_) == 0) {
            localCopiedBytes_ = sourceMetadata[0].kind == FileKind::Directory ? 0 : sourceMetadata[0].size;
            localTotalBytes_ = localCopiedBytes_;
            hasClipboard_ = false;
            clipboardEntries_.clear();
            localState_ = CopyState::Completed;
            localPhase_ = OperationTaskPhase::Idle;
            return true;
        }
        if (errno != EXDEV) {
            localError_ = "Unable to move item";
            localState_ = CopyState::Failed;
            localPhase_ = OperationTaskPhase::Idle;
            return false;
        }
    }

    // Fallback: spawn background worker thread for planning and copy/delete execution
    worker_ = new FileOperationsWorker(
        operationKind_, clipboardAction_, clipboard_, clipboardEntries_,
        destinationDirectory, policy, replacingTopLevelFile_
    );
    worker_->Start();

    if (worker_->State() == CopyState::Failed) {
        localError_ = worker_->Error();
        localState_ = CopyState::Failed;
        delete worker_;
        worker_ = NULL;
        return false;
    }

    localState_ = CopyState::Copying;
    localPhase_ = OperationTaskPhase::Planning;
    return true;
}

bool FileOperations::StartDelete(const FileEntry *entry) {
    if (entry == NULL || IsBusy()) return false;
    std::vector<FileEntry> entries;
    entries.push_back(*entry);
    return StartDelete(entries);
}

bool FileOperations::StartDelete(const std::vector<FileEntry> &entries) {
    if (entries.empty() || IsBusy()) return false;

    const std::vector<FileEntry> roots = NormalizeEntryRoots(entries);
    if (roots.empty()) return false;

    if (worker_ != NULL) {
        delete worker_;
        worker_ = NULL;
    }

    localCopiedBytes_ = 0;
    localTotalBytes_ = 0;
    localError_.clear();
    localCurrentName_.clear();
    operationKind_ = OperationKind::Delete;

    if (hasClipboard_) {
        for (size_t root = 0; root < roots.size(); ++root) {
            for (size_t clip = 0; clip < clipboardEntries_.size(); ++clip) {
                if (IsPathWithin(clipboardEntries_[clip].path, roots[root].path)) {
                    hasClipboard_ = false;
                    clipboardEntries_.clear();
                    break;
                }
            }
            if (!hasClipboard_) break;
        }
    }

    worker_ = new FileOperationsWorker(operationKind_, roots);
    worker_->Start();

    if (worker_->State() == CopyState::Failed) {
        localError_ = worker_->Error();
        localState_ = CopyState::Failed;
        delete worker_;
        worker_ = NULL;
        return false;
    }

    localState_ = CopyState::Copying;
    localPhase_ = OperationTaskPhase::Planning;
    return true;
}

void FileOperations::Update() {
    if (worker_ == NULL) return;

    if (worker_->ThreadFinished()) {
        worker_->Join();
    }
}

void FileOperations::Cancel() {
    if (!IsBusy() || !CanCancel()) return;
    if (worker_ != NULL) {
        worker_->Cancel();
    }
}

void FileOperations::Acknowledge() {
    if (!HasResult()) return;
    if (worker_ != NULL) {
        worker_->Join();
        delete worker_;
        worker_ = NULL;
    }
    localState_ = CopyState::Idle;
    localPhase_ = OperationTaskPhase::Idle;
    localError_.clear();
}

bool FileOperations::HasClipboard() const { return hasClipboard_; }

int FileOperations::ClipboardCount() const {
    return hasClipboard_ ? static_cast<int>(clipboardEntries_.size()) : 0;
}

bool FileOperations::IsMoveOperation() const { return operationKind_ == OperationKind::Move; }
bool FileOperations::IsDeleteOperation() const { return operationKind_ == OperationKind::Delete; }
bool FileOperations::CanCancel() const { return operationKind_ != OperationKind::Delete; }

bool FileOperations::DestinationExists(const std::string &destinationDirectory) const {
    if (!hasClipboard_) return false;
    FsMetadata existing;
    for (size_t index = 0; index < clipboardEntries_.size(); ++index) {
        if (PathExists(JoinPath(destinationDirectory, clipboardEntries_[index].name), &existing)) return true;
    }
    return false;
}

bool FileOperations::CanReplaceDestination(const std::string &destinationDirectory) const {
    if (!hasClipboard_ || clipboardEntries_.size() != 1) return false;
    FsMetadata source;
    FsMetadata existing;
    return LstatPath(clipboard_.path, &source) && source.kind == FileKind::Regular &&
           PathExists(JoinPath(destinationDirectory, clipboard_.name), &existing) &&
           existing.kind == FileKind::Regular;
}

bool FileOperations::IsBusy() const {
    if (worker_ != NULL) return worker_->IsBusy();
    return localState_ == CopyState::Copying;
}

bool FileOperations::HasResult() const {
    if (worker_ != NULL) return worker_->HasResult();
    return localState_ == CopyState::Completed || localState_ == CopyState::Failed || localState_ == CopyState::Canceled;
}

CopyState FileOperations::State() const {
    if (worker_ != NULL) return worker_->State();
    return localState_;
}

OperationTaskPhase FileOperations::Phase() const {
    if (worker_ != NULL) return worker_->Phase();
    return localPhase_;
}

int FileOperations::ProgressPercent() const {
    if (worker_ != NULL) return worker_->ProgressPercent();
    if (localState_ == CopyState::Completed) return 100;
    if (localTotalBytes_ == 0) return 0;
    if (localCopiedBytes_ >= localTotalBytes_) return 100;
    const long double ratio = (static_cast<long double>(localCopiedBytes_) * 100.0L) /
                              static_cast<long double>(localTotalBytes_);
    if (ratio <= 0.0L) return 0;
    if (ratio >= 100.0L) return 100;
    return static_cast<int>(ratio);
}

uint64_t FileOperations::CopiedBytes() const {
    if (worker_ != NULL) return worker_->CopiedBytes();
    return localCopiedBytes_;
}

uint64_t FileOperations::TotalBytes() const {
    if (worker_ != NULL) return worker_->TotalBytes();
    return localTotalBytes_;
}

uint64_t FileOperations::BytesPerSecond() const {
    if (worker_ != NULL) return worker_->BytesPerSecond();
    return 0;
}

const std::string &FileOperations::CurrentName() const {
    if (worker_ != NULL) {
        localCurrentName_ = worker_->CurrentName();
        return localCurrentName_;
    }
    return localCurrentName_;
}

const std::string &FileOperations::Error() const {
    if (worker_ != NULL) {
        localError_ = worker_->Error();
        return localError_;
    }
    return localError_;
}

std::string FileOperations::JoinPath(const std::string &directory, const std::string &name) {
    return ::JoinPath(directory, name);
}

std::vector<FileEntry> FileOperations::NormalizeEntryRoots(const std::vector<FileEntry> &entries) {
    std::vector<FileEntry> sorted = entries;
    std::sort(sorted.begin(), sorted.end(), [](const FileEntry &left, const FileEntry &right) {
        if (left.path.size() != right.path.size()) return left.path.size() < right.path.size();
        return left.path < right.path;
    });

    std::vector<FileEntry> roots;
    for (size_t index = 0; index < sorted.size(); ++index) {
        bool nested = false;
        for (size_t root = 0; root < roots.size(); ++root) {
            if (IsPathWithin(sorted[index].path, roots[root].path)) {
                nested = true;
                break;
            }
        }
        if (!nested) roots.push_back(sorted[index]);
    }
    return roots;
}
