#include "app.h"
#include "logger.h"
#include "utils.h"
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include "cJSON.h"

#ifndef AF_INET
#define AF_INET 2
#endif

#ifndef SOCK_STREAM
#define SOCK_STREAM 1
#endif

void App::UpdatePkgInstallerState(int progress, const std::string &status, bool finished) {
    pkgInstallerProgress_ = progress;
    pkgInstallerStatus_ = status;
    pkgInstallerFinished_ = finished;
}

namespace {
bool LoadInternalModule(OrbisSysModuleInternal module, const char *name) {
    const int result = static_cast<int>(sceSysmoduleLoadModuleInternal(module));
    if (result < 0) {
        LOG_INFO("sysmodule %s failed: 0x%08x", name, result);
        return false;
    }
    LOG_INFO("sysmodule %s ok", name);
    return true;
}

bool LoadModule(OrbisSysModule module, const char *name) {
    const int result = sceSysmoduleLoadModule(module);
    if (result < 0) {
        LOG_INFO("sysmodule %s failed: 0x%08x", name, result);
        return false;
    }
    LOG_INFO("sysmodule %s ok", name);
    return true;
}

void* PkgInstallerThreadFunc(void *arg) {
    App *app = static_cast<App*>(arg);
    std::string path = app->GetPkgInstallerPath();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        app->UpdatePkgInstallerState(0, "Error: socket failed", true);
        return NULL;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
#ifdef __APPLE__
    server.sin_len = sizeof(server);
#endif
    server.sin_family = AF_INET;
    // 9023 = 0x233F -> htons(9023) = 0x3F23
    server.sin_port = 0x3F23;
    // 127.0.0.1 = 0x7F000001 -> htonl(0x7F000001) = 0x0100007F
    server.sin_addr.s_addr = 0x0100007F;

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        app->UpdatePkgInstallerState(0, "Error: connect failed", true);
        close(sock);
        return NULL;
    }

    char payload[1024];
    snprintf(payload, sizeof(payload), "{\"path\":\"%s\"}", path.c_str());
    
    char request[2048];
    snprintf(request, sizeof(request),
             "POST /api/v1/packages/install HTTP/1.1\r\n"
             "Host: 127.0.0.1:9023\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n\r\n"
             "%s", strlen(payload), payload);

    send(sock, request, strlen(request), 0);

    char response[4096];
    int received = recv(sock, response, sizeof(response) - 1, 0);
    if (received <= 0) {
        app->UpdatePkgInstallerState(0, "Error: recv failed", true);
        close(sock);
        return NULL;
    }
    response[received] = '\0';
    close(sock);

    char *body = strstr(response, "\r\n\r\n");
    if (!body) {
        app->UpdatePkgInstallerState(0, "Error: invalid response", true);
        return NULL;
    }
    body += 4;

    cJSON *json = cJSON_Parse(body);
    if (!json) {
        app->UpdatePkgInstallerState(0, "Error: invalid JSON", true);
        return NULL;
    }

    cJSON *jobIdNode = cJSON_GetObjectItem(json, "jobId");
    if (!jobIdNode || !cJSON_IsString(jobIdNode)) {
        app->UpdatePkgInstallerState(0, "Error: missing jobId", true);
        cJSON_Delete(json);
        return NULL;
    }

    std::string jobId = jobIdNode->valuestring;
    cJSON_Delete(json);

