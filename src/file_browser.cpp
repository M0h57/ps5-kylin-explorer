#include "file_browser.h"
#include "logger.h"
#include "utils.h"

#include <algorithm>
#include <chrono>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

namespace {
struct LocationCandidate {
    const char *label;
    const char *path;
};

const LocationCandidate kLocationCandidates[] = {
    { "ROOT", "/" },
    { "DATA", "/data" },
    { "USER", "/user" },
    { "USB 0", "/mnt/usb0" },
    { "USB 1", "/mnt/usb1" },
    { "APP", "/app0" },
};

const uint64_t kPreviewMaxFileSize = 64 * 1024;
const size_t kPreviewMaxBytes = 768;

bool IsAccessibleDirectory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

bool ContainsPath(const std::vector<std::string> &paths, const std::string &path) {
    return std::find(paths.begin(), paths.end(), path) != paths.end();
}

std::string LowerExtension(const std::string &name) {
    const size_t dot = name.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= name.size()) return "";
    std::string extension = name.substr(dot + 1);
    for (size_t index = 0; index < extension.size(); ++index) {
        extension[index] = static_cast<char>(tolower(static_cast<unsigned char>(extension[index])));
    }
    return extension;
}

bool IsKnownTextExtension(const std::string &extension) {
    return extension == "txt" || extension == "log" || extension == "ini" ||
           extension == "cfg" || extension == "json" || extension == "xml" ||
           extension == "csv" || extension == "md" || extension == "lua" ||
           extension == "sh" || extension == "c" || extension == "cpp" ||
           extension == "h" || extension == "hpp";
}

bool IsKnownImageExtension(const std::string &extension) {
    return extension == "png" || extension == "jpg" || extension == "jpeg" ||
           extension == "bmp" || extension == "gif" || extension == "webp";
}

bool IsKnownArchiveExtension(const std::string &extension) {
    return extension == "zip" || extension == "rar" || extension == "7z" ||
           extension == "tar" || extension == "gz";
}

bool LooksLikeText(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) return false;
    size_t printable = 0;
    for (size_t index = 0; index < bytes.size(); ++index) {
        const unsigned char value = bytes[index];
        if (value == 0) return false;
        if (value == '\n' || value == '\r' || value == '\t' || value >= 32 || value >= 0x80) {
            ++printable;
        }
    }
    return printable * 100 / bytes.size() >= 90;
}

std::vector<unsigned char> ReadPrefix(const std::string &path, size_t limit) {
    std::vector<unsigned char> bytes;
    FILE *file = fopen(path.c_str(), "rb");
    if (file == NULL) return bytes;
    bytes.resize(limit);
    const size_t read = fread(&bytes[0], 1, limit, file);
    fclose(file);
    bytes.resize(read);
    return bytes;
}

FileType DetectFileType(const FileEntry &entry, const std::vector<unsigned char> &prefix) {
    if (entry.kind == FileKind::Directory) return FileType::Directory;
    if (entry.kind == FileKind::Symlink) return FileType::Symlink;
    if (IsSpecialFile(entry.kind)) return FileType::Special;
    const std::string extension = LowerExtension(entry.name);
    if (IsKnownTextExtension(extension)) return FileType::Text;
    if (IsKnownImageExtension(extension)) return FileType::Image;
    if (extension == "pkg") return FileType::Package;
    if (extension == "elf" || extension == "self" || extension == "sprx" || extension == "prx" ||
        extension == "bin") {
        return FileType::Executable;
    }
    if (IsKnownArchiveExtension(extension)) return FileType::Archive;
    if (LooksLikeText(prefix)) return FileType::Text;
    if (!prefix.empty()) return FileType::Binary;
    return FileType::Unknown;
}

std::string PreviewFromPrefix(const std::vector<unsigned char> &prefix) {
    std::string bytes;
    bytes.reserve(prefix.size());
    for (size_t index = 0; index < prefix.size(); ++index) {
        const unsigned char value = prefix[index];
        if (value == '\r') continue;
        bytes.push_back(static_cast<char>(value));
    }
    return DisplayCodec::DecodeBytes(bytes).text;
}

}

