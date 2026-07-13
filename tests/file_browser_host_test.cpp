#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/file_browser.h"

namespace {
std::string JoinPath(const std::string &base, const std::string &name) {
    return base + "/" + name;
}

void WriteFile(const std::string &path, const char *content) {
    FILE *file = fopen(path.c_str(), "wb");
    assert(file != NULL);
    assert(fwrite(content, 1, strlen(content), file) == strlen(content));
    fclose(file);
}

void WriteBytes(const std::string &path, const unsigned char *content, size_t size) {
    FILE *file = fopen(path.c_str(), "wb");
    assert(file != NULL);
    assert(fwrite(content, 1, size, file) == size);
    fclose(file);
}

void Select(FileBrowser &browser, const std::string &name) {
    for (size_t count = 0; count < browser.Entries().size(); ++count) {
        if (browser.Selected() != NULL && browser.Selected()->name == name) return;
        browser.MoveSelection(1);
    }
    assert(false);
}

bool HasEntry(const FileBrowser &browser, const std::string &name) {
    for (size_t index = 0; index < browser.Entries().size(); ++index) {
        if (browser.Entries()[index].name == name) return true;
    }
    return false;
}

int EntryIndex(const FileBrowser &browser, const std::string &name) {
    for (size_t index = 0; index < browser.Entries().size(); ++index) {
        if (browser.Entries()[index].name == name) return static_cast<int>(index);
    }
    return -1;
}
}

int main() {
    std::string invalidBytes = "bad-";
    invalidBytes.push_back(static_cast<char>(0xFF));
    invalidBytes += "-name";
    DisplayName decoded = DisplayCodec::DecodeBytes(invalidBytes);
    assert(decoded.lossy);
    assert(decoded.text.find("\\xFF") != std::string::npos);

    char rootTemplate[] = "/tmp/kylin-file-browser-XXXXXX";
    const std::string root = mkdtemp(rootTemplate);
    const std::string filename = "中文删除.txt";
    const std::string previewFilename = "中文预览.txt";
    const std::string emptyDirectory = JoinPath(root, "empty-folder");
    const std::string nonemptyDirectory = JoinPath(root, "nonempty-folder");
    assert(mkdir(emptyDirectory.c_str(), 0777) == 0);
    assert(mkdir(nonemptyDirectory.c_str(), 0777) == 0);
    WriteFile(JoinPath(root, filename), "content");
    WriteFile(JoinPath(root, previewFilename), "第一行中文\nsecond line\nthird line");
    WriteFile(JoinPath(root, "sample.pkg"), "pkg-data");
    WriteFile(JoinPath(root, "small.txt"), "s");
    WriteFile(JoinPath(root, "large.txt"), "large-content");
    WriteFile(JoinPath(root, "photo.PNG"), "png-data");
    WriteFile(JoinPath(root, "archive.ZIP"), "zip-data");
    WriteFile(JoinPath(root, "boot.SELF"), "self-data");
    WriteFile(JoinPath(root, "plain"), "plain ascii text");
    const unsigned char binaryContent[] = { 0x01, 0x00, 0xff, 0x10 };
    WriteBytes(JoinPath(root, "binary.dat"), binaryContent, sizeof(binaryContent));
    WriteFile(JoinPath(nonemptyDirectory, "nested.txt"), "nested");

    FileBrowser browser;
    assert(browser.Open(root));
    assert(browser.Status() == BrowserStatus::Items);

    browser.MoveSelection(-1);
    assert(browser.SelectedIndex() == static_cast<int>(browser.Entries().size()) - 1);
    browser.MoveSelection(1);
    assert(browser.SelectedIndex() == 0);

    Select(browser, "photo.PNG");
    assert(browser.Selected()->type == FileType::Image);
    Select(browser, "archive.ZIP");
    assert(browser.Selected()->type == FileType::Archive);
    Select(browser, "boot.SELF");
    assert(browser.Selected()->type == FileType::Executable);
    Select(browser, "plain");
    assert(browser.Selected()->type == FileType::Text);

    Select(browser, "small.txt");
    browser.CycleSort(1);
    assert(browser.CurrentSort() == SortMode::Size);
    assert(EntryIndex(browser, "small.txt") >= 0);
    assert(EntryIndex(browser, "large.txt") >= 0);
    assert(EntryIndex(browser, "small.txt") < EntryIndex(browser, "large.txt"));
    browser.CycleSort(1);
    assert(browser.CurrentSort() == SortMode::Modified);
    browser.CycleSort(1);
    assert(browser.CurrentSort() == SortMode::Name);
    browser.CycleSort(-1);
    assert(browser.CurrentSort() == SortMode::Modified);

    assert(!browser.CreateDirectory("bad/name"));
    assert(browser.Status() == BrowserStatus::InvalidName);
    assert(browser.CreateDirectory("created-folder"));
    assert(browser.Status() == BrowserStatus::Created);
    assert(access(JoinPath(root, "created-folder").c_str(), F_OK) == 0);
    Select(browser, "created-folder");
    assert(browser.RenameSelected("renamed-folder"));
    assert(browser.Status() == BrowserStatus::Renamed);
    assert(access(JoinPath(root, "renamed-folder").c_str(), F_OK) == 0);
    assert(!browser.RenameSelected("bad/name"));
    assert(browser.Status() == BrowserStatus::InvalidName);
    WriteFile(JoinPath(root, "existing-name.txt"), "existing");
    assert(!browser.RenameSelected("existing-name.txt"));
    assert(browser.Status() == BrowserStatus::AlreadyExists);

    Select(browser, "empty-folder");
    assert(browser.OpenSelected());
    assert(browser.CurrentPath() == emptyDirectory);
    assert(browser.GoBack());
    assert(browser.CurrentPath() == root);

    Select(browser, previewFilename);
    assert(browser.Selected()->type == FileType::Text);
    assert(browser.Selected()->preview.find("第一行中文") != std::string::npos);
    assert(!browser.OpenSelected());
    assert(browser.Status() == BrowserStatus::PreviewAvailable);

    Select(browser, "sample.pkg");
    assert(browser.Selected()->type == FileType::Package);
    assert(browser.Selected()->preview.empty());

    Select(browser, "binary.dat");
    assert(browser.Selected()->type == FileType::Binary);
    assert(browser.Selected()->preview.empty());

    Select(browser, filename);
    browser.ToggleSelectedMark();
    assert(browser.HasMarkedEntries());
    assert(browser.MarkedCount() == 1);
    assert(browser.IsMarked(*browser.Selected()));
    assert(browser.MarkedEntries()[0].name == filename);
    browser.ToggleSelectedMark();
    assert(!browser.HasMarkedEntries());
    browser.ToggleSelectedMark();
    assert(browser.Refresh());
    assert(browser.HasMarkedEntries());
    assert(browser.MarkedEntries()[0].name == filename);

    Select(browser, filename);
    assert(browser.DeleteSelected());
    assert(browser.Status() == BrowserStatus::Deleted);
    assert(access(JoinPath(root, filename).c_str(), F_OK) != 0);
    assert(!browser.HasMarkedEntries());

    Select(browser, "empty-folder");
    assert(browser.DeleteSelected());
    assert(browser.Status() == BrowserStatus::Deleted);
    assert(access(emptyDirectory.c_str(), F_OK) != 0);

    Select(browser, "nonempty-folder");
    assert(!browser.DeleteSelected());
    assert(browser.Status() == BrowserStatus::OperationFailed);
    assert(access(nonemptyDirectory.c_str(), F_OK) == 0);

    printf("file-browser-host-tests-ok\n");
    return 0;
}