    app->UpdatePkgInstallerState(0, "Requesting events...", false);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        app->UpdatePkgInstallerState(0, "Error: connect SSE failed", true);
        close(sock);
        return NULL;
    }

    snprintf(request, sizeof(request),
             "GET /api/v1/packages/jobs/events?job=%s HTTP/1.1\r\n"
             "Host: 127.0.0.1:9023\r\n"
             "Accept: text/event-stream\r\n\r\n", jobId.c_str());

    send(sock, request, strlen(request), 0);

    char line[1024];
    int line_len = 0;
    while (!app->IsPkgCancelRequested()) {
        char c;
        if (recv(sock, &c, 1, 0) <= 0) break;
        if (c == '\r') continue;
        if (c == '\n') {
            line[line_len] = '\0';
            if (strncmp(line, "data: ", 6) == 0) {
                cJSON *eventJson = cJSON_Parse(line + 6);
                if (eventJson) {
                    cJSON *progNode = cJSON_GetObjectItem(eventJson, "progress");
                    cJSON *statNode = cJSON_GetObjectItem(eventJson, "state");
                    
                    int p = progNode ? progNode->valueint : 0;
                    std::string s = statNode && cJSON_IsString(statNode) ? statNode->valuestring : "";
                    
                    bool finished = (s == "Completed" || s == "Failed");
                    app->UpdatePkgInstallerState(p, s, finished);
                    
                    cJSON_Delete(eventJson);
                    if (finished) break;
                }
            }
            line_len = 0;
        } else if (line_len < static_cast<int>(sizeof(line) - 1)) {
            line[line_len++] = c;
        }
    }

    if (app->IsPkgCancelRequested()) {
        // Send cancel request
        int cancelSock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(cancelSock, (struct sockaddr *)&server, sizeof(server)) == 0) {
            snprintf(payload, sizeof(payload), "{\"jobId\":\"%s\"}", jobId.c_str());
            snprintf(request, sizeof(request),
                     "POST /api/v1/packages/jobs/cancel HTTP/1.1\r\n"
                     "Host: 127.0.0.1:9023\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %zu\r\n\r\n"
                     "%s", strlen(payload), payload);
            send(cancelSock, request, strlen(request), 0);
            close(cancelSock);
        }
        app->UpdatePkgInstallerState(0, "Canceled", true);
    }

    close(sock);
    return NULL;
}

const int kFrameWidth = 1920;
const int kFrameHeight = 1080;
const int kFrameDepth = 4;
const char *kFontPath = "embedded:NotoSansSC-Regular.ttf";
const char *kFallbackFontPath = "embedded:NotoSansSC-Regular.ttf";

std::vector<InstalledApp> ScanInstalledApps() {
    std::vector<InstalledApp> apps;
    const char *appRoot = "/user/app";
    DIR *dir = opendir(appRoot);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string titleId = entry->d_name;
            if (titleId == "." || titleId == "..") continue;
            
            std::string path = JoinPath(appRoot, titleId);
            struct stat st;
            if (lstat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                std::string sfoPath = path + "/sce_sys/param.sfo";
                std::string title = "";
                std::string version = "";
                if (ParseSfo(sfoPath, title, version)) {
                    if (title.empty()) title = titleId;
                    
                    InstalledApp app;
                    app.title = title;
                    app.titleId = titleId;
                    app.version = version;
                    app.path = path;
                    apps.push_back(app);
                }
            }
        }
        closedir(dir);
    }
    
    // Fallback/Mock list for debugging or host environment:
    if (apps.empty()) {
        apps.push_back({"Kylin Explorer", "KYLN00002", "0.62", "/user/app/KYLN00002"});
        apps.push_back({"GoldHEN", "GOLD00001", "2.3", "/user/app/GOLD00001"});
        apps.push_back({"Orbis Toolbox", "ORBS00001", "1.0", "/user/app/ORBS00001"});
        apps.push_back({"Apollo Save Tool", "APOL00001", "1.4", "/user/app/APOL00001"});
        apps.push_back({"PS4-Xplorer 2.05", "LAPY20009", "2.05", "/user/app/LAPY20009"});
    }
    return apps;
}

std::vector<BrowserLocation> ScanQuickPlaces() {
    std::vector<BrowserLocation> places;
    // USB 0–6 — matches glimpse_view.cpp port range
    for (int i = 0; i < 7; ++i) {
        char path[32];
        snprintf(path, sizeof(path), "/mnt/usb%d", i);
        if (IsMounted(path)) {
            char label[16];
            snprintf(label, sizeof(label), "USB%d", i);
            places.push_back({label, path});
        }
    }
    // EXT 1 only — matches glimpse_view.cpp
    if (IsMounted("/mnt/ext1")) {
        places.push_back({"EXT1", "/mnt/ext1"});
    }
    return places;
}

bool CanonicalizePathOrParent(const std::string &path, std::string *canonical) {
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved) != NULL) {
        *canonical = resolved;
        return true;
    }
    const std::string parent = ParentPath(path);
    if (parent != path && realpath(parent.c_str(), resolved) != NULL) {
        *canonical = resolved;
        return true;
    }
    return false;
}

}

