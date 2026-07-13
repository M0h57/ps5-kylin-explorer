#include <assert.h>
#include <stdio.h>

#include "../src/operation_log.h"

int main() {
    OperationLog log;
    assert(log.Entries().empty());

    log.Add(OperationLogAction::NewFolder, "new-folder", true);
    log.Add(OperationLogAction::Rename, "rename", false);
    log.Add(OperationLogAction::Move, "move", true);
    assert(log.Entries().size() == 3);
    assert(log.Entries()[0].action == OperationLogAction::Move);
    assert(log.Entries()[1].action == OperationLogAction::Rename);
    assert(log.Entries()[2].action == OperationLogAction::NewFolder);
    assert(!log.Entries()[1].succeeded);

    for (int index = 0; index < 10; ++index) {
        log.Add(index % 2 == 0 ? OperationLogAction::Copy : OperationLogAction::Delete,
                "中文路径-" + std::to_string(index), index % 3 != 0);
    }

    assert(log.Entries().size() == 8);
    assert(log.Entries()[0].path == "中文路径-9");
    assert(log.Entries()[7].path == "中文路径-2");
    assert(log.Entries()[0].action == OperationLogAction::Delete);
    assert(!log.Entries()[0].succeeded);

    printf("operation-log-host-tests-ok\n");
    return 0;
}
