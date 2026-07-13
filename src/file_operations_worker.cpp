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
#include <sys/statvfs.h>
#include <unistd.h>

#ifndef F_NOCACHE
#define F_NOCACHE 48
#endif

namespace {
const size_t kCopyBufSize = 1 * 1024 * 1024;

struct CopyPipeline {
    void *buf[2];
    size_t len[2];
    int fillIdx;
    int srcFd;
    size_t bufSz;
    bool done;
    int readerErr;
    pthread_mutex_t mtx;
    pthread_cond_t cvReady;
    pthread_cond_t cvFree;

    // Diagnostic stats
    uint64_t totalReadNs;
    uint64_t totalWaitFreeNs;
    uint64_t readCallCount;
    uint64_t totalBytesRead;
};

uint64_t GetTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

void* CopyReaderThreadFunc(void *arg) {
    CopyPipeline *p = static_cast<CopyPipeline*>(arg);

    pthread_mutex_lock(&p->mtx);
    for (;;) {
        int fi = p->fillIdx;

        uint64_t waitStart = GetTimeNs();
        // Wait until the fill buffer is free (len[fi] == 0) and done is false
        while (p->len[fi] != 0 && !p->done) {
            pthread_cond_wait(&p->cvFree, &p->mtx);
        }
        p->totalWaitFreeNs += (GetTimeNs() - waitStart);

        if (p->done) {
            break; // Stopped by main thread
        }

        pthread_mutex_unlock(&p->mtx);

        // Read without holding the lock - this is the slow USB read
        ssize_t n;
        uint64_t readStart = GetTimeNs();
        do {
            n = read(p->srcFd, p->buf[fi], p->bufSz);
        } while (n < 0 && errno == EINTR);
        uint64_t readDuration = GetTimeNs() - readStart;

        pthread_mutex_lock(&p->mtx);
        p->totalReadNs += readDuration;
        p->readCallCount++;

        if (n < 0) {
            p->readerErr = errno;
            p->done = true;
            pthread_cond_signal(&p->cvReady);
            break;
        }
        if (n == 0) {
            // EOF
            p->done = true;
            pthread_cond_signal(&p->cvReady);
            break;
        }

        p->totalBytesRead += static_cast<uint64_t>(n);
        p->len[fi] = static_cast<size_t>(n);
        p->fillIdx = 1 - fi; // Swap to the other buffer
        pthread_cond_signal(&p->cvReady);
    }
    pthread_mutex_unlock(&p->mtx);
    return nullptr;
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

bool CloseDescriptor(int *fd) {
    if (fd == NULL || *fd < 0) return true;
    int result;
    do {
        result = close(*fd);
    } while (result != 0 && errno == EINTR);
    *fd = -1;
    return result == 0;
}
}

FileOperationsWorker::FileOperationsWorker(
    OperationKind kind, ClipboardAction action,
    const FileEntry &clipboard, const std::vector<FileEntry> &clipboardEntries,
    const std::string &destinationDirectory, ExistingTargetPolicy policy,
    bool replacingTopLevelFile)
    : threadRunning_(false), threadJoined_(false), threadFinished_(false),
      cancelRequested_(false),
      operationKind_(kind), clipboardAction_(action), clipboard_(clipboard),
      clipboardEntries_(clipboardEntries), destinationDirectory_(destinationDirectory),
      policy_(policy), replacingTopLevelFile_(replacingTopLevelFile),
      state_(CopyState::Idle), phase_(OperationTaskPhase::Idle),
      copiedBytes_(0), totalBytes_(0), startedAt_(0),
      currentJobIndex_(0), currentDeleteIndex_(0) {
}

FileOperationsWorker::FileOperationsWorker(OperationKind kind, const std::vector<FileEntry> &roots)
    : threadRunning_(false), threadJoined_(false), threadFinished_(false),
      cancelRequested_(false),
      operationKind_(kind), clipboardAction_(ClipboardAction::Copy),
      clipboardEntries_(roots), policy_(ExistingTargetPolicy::Reject),
      replacingTopLevelFile_(false), state_(CopyState::Idle), phase_(OperationTaskPhase::Idle),
      copiedBytes_(0), totalBytes_(0), startedAt_(0),
      currentJobIndex_(0), currentDeleteIndex_(0) {
}

FileOperationsWorker::~FileOperationsWorker() {
    Cancel();
    Join();
    ClosePlanDirectories();
}

void FileOperationsWorker::Start() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    state_ = CopyState::Copying;
    phase_ = OperationTaskPhase::Planning;
    startedAt_ = time(NULL);
    error_.clear();
    cancelRequested_ = false;
    threadFinished_ = false;