App::App()
    : scene_(NULL), bodyLatin_(NULL), bodyFallback_(NULL),
      smallLatin_(NULL), smallFallback_(NULL),
      browserInitialized_(false), ui_(NULL),
      pendingTextInput_(PendingTextInput::None),
      propertiesOpen_(false), errorOpen_(false),
      overwriteConfirmOpen_(false), deleteConfirmOpen_(false), recursiveDeleteConfirmOpen_(false),
      operationLogOpen_(false), installedPkgsOpen_(false), installedPkgsIndex_(0),
      frameId_(0), quickPlacesOpen_(false), quickPlacesIndex_(0),
      settingsOpen_(false), settingsIndex_(0) {
}

App::~App() {
}

bool App::Init() {
    setvbuf(stdout, NULL, _IONBF, 0);

    LOG_INFO("app init begin");
    if (!LoadInternalModule(ORBIS_SYSMODULE_INTERNAL_VIDEO_OUT, "video_out") ||
        !LoadInternalModule(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE, "user_service") ||
        !LoadInternalModule(ORBIS_SYSMODULE_INTERNAL_PAD, "pad") ||
        !LoadInternalModule(static_cast<OrbisSysModuleInternal>(0x80000010), "system_service") ||
        !LoadModule(ORBIS_SYSMODULE_IME_DIALOG, "ime_dialog") ||
        !LoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG, "message_dialog")) {
        LOG_INFO("app init failed: sysmodule");
        return false;
    }

    LOG_INFO("scene alloc begin");
    scene_ = new Scene2D(kFrameWidth, kFrameHeight, kFrameDepth);
    LOG_INFO("scene init begin");
    if (!scene_->Init(0xC000000, 2)) {
        LOG_INFO("scene init failed");
        return false;
    }
    LOG_INFO("scene init ok");

    LOG_INFO("language init begin");
    int32_t lang = -1;
    if (sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG, &lang) == 0) {
        if (lang == ORBIS_SYSTEM_PARAM_LANG_CHINESE_S || lang == ORBIS_SYSTEM_PARAM_LANG_CHINESE_T) {
            localizer_.SetLanguage(Language::SimplifiedChinese);
        } else {
            localizer_.SetLanguage(Language::English);
        }
        LOG_INFO("language set to %d", lang);
    } else {
        LOG_INFO("language param get failed");
    }

    LOG_INFO("font init begin");
    if (!scene_->InitFont(&bodyLatin_, kFontPath, 30) ||
        !scene_->InitFont(&bodyFallback_, kFallbackFontPath, 30) ||
        !scene_->InitFont(&smallLatin_, kFontPath, 21) ||
        !scene_->InitFont(&smallFallback_, kFallbackFontPath, 21)) {
        LOG_INFO("font init failed");
        return false;
    }
    LOG_INFO("font body latin face: %p", bodyLatin_);
    LOG_INFO("font body fallback face: %p", bodyFallback_);
    LOG_INFO("font small latin face: %p", smallLatin_);
    LOG_INFO("font small fallback face: %p", smallFallback_);
    LOG_INFO("font init ok");

    LOG_INFO("input init begin");
    if (!input_.Init()) {
        LOG_INFO("input init failed");
        return false;
    }
    LOG_INFO("input init ok");

    LOG_INFO("dialogs init begin");
    dialogs_.Init(input_.UserId());
    LOG_INFO("dialogs init done");
    LOG_INFO("ui init begin");
    ui_ = new ExplorerUi(scene_, bodyLatin_, bodyFallback_, smallLatin_, smallFallback_, &localizer_);
    LOG_INFO("app init ok");
    return true;
}