FileBrowser::FileBrowser()
    : currentPath_("/"), currentLocationLabel_("ROOT"), selectedIndex_(0), locationIndex_(0),
      status_(BrowserStatus::Items), statusValue_(0), sortMode_(SortMode::Name) {
}

void FileBrowser::Initialize() {
    DiscoverLocations();
}

bool FileBrowser::Open(const std::string &path) {
    return OpenInternal(path, true);
}

bool FileBrowser::OpenInternal(const std::string &path, bool rememberHistory) {
    std::vector<FsDirectoryEntry> fsEntries;
    std::string fsError;
    if (!defaultFileSystem_.ListDirectory(FsPath(path), &fsEntries, &fsError)) {
        status_ = BrowserStatus::OpenFailed;
        return false;
    }

    std::vector<FileEntry> nextEntries;
    for (size_t fsIndex = 0; fsIndex < fsEntries.size(); ++fsIndex) {
        const FsDirectoryEntry &fsEntry = fsEntries[fsIndex];
        const std::string &name = fsEntry.name.Bytes();
        FileEntry entry;
        entry.name = name;
        entry.path = fsEntry.path.Bytes();
        entry.displayName = fsEntry.name.Display().text;
        entry.displayPath = fsEntry.path.Display().text;
        entry.preview.clear();
        entry.size = 0;
        entry.modifiedTime = 0;
        entry.mode = 0;
        entry.isDirectory = false;
        entry.displayNameLossy = fsEntry.name.Display().lossy;
        entry.displayPathLossy = fsEntry.path.Display().lossy;
        entry.kind = fsEntry.metadata.kind;
        entry.type = FileType::Unknown;

        entry.isDirectory = entry.kind == FileKind::Directory;
        entry.mode = fsEntry.metadata.mode;
        entry.modifiedTime = fsEntry.metadata.modifiedTime;
        if (!entry.isDirectory) {
            entry.size = fsEntry.metadata.size;
        }

        std::vector<unsigned char> prefix;
        if (entry.kind == FileKind::Regular && entry.size <= kPreviewMaxFileSize) {
            prefix = ReadPrefix(entry.path, kPreviewMaxBytes);
        }
        entry.type = DetectFileType(entry, prefix);
        if (entry.type == FileType::Text && !prefix.empty()) {
            entry.preview = PreviewFromPrefix(prefix);
        }
        nextEntries.push_back(entry);
    }

    if (rememberHistory && path != currentPath_) {
        history_.push_back(currentPath_);
    }
    entries_.swap(nextEntries);
    currentPath_ = path;
    selectedIndex_ = 0;
    markedPaths_.clear();
    UpdateCurrentLocation();
    SortEntries();

    statusValue_ = static_cast<int>(entries_.size());
    status_ = BrowserStatus::Items;
    return true;
}

void FileBrowser::SortEntries() {
    std::sort(entries_.begin(), entries_.end(), [this](const FileEntry &left, const FileEntry &right) {
        if (left.isDirectory != right.isDirectory) {
            return left.isDirectory > right.isDirectory;
        }
        if (sortMode_ == SortMode::Size && left.size != right.size) {
            return left.size < right.size;
        }
        if (sortMode_ == SortMode::Modified && left.modifiedTime != right.modifiedTime) {
            return left.modifiedTime > right.modifiedTime;
        }
        return left.name < right.name;
    });
}

bool FileBrowser::Refresh() {
    const FileEntry *selected = Selected();
    const std::string selectedPath = selected == NULL ? "" : selected->path;
    const std::vector<std::string> markedPaths = markedPaths_;
    if (!OpenInternal(currentPath_, false)) {
        return false;
    }
    markedPaths_ = markedPaths;
    PruneSelectionMarks();
    for (size_t index = 0; index < entries_.size(); ++index) {
        if (entries_[index].path == selectedPath) {
            selectedIndex_ = static_cast<int>(index);
            break;
        }
    }
    status_ = BrowserStatus::Refreshed;
    return true;
}