    if (operationKind_ == OperationKind::Delete) {
        for (size_t index = clipboardEntries_.size(); index > 0; --index) {
            DeletePlanFrame frame;
            frame.path = clipboardEntries_[index - 1].path;
            frame.dir = NULL;
            frame.opened = false;
            deletePlanStack_.push_back(frame);
        }
    } else {
        for (size_t index = clipboardEntries_.size(); index > 0; --index) {
            const FileEntry &entry = clipboardEntries_[index - 1];
            CopyPlanFrame frame;
            frame.sourcePath = entry.path;
            frame.targetPath = JoinPath(destinationDirectory_, entry.name);
            frame.dir = NULL;
            frame.opened = false;
            copyPlanStack_.push_back(frame);
        }
    }

    threadRunning_ = (pthread_create(&thread_, NULL, ThreadFunc, this) == 0);
    if (!threadRunning_) {
        state_ = CopyState::Failed;
        phase_ = OperationTaskPhase::Idle;
        error_ = "Failed to create worker thread";
    }
}

void FileOperationsWorker::Cancel() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    cancelRequested_ = true;
    if (state_ == CopyState::Copying) {
        state_ = CopyState::Canceled;
        phase_ = OperationTaskPhase::Idle;
    }
}

void FileOperationsWorker::Join() {
    bool run = false;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (threadRunning_ && !threadJoined_) {
            run = true;
        }
    }
    if (run) {
        pthread_join(thread_, NULL);
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        threadJoined_ = true;
        threadRunning_ = false;
    }
}

CopyState FileOperationsWorker::State() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_;
}

OperationTaskPhase FileOperationsWorker::Phase() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return phase_;
}

int FileOperationsWorker::ProgressPercent() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (state_ == CopyState::Completed) return 100;
    if (totalBytes_ == 0) return 0;
    if (copiedBytes_ >= totalBytes_) return 100;
    const long double ratio = (static_cast<long double>(copiedBytes_) * 100.0L) /
                              static_cast<long double>(totalBytes_);
    if (ratio <= 0.0L) return 0;
    if (ratio >= 100.0L) return 100;
    return static_cast<int>(ratio);
}

uint64_t FileOperationsWorker::CopiedBytes() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return copiedBytes_;
}

uint64_t FileOperationsWorker::TotalBytes() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return totalBytes_;
}

uint64_t FileOperationsWorker::BytesPerSecond() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const time_t elapsed = time(NULL) - startedAt_;
    return elapsed <= 0 ? 0 : copiedBytes_ / static_cast<uint64_t>(elapsed);
}

std::string FileOperationsWorker::CurrentName() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return currentName_;
}

std::string FileOperationsWorker::Error() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return error_;
}

bool FileOperationsWorker::IsBusy() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_ == CopyState::Copying;
}

bool FileOperationsWorker::HasResult() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_ == CopyState::Completed || state_ == CopyState::Failed || state_ == CopyState::Canceled;
}

bool FileOperationsWorker::ThreadFinished() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return threadFinished_;
}

void* FileOperationsWorker::ThreadFunc(void *arg) {
    FileOperationsWorker *worker = static_cast<FileOperationsWorker*>(arg);
    worker->RunBackground();
    return NULL;
}