void App::Run() {
    LOG_INFO("run loop begin");
    for (;;) {
        input_.Update();

        if (!browserInitialized_) {
            LOG_INFO("browser init begin");
            browser_.Initialize();
            LOG_INFO("browser open root begin");
            browser_.Open("/");
            LOG_INFO("browser open root done");
            browserInitialized_ = true;
        }

        operations_.Update();

        if (operations_.HasResult()) {
            const CopyState opState = operations_.State();
            const bool succeeded = (opState == CopyState::Completed);
            const bool canceled = (opState == CopyState::Canceled);

            OperationLogAction logAction = OperationLogAction::Copy;
            if (operations_.IsMoveOperation()) logAction = OperationLogAction::Move;
            if (operations_.IsDeleteOperation()) logAction = OperationLogAction::Delete;
            operationLog_.Add(logAction, operations_.CurrentName(), succeeded);

            if (succeeded) {
                browser_.Refresh();

                std::string title;
                std::string msg;
                if (operations_.IsDeleteOperation()) {
                    title = localizer_.Get(TextId::DeleteCompleted);
                    msg = localizer_.CurrentLanguage() == Language::SimplifiedChinese ?
                          "所选项目已成功删除。" : "The selected items have been deleted successfully.";
                } else if (operations_.IsMoveOperation()) {
                    title = localizer_.Get(TextId::MoveCompleted);
                    msg = localizer_.CurrentLanguage() == Language::SimplifiedChinese ?
                          "所选项目已成功移动。" : "The selected items have been moved successfully.";
                } else {
                    title = localizer_.Get(TextId::CopyCompleted);
                    msg = localizer_.CurrentLanguage() == Language::SimplifiedChinese ?
                          "所选项目已成功复制。" : "The selected items have been copied successfully.";
                }

                ui_->SetErrorText(msg, title);
                errorOpen_ = true;
                ShowToast(operations_.IsDeleteOperation() ? localizer_.Get(TextId::StatusDeleted) : localizer_.Get(TextId::StatusCopied), ToastType::Success);
            }
            else if (canceled) {
                ShowToast(operations_.IsMoveOperation() ? localizer_.Get(TextId::MoveCanceled) : localizer_.Get(TextId::CopyCanceled), ToastType::Warning);
            }
            else { // Failed
                const std::string errStr = operations_.Error();
                LOG_INFO("Operation failed: %s", errStr.c_str());
                ui_->SetErrorText(std::string("Operation failed:\n") + errStr);
                errorOpen_ = true;
                ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
            }

            operations_.Acknowledge();
        }
        ProcessTextInputResult();
        HandleInput();
        toastManager_.Update();
        if (frameId_ == 0) {
            LOG_INFO("first draw begin");
        }
        Draw();
        if (frameId_ == 0) {
            LOG_INFO("first flip begin");
        }

        scene_->SubmitFlip(frameId_);
        scene_->FrameWait(frameId_);
        scene_->FrameBufferSwap();
        if (frameId_ == 0) {
            LOG_INFO("first frame ok");
            sceSystemServiceHideSplashScreen();
        }
        ++frameId_;
    }
}

void App::ProcessTextInputResult() {
    const TextInputResult inputResult = dialogs_.UpdateTextInput();
    if (inputResult == TextInputResult::Accepted) {
        bool succeeded = true;
        const std::string inputValue = dialogs_.TextInputValue();
        if (pendingTextInput_ == PendingTextInput::NewFolder) {
            succeeded = browser_.CreateDirectory(inputValue);
            operationLog_.Add(OperationLogAction::NewFolder, JoinPath(browser_.CurrentPath(), inputValue), succeeded);
            if (succeeded) {
                ShowToast(localizer_.Get(TextId::StatusCreated), ToastType::Success);
            } else {
                ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
            }
        }
        if (pendingTextInput_ == PendingTextInput::Rename) {
            succeeded = browser_.RenameSelected(inputValue);
            operationLog_.Add(OperationLogAction::Rename, JoinPath(browser_.CurrentPath(), inputValue), succeeded);
            if (succeeded) {
                ShowToast(localizer_.Get(TextId::StatusRenamed), ToastType::Success);
            } else {
                ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
            }
        }
        errorOpen_ = !succeeded;
        pendingTextInput_ = PendingTextInput::None;
    } else if (inputResult == TextInputResult::Canceled || inputResult == TextInputResult::Failed) {
        pendingTextInput_ = PendingTextInput::None;
    }
}

UiState App::DetermineCurrentState() const {
    if (operations_.IsBusy()) return UiState::OperationRunning;
    if (operations_.HasResult()) return UiState::OperationFinished;
    if (dialogs_.IsTextInputOpen()) return UiState::TextInput;
    if (errorOpen_) return UiState::Error;
    if (propertiesOpen_) return UiState::Properties;
    if (operationLogOpen_) return UiState::OperationLog;
    if (installedPkgsOpen_) return UiState::InstalledPkgs;
    if (overwriteConfirmOpen_) return UiState::OverwriteConfirm;
    if (deleteConfirmOpen_) return UiState::DeleteConfirm;
    if (recursiveDeleteConfirmOpen_) return UiState::RecursiveDeleteConfirm;
    if (quickPlacesOpen_) return UiState::QuickPlaces;
    if (settingsOpen_) return UiState::Settings;
    if (menu_.IsOpen()) return UiState::ContextMenu;
    return UiState::Browsing;
}

