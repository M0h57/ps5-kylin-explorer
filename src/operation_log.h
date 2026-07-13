#ifndef KYLIN_EXPLORER_OPERATION_LOG_H
#define KYLIN_EXPLORER_OPERATION_LOG_H

#include <string>
#include <vector>

enum class OperationLogAction {
    NewFolder,
    Rename,
    Copy,
    Move,
    Delete,
};

struct OperationLogEntry {
    OperationLogAction action;
    std::string path;
    bool succeeded;
};

class OperationLog {
public:
    void Add(OperationLogAction action, const std::string &path, bool succeeded);
    const std::vector<OperationLogEntry> &Entries() const;

private:
    std::vector<OperationLogEntry> entries_;
};

#endif