void FileOperationsWorker::RunBackground() {
    // Phase 1: Planning
    ProcessPlanning();

    bool planFailed = false;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (cancelRequested_) {
            Rollback();
            state_ = CopyState::Canceled;
            phase_ = OperationTaskPhase::Idle;
            threadFinished_ = true;
            return;
        }
        if (state_ == CopyState::Failed) {
            planFailed = true;
        }
    }

    if (planFailed) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        threadFinished_ = true;
        return;
    }

    // Phase 2: Execution
    if (operationKind_ == OperationKind::Delete) {
        RunDeletion();
    } else {
        RunCopying();
    }

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        threadFinished_ = true;
    }
}

void FileOperationsWorker::ProcessPlanning() {
    if (operationKind_ == OperationKind::Delete) {
        ProcessDeletePlanning();
    } else {
        ProcessCopyPlanning();
    }
}

void FileOperationsWorker::ProcessCopyPlanning() {
    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (cancelRequested_) return;
            if (state_ == CopyState::Failed) return;

            if (copyPlanStack_.empty()) {
                struct statvfs vfs;
                if (statvfs(destinationDirectory_.c_str(), &vfs) == 0) {
                    uint64_t blockSize = vfs.f_frsize;
                    if (blockSize == 0) blockSize = vfs.f_bsize;
                    uint64_t freeBytes = static_cast<uint64_t>(vfs.f_bavail) * blockSize;
                    if (freeBytes > 0 && totalBytes_ > freeBytes) {
                        LOG_ERROR("pre-flight ENOSPC: need=%llu free=%llu dst=%s",
                                  (unsigned long long)totalBytes_,
                                  (unsigned long long)freeBytes,
                                  destinationDirectory_.c_str());
                        Fail("No space left on device");
                        return;
                    } else if (freeBytes == 0 && totalBytes_ > 0) {
                        LOG_INFO("pre-flight space check warning: reported free space is 0, but continuing copy anyway (VFS blocks=%llu, size=%llu, total=%llu)",
                                 (unsigned long long)vfs.f_bavail, (unsigned long long)blockSize, (unsigned long long)vfs.f_blocks);
                    }
                }
                phase_ = OperationTaskPhase::Running;
                if (jobs_.empty()) CompleteOperation();
                return;
            }
        }

        CopyPlanFrame &frame = copyPlanStack_.back();
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            currentName_ = frame.sourcePath;
        }

        if (!frame.opened) {
            FsMetadata details;
            if (!LstatPath(frame.sourcePath, &details)) {
                Fail("Unable to inspect source item");
                return;
            }
            if (details.kind == FileKind::Directory) {
                FsMetadata existing;
                if (PathExists(frame.targetPath, &existing)) {
                    Fail("Destination already exists");
                    return;
                }
                if (mkdir(frame.targetPath.c_str(), 0777) != 0) {
                    LOG_ERROR("mkdir failed for %s: %s (errno: %d)", frame.targetPath.c_str(), strerror(errno), errno);
                    Fail("Unable to create target directory");
                    return;
                }
                {
                    std::lock_guard<std::recursive_mutex> lock(mutex_);
                    createdDirectories_.push_back(frame.targetPath);
                }
                frame.dir = opendir(frame.sourcePath.c_str());
                if (frame.dir == NULL) {
                    LOG_ERROR("opendir failed for %s: %s (errno: %d)", frame.sourcePath.c_str(), strerror(errno), errno);
                    Fail("Unable to open source directory");
                    return;
                }
                frame.opened = true;
                continue;
            }
            if (details.kind == FileKind::Regular) {
                if (!AddFileJob(frame.sourcePath, frame.targetPath, details.size)) return;
                copyPlanStack_.pop_back();
                continue;
            }
            Fail("Special file copy is not supported");
            return;
        }

        struct dirent *entry = NULL;
        while ((entry = readdir(frame.dir)) != NULL) {
            const std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            CopyPlanFrame child;
            child.sourcePath = JoinPath(frame.sourcePath, name);
            child.targetPath = JoinPath(frame.targetPath, name);
            child.dir = NULL;
            child.opened = false;
            copyPlanStack_.push_back(child);
            break;
        }
        if (entry == NULL) {
            closedir(frame.dir);
            frame.dir = NULL;
            copyPlanStack_.pop_back();
        }
    }
}