void App::HandleInput() {
    switch (DetermineCurrentState()) {
        case UiState::OperationRunning:       HandleOperationRunningInput(); break;
        case UiState::OperationFinished:      HandleOperationFinishedInput(); break;
        case UiState::TextInput:              /* Dialogue owns Pad input */ break;
        case UiState::Error:                  HandleErrorInput(); break;
        case UiState::Properties:             HandlePropertiesInput(); break;
        case UiState::OperationLog:           HandleOperationLogInput(); break;
        case UiState::InstalledPkgs:          HandleInstalledPkgsInput(); break;
        case UiState::PkgInstaller:           HandlePkgInstallerInput(); break;
        case UiState::OverwriteConfirm:       HandleOverwriteConfirmInput(); break;
        case UiState::DeleteConfirm:          HandleDeleteConfirmInput(); break;
        case UiState::RecursiveDeleteConfirm: HandleRecursiveDeleteConfirmInput(); break;
        case UiState::ContextMenu:            HandleContextMenuInput(); break;
        case UiState::QuickPlaces:            HandleQuickPlacesInput(); break;
        case UiState::Settings:               HandleSettingsInput(); break;
        case UiState::Browsing:               HandleBrowsingInput(); break;
    }
}

void App::Draw() {
    ui_->Draw(browser_, menu_, operations_, propertiesOpen_, errorOpen_,
              overwriteConfirmOpen_, deleteConfirmOpen_, recursiveDeleteConfirmOpen_,
              operationLogOpen_, operationLog_,
              installedPkgsOpen_, installedPkgsIndex_, installedApps_,
              settingsOpen_, settingsIndex_, config_,
              quickPlacesOpen_, quickPlacesIndex_, quickPlaces_,
              pkgInstallerOpen_, pkgInstallerPath_, pkgInstallerProgress_, pkgInstallerStatus_, pkgInstallerFinished_,
              toastManager_,
              input_.Held(ORBIS_PAD_BUTTON_L2));
}

void App::HandleOperationRunningInput() {
    if (operations_.CanCancel() && input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        operations_.Cancel();
    }
}

void App::HandleOperationFinishedInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS) || input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        const bool succeeded = operations_.State() == CopyState::Completed;
        OperationLogAction action = OperationLogAction::Copy;
        if (operations_.IsMoveOperation()) action = OperationLogAction::Move;
        if (operations_.IsDeleteOperation()) action = OperationLogAction::Delete;
        operationLog_.Add(action, operations_.CurrentName(), succeeded);
        if (operations_.State() == CopyState::Completed) {
            browser_.Refresh();
            ShowToast(operations_.IsDeleteOperation() ? localizer_.Get(TextId::StatusDeleted) : localizer_.Get(TextId::StatusCopied), ToastType::Success);
        } else {
            LOG_INFO("Operation failed: %s", operations_.Error().c_str());
            ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
        }
        operations_.Acknowledge();
    }
}

void App::HandleErrorInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS) || input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        errorOpen_ = false;
        ui_->SetErrorText("", "");
    }
}

void App::HandlePropertiesInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE) || input_.Pressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
        propertiesOpen_ = false;
    }
}

void App::HandleOperationLogInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE) || input_.Pressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
        operationLogOpen_ = false;
    }
}

void App::HandleInstalledPkgsInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_UP) && !installedApps_.empty()) {
        installedPkgsIndex_ = installedPkgsIndex_ <= 0 ? static_cast<int>(installedApps_.size()) - 1 : installedPkgsIndex_ - 1;
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_DOWN) && !installedApps_.empty()) {
        installedPkgsIndex_ = (installedPkgsIndex_ + 1) % static_cast<int>(installedApps_.size());
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE) || input_.Pressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
        installedPkgsOpen_ = false;
    }
}

void App::HandlePkgInstallerInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        if (pkgInstallerFinished_) {
            pkgInstallerOpen_ = false;
        } else {
            pkgInstallerCancelRequested_ = true;
        }
    }
}

void App::HandleOverwriteConfirmInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS)) {
        overwriteConfirmOpen_ = false;
        if (!operations_.StartPaste(pendingPasteDestination_, ExistingTargetPolicy::Replace) &&
            !operations_.HasResult()) {
            browser_.ReportOperationFailed();
            errorOpen_ = true;
        }
        pendingPasteDestination_.clear();
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        overwriteConfirmOpen_ = false;
        pendingPasteDestination_.clear();
    }
}

