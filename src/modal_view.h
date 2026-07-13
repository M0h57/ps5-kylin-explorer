#ifndef KYLIN_EXPLORER_MODAL_VIEW_H
#define KYLIN_EXPLORER_MODAL_VIEW_H

#include <string>
#include <vector>

#include "file_browser.h"

struct UiContext;
class FileOperations;
class OperationLog;

class ModalView {
public:
    ModalView();
    void DrawProperties(const UiContext &ctx, const FileBrowser &browser);
    void DrawError(const UiContext &ctx, const FileBrowser &browser, const std::string &errorText = "", const std::string &title = "");
    void DrawCopyProgress(const UiContext &ctx, const FileOperations &operations);

    void DrawQuickPlaces(const UiContext &ctx, const std::vector<BrowserLocation> &places, int selectedIndex);
    void DrawOverwriteConfirm(const UiContext &ctx);
    void DrawDeleteConfirm(const UiContext &ctx);
    void DrawRecursiveDeleteConfirm(const UiContext &ctx);
    void DrawOperationLog(const UiContext &ctx, const OperationLog &operationLog);
    void DrawPkgProgress(const UiContext &ctx, const std::string &path, int progress, const std::string &status, bool finished);
};

#endif