void FileOperationsWorker::ProcessDeletePlanning() {
    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (cancelRequested_) return;
            if (state_ == CopyState::Failed) return;

            if (deletePlanStack_.empty()) {
                totalBytes_ = deletePaths_.size();
                phase_ = OperationTaskPhase::Running;
                if (deletePaths_.empty()) {
                    state_ = CopyState::Completed;
                    phase_ = OperationTaskPhase::Idle;
                }
                return;
            }
        }

        DeletePlanFrame &frame = deletePlanStack_.back();
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            currentName_ = frame.path;
        }

        if (!frame.opened) {
            FsMetadata details;
            if (!LstatPath(frame.path, &details)) {
                Fail("Unable to inspect item for deletion");
                return;
            }
            if (details.kind == FileKind::Directory) {
                frame.dir = opendir(frame.path.c_str());
                if (frame.dir == NULL) {
                    LOG_ERROR("opendir failed for delete %s: %s (errno: %d)", frame.path.c_str(), strerror(errno), errno);
                    Fail("Unable to open directory for deletion");
                    return;
                }
                frame.opened = true;
                continue;
            }
            deletePaths_.push_back(frame.path);
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                totalBytes_ = deletePaths_.size();
            }
            deletePlanStack_.pop_back();
            continue;
        }

        struct dirent *entry = NULL;
        while ((entry = readdir(frame.dir)) != NULL) {
            const std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            DeletePlanFrame child;
            child.path = JoinPath(frame.path, name);
            child.dir = NULL;
            child.opened = false;
            deletePlanStack_.push_back(child);
            break;
        }
        if (entry == NULL) {
            closedir(frame.dir);
            frame.dir = NULL;
            deletePaths_.push_back(frame.path);
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                totalBytes_ = deletePaths_.size();
            }
            deletePlanStack_.pop_back();
        }
    }
}

bool FileOperationsWorker::AddFileJob(const std::string &sourcePath, const std::string &targetPath, uint64_t size) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    FsMetadata existing;
    const bool targetExists = PathExists(targetPath, &existing);
    const bool replacingTarget = replacingTopLevelFile_ &&
                                 sourcePath == clipboard_.path &&
                                 existing.kind == FileKind::Regular;
    if ((targetExists && !replacingTarget) ||
        PathExists(targetPath + ".kylin-copy.tmp", NULL)) {
        Fail("Destination already exists");
        return false;
    }
    CopyJob job;
    job.sourcePath = sourcePath;
    job.targetPath = targetPath;
    job.size = size;
    jobs_.push_back(job);
    totalBytes_ += size;
    return true;
}

void FileOperationsWorker::RunCopying() {
    while (true) {
        CopyJob job;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (cancelRequested_) {
                Rollback();
                state_ = CopyState::Canceled;
                phase_ = OperationTaskPhase::Idle;
                return;
            }
            if (currentJobIndex_ >= jobs_.size()) {
                CompleteOperation();
                return;
            }
            job = jobs_[currentJobIndex_];
            currentName_ = job.sourcePath;
        }

        if (!ExecuteCopyJob(job)) {
            return;
        }
    }
}

bool FileOperationsWorker::ExecuteCopyJob(const CopyJob &job) {
    std::string tempPath = job.targetPath + ".kylin-copy.tmp";
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        targetPath_ = job.targetPath;
        temporaryPath_ = tempPath;
    }

    int sourceFd = open(job.sourcePath.c_str(), O_RDONLY);
    if (sourceFd < 0) {
        LOG_ERROR("open source failed for %s: %s (errno: %d)", job.sourcePath.c_str(), strerror(errno), errno);
        Fail("Unable to open copy source");
        return false;
    }