bool FileBrowser::OpenSelected() {
    const FileEntry *entry = Selected();
    if (entry == NULL) return false;
    if (entry->isDirectory) {
        return Open(entry->path);
    }
    if (entry->type == FileType::Text && !entry->preview.empty()) {
        status_ = BrowserStatus::PreviewAvailable;
        return false;
    }
    if (entry->type == FileType::Package) {
        // We return true but don't change directory.
        // The App will check the selected entry type and start the installer.
        return true;
    }
    return false;
}

bool FileBrowser::GoUp() {
    return Open(ParentPath(currentPath_));
}

bool FileBrowser::GoBack() {
    if (history_.empty()) {
        return GoUp();
    }
    const std::string path = history_.back();
    history_.pop_back();
    return OpenInternal(path, false);
}

bool FileBrowser::SwitchLocation(int delta) {
    if (locations_.empty()) {
        status_ = BrowserStatus::NoLocations;
        return false;
    }

    locationIndex_ += delta;
    if (locationIndex_ < 0) {
        locationIndex_ = static_cast<int>(locations_.size()) - 1;
    } else if (locationIndex_ >= static_cast<int>(locations_.size())) {
        locationIndex_ = 0;
    }
    return Open(locations_[locationIndex_].path);
}

void FileBrowser::MoveSelection(int delta) {
    if (entries_.empty()) {
        selectedIndex_ = 0;
        return;
    }
    selectedIndex_ += delta;
    if (selectedIndex_ < 0) {
        selectedIndex_ = static_cast<int>(entries_.size()) - 1;
    } else if (selectedIndex_ >= static_cast<int>(entries_.size())) {
        selectedIndex_ = 0;
    }
}

void FileBrowser::CycleSort(int delta) {
    int value = static_cast<int>(sortMode_) + delta;
    if (value < 0) {
        value = 2;
    } else if (value > 2) {
        value = 0;
    }
    const FileEntry *selected = Selected();
    const std::string selectedPath = selected == NULL ? "" : selected->path;
    sortMode_ = static_cast<SortMode>(value);
    SortEntries();
    for (size_t index = 0; index < entries_.size(); ++index) {
        if (entries_[index].path == selectedPath) {
            selectedIndex_ = static_cast<int>(index);
            break;
        }
    }
}

bool FileBrowser::CreateDirectory(const std::string &name) {
    if (!IsValidName(name)) {
        status_ = BrowserStatus::InvalidName;
        return false;
    }
    const std::string path = JoinPath(currentPath_, name);
    if (mkdir(path.c_str(), 0777) != 0) {
        LOG_ERROR("CreateDirectory failed for %s: %s (errno: %d)", path.c_str(), strerror(errno), errno);
        status_ = BrowserStatus::OperationFailed;
        return false;
    }
    Refresh();
    SelectPath(path);
    status_ = BrowserStatus::Created;
    return true;
}

bool FileBrowser::RenameSelected(const std::string &name) {
    const FileEntry *entry = Selected();
    if (entry == NULL || !IsValidName(name)) {
        status_ = BrowserStatus::InvalidName;
        return false;
    }
    const std::string oldPath = entry->path;
    const std::string newPath = JoinPath(currentPath_, name);
    if (oldPath == newPath) return true;
    
    struct stat st;
    if (stat(newPath.c_str(), &st) == 0) {
        status_ = BrowserStatus::AlreadyExists;
        return false;
    }
    
    if (rename(oldPath.c_str(), newPath.c_str()) != 0) {
        LOG_ERROR("RenameSelected failed from %s to %s: %s (errno: %d)", oldPath.c_str(), newPath.c_str(), strerror(errno), errno);
        status_ = BrowserStatus::OperationFailed;
        return false;
    }
    Refresh();
    SelectPath(newPath);
    status_ = BrowserStatus::Renamed;
    return true;
}