void App::HandleDeleteConfirmInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS)) {
        deleteConfirmOpen_ = false;
        const std::vector<FileEntry> entries = ActiveEntries();
        bool includesDirectory = false;
        for (size_t index = 0; index < entries.size(); ++index) {
            if (entries[index].isDirectory) includesDirectory = true;
        }
        if (includesDirectory) {
            recursiveDeleteConfirmOpen_ = true;
        } else if (operations_.StartDelete(entries)) {
            browser_.ClearSelectionMarks();
        } else {
            LOG_ERROR("HandleDeleteConfirm: StartDelete failed for %zu entries", entries.size());
            browser_.ReportOperationFailed();
            errorOpen_ = true;
        }
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        deleteConfirmOpen_ = false;
    }
}

void App::HandleRecursiveDeleteConfirmInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS)) {
        recursiveDeleteConfirmOpen_ = false;
        if (operations_.StartDelete(ActiveEntries())) {
            browser_.ClearSelectionMarks();
        } else {
            LOG_ERROR("HandleRecursiveDeleteConfirm: StartDelete failed");
            browser_.ReportOperationFailed();
            errorOpen_ = true;
        }
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        recursiveDeleteConfirmOpen_ = false;
    }
}

void App::HandleContextMenuInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_UP)) {
        menu_.Move(-1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_DOWN)) {
        menu_.Move(1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE) || input_.Pressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
        menu_.Close();
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS)) {
        const MenuItem &item = menu_.Selected();
        if (item.enabled) {
            ExecuteMenuAction(item.action);
        }
    }
}

void App::HandleBrowsingInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_UP)) {
        browser_.MoveSelection(input_.Held(ORBIS_PAD_BUTTON_L2) ? -10 : -1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_DOWN)) {
        browser_.MoveSelection(input_.Held(ORBIS_PAD_BUTTON_L2) ? 10 : 1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_LEFT)) {
        browser_.CycleSort(-1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_RIGHT)) {
        browser_.CycleSort(1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS)) {
        if (browser_.OpenSelected() && browser_.Selected() != NULL && browser_.Selected()->type == FileType::Package) {
            pkgInstallerOpen_ = true;
            pkgInstallerPath_ = browser_.Selected()->path;
            pkgInstallerProgress_ = 0;
            pkgInstallerStatus_ = "Connecting...";
            pkgInstallerFinished_ = false;
            pkgInstallerCancelRequested_ = false;
            
            pthread_create(&pkgInstallerThread_, NULL, PkgInstallerThreadFunc, this);
            pthread_detach(pkgInstallerThread_);
        }
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_SQUARE)) {
        browser_.ToggleSelectedMark();
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        browser_.GoBack();
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
        menu_.Open(browser_.Selected() != NULL, operations_.HasClipboard());
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_OPTIONS)) {
        quickPlaces_ = ScanQuickPlaces();
        quickPlacesIndex_ = 0;
        quickPlacesOpen_ = true;
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_L1)) {
        browser_.SwitchLocation(-1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_R1)) {
        browser_.SwitchLocation(1);
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_TOUCH_PAD)) {
        settingsOpen_ = true;
        settingsIndex_ = 0;
    }
}