#ifdef F_NOCACHE
    (void)fcntl(sourceFd, F_NOCACHE, 1);
#endif
    // Hint sequential read-ahead
#ifdef F_RDAHEAD
    (void)fcntl(sourceFd, F_RDAHEAD, 1);
#endif
#if defined(POSIX_FADV_SEQUENTIAL)
    (void)posix_fadvise(sourceFd, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

    struct stat sourceStat;
    if (fstat(sourceFd, &sourceStat) != 0 || !S_ISREG(sourceStat.st_mode)) {
        CloseDescriptor(&sourceFd);
        Fail("Copy source is not a regular file");
        return false;
    }

    uint64_t actualSize = static_cast<uint64_t>(sourceStat.st_size);
#ifdef __ORBIS__
    if (sourceStat.st_blocks == 65536) {
        const unsigned char *raw = reinterpret_cast<const unsigned char *>(&sourceStat);
        uint64_t sizeAt72;
        memcpy(&sizeAt72, raw + 72, sizeof(sizeAt72));
        if (sizeAt72 != actualSize) actualSize = sizeAt72;
    }
#endif

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (actualSize != job.size) {
            totalBytes_ = totalBytes_ >= job.size ? totalBytes_ - job.size : 0;
            totalBytes_ += actualSize;
        }
    }

    // Open target as standard cached file descriptor (no O_DIRECT, no F_NOCACHE)
    // This allows the OS to coalesce writes to the internal SSD.
    int targetFd = open(tempPath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (targetFd < 0) {
        CloseDescriptor(&sourceFd);
        LOG_ERROR("open target failed for %s: %s (errno: %d)", tempPath.c_str(), strerror(errno), errno);
        Fail("Unable to open copy target");
        return false;
    }

    // Allocate page-aligned double buffers
    void *rawBuf0 = nullptr;
    void *rawBuf1 = nullptr;
    if (posix_memalign(&rawBuf0, 4096, kCopyBufSize) != 0) {
        CloseDescriptor(&sourceFd);
        CloseDescriptor(&targetFd);
        unlink(tempPath.c_str());
        Fail("Out of memory for copy buffer 0");
        return false;
    }
    if (posix_memalign(&rawBuf1, 4096, kCopyBufSize) != 0) {
        free(rawBuf0);
        CloseDescriptor(&sourceFd);
        CloseDescriptor(&targetFd);
        unlink(tempPath.c_str());
        Fail("Out of memory for copy buffer 1");
        return false;
    }

    CopyPipeline pipe;
    pipe.buf[0] = rawBuf0;
    pipe.buf[1] = rawBuf1;
    pipe.len[0] = 0;
    pipe.len[1] = 0;
    pipe.fillIdx = 0;
    pipe.srcFd = sourceFd;
    pipe.bufSz = kCopyBufSize;
    pipe.done = false;
    pipe.readerErr = 0;
    pipe.totalReadNs = 0;
    pipe.totalWaitFreeNs = 0;
    pipe.readCallCount = 0;
    pipe.totalBytesRead = 0;
    pthread_mutex_init(&pipe.mtx, nullptr);
    pthread_cond_init(&pipe.cvReady, nullptr);
    pthread_cond_init(&pipe.cvFree, nullptr);

    pthread_t readerThread;
    bool threadOk = (pthread_create(&readerThread, nullptr, CopyReaderThreadFunc, &pipe) == 0);
    if (!threadOk) {
        pthread_mutex_destroy(&pipe.mtx);
        pthread_cond_destroy(&pipe.cvReady);
        pthread_cond_destroy(&pipe.cvFree);
        free(rawBuf0);
        free(rawBuf1);
        CloseDescriptor(&sourceFd);
        CloseDescriptor(&targetFd);
        unlink(tempPath.c_str());
        Fail("Failed to create copy reader thread");
        return false;
    }

    bool hasError = false;
    std::string errorMessage;
    int writeErrno = 0;
    int drainIdx = 0;

    uint64_t totalWriteNs = 0;
    uint64_t totalWaitReadyNs = 0;
    uint64_t writeCallCount = 0;
    uint64_t totalBytesWritten = 0;
    uint64_t cycleCount = 0;

    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (cancelRequested_) {
                pthread_mutex_lock(&pipe.mtx);
                pipe.done = true;
                pthread_cond_signal(&pipe.cvFree);
                pthread_mutex_unlock(&pipe.mtx);
                break;
            }
        }

        uint64_t waitStart = GetTimeNs();
        pthread_mutex_lock(&pipe.mtx);
        while (pipe.len[drainIdx] == 0 && !pipe.done) {
            pthread_cond_wait(&pipe.cvReady, &pipe.mtx);
        }
        totalWaitReadyNs += (GetTimeNs() - waitStart);

        size_t nbytes = pipe.len[drainIdx];

        if (nbytes == 0 && pipe.done) {
            pthread_mutex_unlock(&pipe.mtx);
            break;
        }

        pthread_mutex_unlock(&pipe.mtx);

        size_t written = 0;
        bool writeFailed = false;
        uint64_t writeStart = GetTimeNs();
        while (written < nbytes) {
            ssize_t result = write(targetFd, static_cast<char*>(pipe.buf[drainIdx]) + written, nbytes - written);
            writeCallCount++;
            if (result < 0 && errno == EINTR) continue;
            if (result == 0) {
                writeErrno = ENOSPC;
                writeFailed = true;
                break;
            }
            if (result < 0) {
                writeErrno = errno;
                writeFailed = true;
                break;
            }
            written += static_cast<size_t>(result);
            totalBytesWritten += static_cast<uint64_t>(result);
        }
        totalWriteNs += (GetTimeNs() - writeStart);

        if (writeFailed) {
            hasError = true;
            errorMessage = strerror(writeErrno);
            LOG_ERROR("Write failed for target: %s (errno: %d)", errorMessage.c_str(), writeErrno);

            pthread_mutex_lock(&pipe.mtx);
            pipe.done = true;
            pthread_cond_signal(&pipe.cvFree);
            pthread_mutex_unlock(&pipe.mtx);
            break;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            copiedBytes_ += static_cast<uint64_t>(written);
            if (copiedBytes_ > totalBytes_) totalBytes_ = copiedBytes_;
        }

        pthread_mutex_lock(&pipe.mtx);
        pipe.len[drainIdx] = 0;
        drainIdx = 1 - drainIdx;
        pthread_cond_signal(&pipe.cvFree);
        pthread_mutex_unlock(&pipe.mtx);

        cycleCount++;
        if (cycleCount > 0 && cycleCount % 10 == 0) {
            uint64_t rNs, wfNs, rCalls, rBytes;
            pthread_mutex_lock(&pipe.mtx);
            rNs = pipe.totalReadNs;
            wfNs = pipe.totalWaitFreeNs;
            rCalls = pipe.readCallCount;
            rBytes = pipe.totalBytesRead;
            pthread_mutex_unlock(&pipe.mtx);

            LOG_INFO("[STATS] Cycles=%llu, "
                     "Read: time=%llu ms, calls=%llu, bytes=%llu (avg=%llu KB); "
                     "Write: time=%llu ms, calls=%llu, bytes=%llu (avg=%llu KB); "
                     "Wait: Ready=%llu ms, Free=%llu ms",
                     (unsigned long long)cycleCount,
                     (unsigned long long)(rNs / 1000000), (unsigned long long)rCalls, (unsigned long long)rBytes, (unsigned long long)(rBytes / (rCalls ? rCalls : 1) / 1024),
                     (unsigned long long)(totalWriteNs / 1000000), (unsigned long long)writeCallCount, (unsigned long long)totalBytesWritten, (unsigned long long)(totalBytesWritten / (writeCallCount ? writeCallCount : 1) / 1024),
                     (unsigned long long)(totalWaitReadyNs / 1000000), (unsigned long long)(wfNs / 1000000));
        }
    }

    pthread_join(readerThread, nullptr);

    // Print final diagnostic stats
    {
        uint64_t rNs, wfNs, rCalls, rBytes;
        pthread_mutex_lock(&pipe.mtx);
        rNs = pipe.totalReadNs;
        wfNs = pipe.totalWaitFreeNs;
        rCalls = pipe.readCallCount;
        rBytes = pipe.totalBytesRead;
        pthread_mutex_unlock(&pipe.mtx);

        LOG_INFO("[STATS FINAL] Cycles=%llu, "
                 "Read: total_time=%llu ms, calls=%llu, bytes=%llu (avg=%llu KB); "
                 "Write: total_time=%llu ms, calls=%llu, bytes=%llu (avg=%llu KB); "
                 "Wait: Ready=%llu ms, Free=%llu ms",
                 (unsigned long long)cycleCount,
                 (unsigned long long)(rNs / 1000000), (unsigned long long)rCalls, (unsigned long long)rBytes, (unsigned long long)(rBytes / (rCalls ? rCalls : 1) / 1024),
                 (unsigned long long)(totalWriteNs / 1000000), (unsigned long long)writeCallCount, (unsigned long long)totalBytesWritten, (unsigned long long)(totalBytesWritten / (writeCallCount ? writeCallCount : 1) / 1024),
                 (unsigned long long)(totalWaitReadyNs / 1000000), (unsigned long long)(wfNs / 1000000));
    }

    if (!hasError && pipe.readerErr != 0) {
        hasError = true;
        errorMessage = strerror(pipe.readerErr);
        LOG_ERROR("Reader thread failed: %s (errno: %d)", errorMessage.c_str(), pipe.readerErr);
    }

    pthread_mutex_destroy(&pipe.mtx);
    pthread_cond_destroy(&pipe.cvReady);
    pthread_cond_destroy(&pipe.cvFree);

    free(rawBuf0);
    free(rawBuf1);

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (cancelRequested_) {
            CloseDescriptor(&sourceFd);
            CloseDescriptor(&targetFd);
            unlink(tempPath.c_str());
            Rollback();
            state_ = CopyState::Canceled;
            phase_ = OperationTaskPhase::Idle;
            return false;
        }
    }

    if (hasError) {
        CloseDescriptor(&sourceFd);
        CloseDescriptor(&targetFd);
        unlink(tempPath.c_str());
        Fail("Unable to copy file: " + errorMessage);
        return false;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        phase_ = OperationTaskPhase::Finalizing;
    }

