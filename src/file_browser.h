#ifndef KYLIN_EXPLORER_FILE_BROWSER_H
#define KYLIN_EXPLORER_FILE_BROWSER_H

#include <stdint.h>
#include <string>
#include <vector>
#include "file_system.h"

enum class FileType {
    Directory,
    Text,
    Image,
    Package,
    Archive,
    Executable,
    Binary,
    Symlink,
    Special,
    Unknown,
};

struct FileEntry {
    // name/path are the original POSIX bytes. Use displayName/displayPath for UI text.
    std::string name;
    std::string path;
    std::string displayName;
    std::string displayPath;
    std::string preview;
    uint64_t size;
    uint64_t modifiedTime;
    uint32_t mode;
    bool isDirectory;
    bool displayNameLossy;
    bool displayPathLossy;
    FileKind kind;
    FileType type;
};

struct BrowserLocation {
    std::string label;
    std::string path;
};

enum class BrowserStatus {
    Items,
    OpenFailed,
    Empty,
    PreviewLater,
    PreviewAvailable,
    NoLocations,
    Refreshed,
    Created,
    Renamed,
    Deleted,
    InvalidName,
    AlreadyExists,
    OperationFailed,
};

enum class SortMode {
    Name,
    Size,
    Modified,
};

class FileBrowser {
public:
    FileBrowser();
    void Initialize();
    bool Open(const std::string &path);
    bool Refresh();
    bool OpenSelected();
    bool GoUp();
    bool GoBack();
    bool SwitchLocation(int delta);
    void MoveSelection(int delta);
    void CycleSort(int delta);
    bool CreateDirectory(const std::string &name);
    bool RenameSelected(const std::string &name);
    bool DeleteSelected();
    void ToggleSelectedMark();
    void ClearSelectionMarks();
    void ReportOperationFailed();

    const std::string &CurrentPath() const;
    const std::string &CurrentLocationLabel() const;
    BrowserStatus Status() const;
    int StatusValue() const;
    SortMode CurrentSort() const;
    const std::vector<FileEntry> &Entries() const;
    const std::vector<BrowserLocation> &Locations() const;
    const FileEntry *Selected() const;
    int SelectedIndex() const;
    bool IsMarked(const FileEntry &entry) const;
    bool HasMarkedEntries() const;
    int MarkedCount() const;
    std::vector<FileEntry> MarkedEntries() const;
private:
    bool OpenInternal(const std::string &path, bool rememberHistory);
    void SortEntries();
    bool IsValidName(const std::string &name) const;
    void SelectPath(const std::string &path);
    void PruneSelectionMarks();
    void DiscoverLocations();
    void UpdateCurrentLocation();

    std::string currentPath_;
    std::string currentLocationLabel_;
    std::vector<FileEntry> entries_;
    std::vector<std::string> markedPaths_;
    std::vector<BrowserLocation> locations_;
    std::vector<std::string> history_;
    int selectedIndex_;
    int locationIndex_;
    BrowserStatus status_;
    int statusValue_;
    SortMode sortMode_;
    PosixFileSystem defaultFileSystem_;
};

#endif
