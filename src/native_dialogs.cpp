#include "native_dialogs.h"

#include <string.h>
#include <orbis/ImeDialog.h>
#include <orbis/Sysmodule.h>
#include "utf8.h"

NativeDialogs::NativeDialogs()
    : userId_(-1), initialized_(false), textInputOpen_(false) {
    memset(title_, 0, sizeof(title_));
    memset(placeholder_, 0, sizeof(placeholder_));
    memset(input_, 0, sizeof(input_));
}

bool NativeDialogs::Init(int userId) {
    userId_ = userId;
    initialized_ = true;
    return true;
}

bool NativeDialogs::OpenTextInput(const std::string &title, const std::string &placeholder,
                                  const std::string &initialValue) {
    if (!initialized_ || textInputOpen_) return false;

    WideFromUtf8(title, title_, sizeof(title_) / sizeof(title_[0]));
    WideFromUtf8(placeholder, placeholder_, sizeof(placeholder_) / sizeof(placeholder_[0]));
    WideFromUtf8(initialValue, input_, sizeof(input_) / sizeof(input_[0]));

    OrbisImeDialogSetting setting;
    memset(&setting, 0, sizeof(setting));
    setting.userId = userId_;
    setting.type = ORBIS_TYPE_DEFAULT;
    setting.enterLabel = ORBIS_BUTTON_LABEL_DEFAULT;
    setting.inputMethod = ORBIS__DEFAULT;
    setting.maxTextLength = 255;
    setting.inputTextBuffer = input_;
    setting.horizontalAlignment = ORBIS_H_CENTER;
    setting.verticalAlignment = ORBIS_V_CENTER;
    setting.placeholder = placeholder_;
    setting.title = title_;

    textInputOpen_ = sceImeDialogInit(&setting, NULL) >= 0;
    return textInputOpen_;
}

TextInputResult NativeDialogs::UpdateTextInput() {
    if (!textInputOpen_) return TextInputResult::None;
    if (sceImeDialogGetStatus() != ORBIS_DIALOG_STATUS_STOPPED) return TextInputResult::None;

    OrbisDialogResult result;
    memset(&result, 0, sizeof(result));
    const bool readResult = sceImeDialogGetResult(&result) >= 0;
    sceImeDialogTerm();
    textInputOpen_ = false;
    if (!readResult) return TextInputResult::Failed;
    if (result.endstatus != ORBIS_DIALOG_OK) return TextInputResult::Canceled;

    textInputValue_ = Utf8FromWide(input_);
    return TextInputResult::Accepted;
}

bool NativeDialogs::IsTextInputOpen() const { return textInputOpen_; }
const std::string &NativeDialogs::TextInputValue() const { return textInputValue_; }