#if defined(POSIX_FADV_DONTNEED)
    (void)posix_fadvise(sourceFd, 0, 0, POSIX_FADV_DONTNEED);
    (void)posix_fadvise(targetFd, 0, 0, POSIX_FADV_DONTNEED);
#endif

    CloseDescriptor(&targetFd);
    CloseDescriptor(&sourceFd);

    if (rename(tempPath.c_str(), job.targetPath.c_str()) != 0) {
        unlink(tempPath.c_str());
        LOG_ERROR("Finalize rename failed from %s to %s: %s (errno: %d)",
                  tempPath.c_str(), job.targetPath.c_str(), strerror(errno), errno);
        Fail("Unable to finalize copied file");
        return false;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        createdFiles_.push_back(job.targetPath);
        temporaryPath_.clear();
        ++currentJobIndex_;
        phase_ = OperationTaskPhase::Running;
    }

    return true;
}

void FileOperationsWorker::RunDeletion() {
    while (true) {
        std::string path;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (cancelRequested_) {
                state_ = CopyState::Canceled;
                phase_ = OperationTaskPhase::Idle;
                return;
            }
            if (currentDeleteIndex_ >= deletePaths_.size()) {
                state_ = CopyState::Completed;
                phase_ = OperationTaskPhase::Idle;
                return;
            }
            path = deletePaths_[currentDeleteIndex_];
            currentName_ = path;
        }

        FsMetadata details;
        if (!LstatPath(path, &details) ||
            (details.kind == FileKind::Directory ? rmdir(path.c_str()) : unlink(path.c_str())) != 0) {
            LOG_ERROR("Delete failed for %s: %s (errno: %d)", path.c_str(), strerror(errno), errno);
            Fail("Unable to delete item");
            return;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            ++currentDeleteIndex_;
            ++copiedBytes_;
            if (copiedBytes_ > totalBytes_) totalBytes_ = copiedBytes_;
        }
    }
}