void App::ExecuteMenuAction(MenuAction action) {
    if (action == MenuAction::Refresh) {
        browser_.Refresh();
        ShowToast(localizer_.Get(TextId::StatusRefreshed), ToastType::Success);
    }
    if (action == MenuAction::CycleSort) {
        browser_.CycleSort(1);
    }
    if (action == MenuAction::Properties) {
        propertiesOpen_ = browser_.Selected() != NULL;
        menu_.Close();
    }
    if (action == MenuAction::NewFolder) {
        if (dialogs_.OpenTextInput(localizer_.Get(TextId::InputNewFolderTitle),
                                  localizer_.Get(TextId::InputNewFolderPlaceholder), "")) {
            pendingTextInput_ = PendingTextInput::NewFolder;
            menu_.Close();
        } else {
            browser_.ReportOperationFailed();
            errorOpen_ = true;
            ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
        }
    }
    if (action == MenuAction::Rename) {
        const FileEntry *selected = browser_.Selected();
        if (selected != NULL &&
            dialogs_.OpenTextInput(localizer_.Get(TextId::InputRenameTitle),
                                  localizer_.Get(TextId::InputRenamePlaceholder),
                                  selected->name)) {
            pendingTextInput_ = PendingTextInput::Rename;
            menu_.Close();
        } else {
            browser_.ReportOperationFailed();
            errorOpen_ = true;
            ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
        }
    }
    if (action == MenuAction::Copy) {
        if (!operations_.CopyToClipboard(ActiveEntries())) {
            browser_.ReportOperationFailed();
            errorOpen_ = true;
            ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
        } else {
            browser_.ClearSelectionMarks();
            menu_.Close();
            ShowToast("Added to clipboard", ToastType::Info);
        }
    }
    if (action == MenuAction::Cut) {
        if (!operations_.CutToClipboard(ActiveEntries())) {
            browser_.ReportOperationFailed();
            errorOpen_ = true;
            ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
        } else {
            browser_.ClearSelectionMarks();
            menu_.Close();
            ShowToast("Added to clipboard", ToastType::Info);
        }
    }
    if (action == MenuAction::Paste) {
        menu_.Close();
        RequestPaste(browser_.CurrentPath());
    }
    if (action == MenuAction::Delete) {
        const std::vector<FileEntry> entries = ActiveEntries();
        LOG_INFO("Delete requested: %zu entries from %s", entries.size(), browser_.CurrentPath().c_str());
        deleteConfirmOpen_ = !entries.empty();
        menu_.Close();
    }
}

void App::RequestPaste(const std::string &destination) {
    if (operations_.DestinationExists(destination) && operations_.CanReplaceDestination(destination)) {
        pendingPasteDestination_ = destination;
        overwriteConfirmOpen_ = true;
        return;
    }
    if (!operations_.StartPaste(destination) && !operations_.HasResult()) {
        browser_.ReportOperationFailed();
        errorOpen_ = true;
        ShowToast(localizer_.Get(TextId::StatusOperationFailed), ToastType::Error);
    } else {
        ShowToast("Processing...", ToastType::Info);
    }
}

std::vector<FileEntry> App::ActiveEntries() const {
    if (browser_.HasMarkedEntries()) {
        return browser_.MarkedEntries();
    }
    std::vector<FileEntry> entries;
    const FileEntry *selected = browser_.Selected();
    if (selected != NULL) entries.push_back(*selected);
    return entries;
}

void App::HandleQuickPlacesInput() {
    const int count = static_cast<int>(quickPlaces_.size());
    if (input_.Pressed(ORBIS_PAD_BUTTON_UP)) {
        quickPlacesIndex_ = quickPlacesIndex_ <= 0 ? count - 1 : quickPlacesIndex_ - 1;
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_DOWN)) {
        quickPlacesIndex_ = (quickPlacesIndex_ + 1) % count;
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS) && count > 0) {
        browser_.Open(quickPlaces_[quickPlacesIndex_].path);
        quickPlacesOpen_ = false;
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE) || input_.Pressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
        quickPlacesOpen_ = false;
    }
}

void App::HandleSettingsInput() {
    if (input_.Pressed(ORBIS_PAD_BUTTON_UP)) {
        settingsIndex_ = settingsIndex_ <= 0 ? 1 : settingsIndex_ - 1;
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_DOWN)) {
        settingsIndex_ = (settingsIndex_ + 1) % 2;
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_LEFT) || input_.Pressed(ORBIS_PAD_BUTTON_RIGHT)) {
        if (settingsIndex_ == 0) {
            localizer_.ToggleLanguage();
            ShowToast(localizer_.LanguageName(), ToastType::Success);
        } else if (settingsIndex_ == 1) {
            config_.tempFahrenheit = !config_.tempFahrenheit;
            ShowToast(localizer_.Get(TextId::SettingsTempUnit), ToastType::Success);
        }
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CROSS)) {
        if (settingsIndex_ == 0) {
            localizer_.ToggleLanguage();
            ShowToast(localizer_.LanguageName(), ToastType::Success);
        } else if (settingsIndex_ == 1) {
            config_.tempFahrenheit = !config_.tempFahrenheit;
            ShowToast(localizer_.Get(TextId::SettingsTempUnit), ToastType::Success);
        }
    } else if (input_.Pressed(ORBIS_PAD_BUTTON_CIRCLE) || input_.Pressed(ORBIS_PAD_BUTTON_TOUCH_PAD)) {
        settingsOpen_ = false;
    }
}

void App::ShowToast(const std::string &message, ToastType type) {
    toastManager_.Add(message, type);
}
