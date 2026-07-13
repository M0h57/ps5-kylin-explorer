#ifndef KYLIN_EXPLORER_NATIVE_DIALOGS_H
#define KYLIN_EXPLORER_NATIVE_DIALOGS_H

#include <string>

enum class TextInputResult {
    None,
    Accepted,
    Canceled,
    Failed,
};

class NativeDialogs {
public:
    NativeDialogs();
    bool Init(int userId);
    bool OpenTextInput(const std::string &title, const std::string &placeholder,
                       const std::string &initialValue);
    TextInputResult UpdateTextInput();
    bool IsTextInputOpen() const;
    const std::string &TextInputValue() const;

private:
    int userId_;
    bool initialized_;
    bool textInputOpen_;
    std::string textInputValue_;
    wchar_t title_[96];
    wchar_t placeholder_[96];
    wchar_t input_[256];
};

#endif