void FileOperationsWorker::CompleteOperation() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    ClosePlanDirectories();
    if (clipboardAction_ == ClipboardAction::Move) {
        for (size_t index = 0; index < clipboardEntries_.size(); ++index) {
            if (!RemovePathRecursive(clipboardEntries_[index].path)) {
                error_ = "Copied item but unable to remove source";
                state_ = CopyState::Failed;
                phase_ = OperationTaskPhase::Idle;
                return;
            }
        }
    }
    if (copiedBytes_ > totalBytes_) totalBytes_ = copiedBytes_;
    state_ = CopyState::Completed;
    phase_ = OperationTaskPhase::Idle;
}

void FileOperationsWorker::ClosePlanDirectories() {
    for (size_t index = 0; index < copyPlanStack_.size(); ++index) {
        if (copyPlanStack_[index].dir != NULL) {
            closedir(copyPlanStack_[index].dir);
            copyPlanStack_[index].dir = NULL;
        }
    }
    for (size_t index = 0; index < deletePlanStack_.size(); ++index) {
        if (deletePlanStack_[index].dir != NULL) {
            closedir(deletePlanStack_[index].dir);
            deletePlanStack_[index].dir = NULL;
        }
    }
}

void FileOperationsWorker::SyncParentDirectory(const std::string &path) {
    const std::string parent = ParentPath(path);
    int fd = open(parent.c_str(), O_RDONLY);
    if (fd < 0) return;
    if (fsync(fd) != 0) {
        LOG_INFO("Parent directory fsync skipped/failed for %s: %s (errno: %d)",
                 parent.c_str(), strerror(errno), errno);
    }
    CloseDescriptor(&fd);
}

