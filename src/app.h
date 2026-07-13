#ifndef KYLIN_EXPLORER_APP_H
#define KYLIN_EXPLORER_APP_H

#include <sstream>
#include <string>
#include <vector>
#include <orbis/Pad.h>

#include "graphics.h"
#include "file_browser.h"
#include "file_operations.h"
#include "context_menu.h"
#include "input.h"
#include "localization.h"
#include "native_dialogs.h"
#include "operation_log.h"
#include "sfo_parser.h"
#include "ui.h"
#include "toast.h"

enum class UiState {
    Browsing,
    ContextMenu,
    TextInput,
    Properties,
    Error,
    OperationLog,
    InstalledPkgs,
    OverwriteConfirm,
    DeleteConfirm,
    RecursiveDeleteConfirm,
    OperationRunning,
    OperationFinished,
    QuickPlaces,
    Settings,
    PkgInstaller,
};

class App {
public:
    App();
    ~App();

    bool Init();
    void Run();

    void UpdatePkgInstallerState(int progress, const std::string &status, bool finished);
    std::string GetPkgInstallerPath() const { return pkgInstallerPath_; }
    bool IsPkgCancelRequested() const { return pkgInstallerCancelRequested_; }

private:
    void Update();
    void HandleInput();
    void Draw();

    // Input handlers for specific states
    void HandleBrowsingInput();
    void HandleContextMenuInput();
    void HandlePropertiesInput();
    void HandleErrorInput();
    void HandleOperationLogInput();
    void HandleInstalledPkgsInput();
    void HandleOverwriteConfirmInput();
    void HandleDeleteConfirmInput();
    void HandleRecursiveDeleteConfirmInput();
    void HandleOperationRunningInput();
    void HandleOperationFinishedInput();
    void HandleSettingsInput();
    void HandleQuickPlacesInput();
    void HandlePkgInstallerInput();
    // Context Menu execution helpers
    void ExecuteMenuAction(MenuAction action);
    void ProcessTextInputResult();
    void RequestPaste(const std::string &destination);
    UiState DetermineCurrentState() const;
    std::vector<FileEntry> ActiveEntries() const;

    // Orchestrator properties
    Scene2D *scene_;
    FT_Face bodyLatin_;
    FT_Face bodyFallback_;
    FT_Face smallLatin_;
    FT_Face smallFallback_;

    Input input_;
    FileBrowser browser_;
    bool browserInitialized_;
    FileOperations operations_;
    OperationLog operationLog_;
    Localizer localizer_;
    ContextMenu menu_;
    NativeDialogs dialogs_;
    ExplorerUi *ui_;
    ToastManager toastManager_;

    void ShowToast(const std::string &message, ToastType type = ToastType::Success);

    enum class PendingTextInput {
        None,
        NewFolder,
        Rename,
    };
    PendingTextInput pendingTextInput_;

    bool propertiesOpen_;
    bool errorOpen_;
    bool overwriteConfirmOpen_;
    bool deleteConfirmOpen_;
    bool recursiveDeleteConfirmOpen_;
    bool operationLogOpen_;
    bool installedPkgsOpen_;
    int installedPkgsIndex_;
    std::vector<InstalledApp> installedApps_;
    std::string pendingPasteDestination_;
    int frameId_;
    bool quickPlacesOpen_;
    int quickPlacesIndex_;
    std::vector<BrowserLocation> quickPlaces_;
    bool settingsOpen_;
    int settingsIndex_;
    AppConfig config_;

    // Package Installer state
    bool pkgInstallerOpen_;
    std::string pkgInstallerPath_;
    int pkgInstallerProgress_;
    std::string pkgInstallerStatus_;
    bool pkgInstallerFinished_;
    pthread_t pkgInstallerThread_;
    bool pkgInstallerCancelRequested_;
};

#endif