bool FileBrowser::DeleteSelected() {
    const FileEntry *entry = Selected();
    if (entry == NULL) {
        status_ = BrowserStatus::OperationFailed;
        return false;
    }
    
    int result = entry->kind == FileKind::Directory ? rmdir(entry->path.c_str()) : unlink(entry->path.c_str());
    if (result != 0) {
        LOG_ERROR("DeleteSelected failed for %s: %s (errno: %d)", entry->path.c_str(), strerror(errno), errno);
        status_ = BrowserStatus::OperationFailed;
        return false;
    }
    Refresh();
    status_ = BrowserStatus::Deleted;
    return true;
}

void FileBrowser::ToggleSelectedMark() {
    const FileEntry *entry = Selected();
    if (entry == NULL) return;
    std::vector<std::string>::iterator found = std::find(markedPaths_.begin(), markedPaths_.end(), entry->path);
    if (found == markedPaths_.end()) {
        markedPaths_.push_back(entry->path);
    } else {
        markedPaths_.erase(found);
    }
}

void FileBrowser::ClearSelectionMarks() {
    markedPaths_.clear();
}

void FileBrowser::ReportOperationFailed() {
    status_ = BrowserStatus::OperationFailed;
}

bool FileBrowser::IsValidName(const std::string &name) const {
    return FsName::IsValidBytes(name);
}

void FileBrowser::SelectPath(const std::string &path) {
    for (size_t index = 0; index < entries_.size(); ++index) {
        if (entries_[index].path == path) {
            selectedIndex_ = static_cast<int>(index);
            return;
        }
    }
}

void FileBrowser::PruneSelectionMarks() {
    std::vector<std::string> kept;
    for (size_t mark = 0; mark < markedPaths_.size(); ++mark) {
        for (size_t index = 0; index < entries_.size(); ++index) {
            if (entries_[index].path == markedPaths_[mark]) {
                kept.push_back(markedPaths_[mark]);
                break;
            }
        }
    }
    markedPaths_.swap(kept);
}

void FileBrowser::DiscoverLocations() {
    locations_.clear();
    for (size_t index = 0; index < sizeof(kLocationCandidates) / sizeof(kLocationCandidates[0]); ++index) {
        if (IsAccessibleDirectory(kLocationCandidates[index].path)) {
            BrowserLocation location;
            location.label = kLocationCandidates[index].label;
            location.path = kLocationCandidates[index].path;
            locations_.push_back(location);
        }
    }
    UpdateCurrentLocation();
}

void FileBrowser::UpdateCurrentLocation() {
    currentLocationLabel_ = "CUSTOM";
    for (size_t index = 0; index < locations_.size(); ++index) {
        if (IsPathWithin(currentPath_, locations_[index].path)) {
            currentLocationLabel_ = locations_[index].label;
        }
        if (currentPath_ == locations_[index].path) {
            locationIndex_ = static_cast<int>(index);
            return;
        }
    }
}

const std::string &FileBrowser::CurrentPath() const { return currentPath_; }
const std::string &FileBrowser::CurrentLocationLabel() const { return currentLocationLabel_; }
BrowserStatus FileBrowser::Status() const { return status_; }
int FileBrowser::StatusValue() const { return statusValue_; }
SortMode FileBrowser::CurrentSort() const { return sortMode_; }
const std::vector<FileEntry> &FileBrowser::Entries() const { return entries_; }
const std::vector<BrowserLocation> &FileBrowser::Locations() const { return locations_; }
const FileEntry *FileBrowser::Selected() const {
    return entries_.empty() ? NULL : &entries_[selectedIndex_];
}
int FileBrowser::SelectedIndex() const { return selectedIndex_; }
bool FileBrowser::IsMarked(const FileEntry &entry) const {
    return ContainsPath(markedPaths_, entry.path);
}
bool FileBrowser::HasMarkedEntries() const { return !markedPaths_.empty(); }
int FileBrowser::MarkedCount() const { return static_cast<int>(markedPaths_.size()); }
std::vector<FileEntry> FileBrowser::MarkedEntries() const {
    std::vector<FileEntry> marked;
    for (size_t mark = 0; mark < markedPaths_.size(); ++mark) {
        for (size_t index = 0; index < entries_.size(); ++index) {
            if (entries_[index].path == markedPaths_[mark]) {
                marked.push_back(entries_[index]);
                break;
            }
        }
    }
    return marked;
}
