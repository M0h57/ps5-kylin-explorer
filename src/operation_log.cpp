#include "operation_log.h"
#include "logger.h"

namespace {
const size_t kMaxLogEntries = 8;
}

void OperationLog::Add(OperationLogAction action, const std::string &path, bool succeeded) {
    OperationLogEntry entry;
    entry.action = action;
    entry.path = path;
    entry.succeeded = succeeded;
    entries_.insert(entries_.begin(), entry);
    if (entries_.size() > kMaxLogEntries) entries_.resize(kMaxLogEntries);

    const char *actionStr = "UNKNOWN";
    switch (action) {
        case OperationLogAction::NewFolder: actionStr = "NEW_FOLDER"; break;
        case OperationLogAction::Rename: actionStr = "RENAME"; break;
        case OperationLogAction::Copy: actionStr = "COPY"; break;
        case OperationLogAction::Move: actionStr = "MOVE"; break;
        case OperationLogAction::Delete: actionStr = "DELETE"; break;
    }
    LOG_INFO("Operation %s on %s - %s", actionStr, path.c_str(), succeeded ? "SUCCESS" : "FAILED");
}

const std::vector<OperationLogEntry> &OperationLog::Entries() const {
    return entries_;
}