void FileOperationsWorker::Rollback() {
    ClosePlanDirectories();
    if (!temporaryPath_.empty()) unlink(temporaryPath_.c_str());
    for (std::vector<std::string>::reverse_iterator item = createdFiles_.rbegin();
         item != createdFiles_.rend(); ++item) {
        unlink(item->c_str());
    }
    for (std::vector<std::string>::reverse_iterator item = createdDirectories_.rbegin();
         item != createdDirectories_.rend(); ++item) {
        rmdir(item->c_str());
    }
    createdFiles_.clear();
    createdDirectories_.clear();
}

void FileOperationsWorker::Fail(const std::string &message) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Rollback();
    error_ = message;
    state_ = CopyState::Failed;
    phase_ = OperationTaskPhase::Idle;
}

bool FileOperationsWorker::RemovePathRecursive(const std::string &path) {
    FsMetadata details;
    if (!LstatPath(path, &details)) return true;

    if (details.kind == FileKind::Directory) {
        DIR *dir = opendir(path.c_str());
        if (dir != NULL) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                const std::string name = entry->d_name;
                if (name == "." || name == "..") continue;
                if (!RemovePathRecursive(JoinPath(path, name))) {
                    closedir(dir);
                    return false;
                }
            }
            closedir(dir);
        }
        return rmdir(path.c_str()) == 0;
    }
    return unlink(path.c_str()) == 0;
}

std::string FileOperationsWorker::JoinPath(const std::string &directory, const std::string &name) {
    return ::JoinPath(directory, name);
}
