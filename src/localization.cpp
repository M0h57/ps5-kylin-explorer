#include "localization.h"

namespace {
struct Translation {
    const char *english;
    const char *chinese;
};

const Translation kTranslations[] = {
    { "GUARDED WRITES", "受保护写入" },
    { "LOCATION", "位置" },
    { "PATH", "路径" },
    { "DETAILS", "详细信息" },
    { "No items in this directory", "此目录没有内容" },
    { "Directory", "目录" },
    { "File", "文件" },
    { "Text", "文本" },
    { "Image", "图片" },
    { "Package", "安装包" },
    { "Archive", "压缩包" },
    { "Executable", "可执行文件" },
    { "Binary", "二进制" },
    { "Unknown", "未知" },
    { "SIZE", "大小" },
    { "MODIFIED", "修改时间" },
    { "MODE", "权限" },
    { "MILESTONE  0.62", "里程碑  0.62" },
    { "Guarded writes enabled", "受保护写入已启用" },
    { "X Open  Square Mark  O Back  Triangle Menu  Options Refresh  Left/Right Sort  L2 Fast scroll", "X 打开  Square 标记  O 返回  Triangle 菜单  Options 刷新  左右 排序  L2 快速滚动" },
    { "X Select  O Close  D-pad Navigate", "X 选择  O 关闭  方向键 导航" },
    { "Open", "打开" },
    { "Mark", "标记" },
    { "Back", "返回" },
    { "Menu", "菜单" },
    { "Refresh", "刷新" },
    { "Sort", "排序" },
    { "Fast scroll", "快速滚动" },
    { "Select", "选择" },
    { "Close", "关闭" },
    { "Navigate", "导航" },
    { "Settings", "设置" },
    { "Switch", "切换" },
    { "Action", "执行" },
    { "Save & Exit", "保存并退出" },
    { "Cancel", "取消" },
    { "Replace", "覆盖" },
    { "Delete", "删除" },
    { "Delete tree", "递归删除" },
    { "CONTEXT MENU", "上下文菜单" },
    { "Refresh", "刷新" },
    { "Sort", "排序" },
    { "Properties", "属性" },
    { "New folder", "新建文件夹" },
    { "Rename", "重命名" },
    { "Copy", "复制" },
    { "Paste", "粘贴" },
    { "Delete", "删除" },
    { "Name", "名称" },
    { "Size", "大小" },
    { "Modified", "修改时间" },
    { "items", "个项目" },
    { "Unable to open directory", "无法打开目录" },
    { "Directory is empty", "目录为空" },
    { "Preview is planned for a later milestone", "文件预览将在后续版本提供" },
    { "Preview is shown in the details panel", "预览已显示在详情栏" },
    { "No accessible shortcut locations", "没有可访问的快捷位置" },
    { "Directory refreshed", "目录已刷新" },
    { "Planned for a later milestone", "此功能将在后续版本提供" },
    { "planned", "待实现" },
    { "Directory created", "目录已创建" },
    { "Item renamed", "项目已重命名" },
    { "Invalid name", "名称无效" },
    { "An item with this name already exists", "已存在同名项目" },
    { "Operation failed", "操作失败" },
    { "New folder", "新建文件夹" },
    { "Enter a folder name", "输入文件夹名称" },
    { "Rename", "重命名" },
    { "Enter a new name", "输入新名称" },
    { "PROPERTIES", "属性" },
    { "O Close", "O 关闭" },
    { "ERROR", "错误" },
    { "X or O Close", "X 或 O 关闭" },
    { "Copied to clipboard", "已复制到剪贴板" },
    { "Copy is unavailable for this item", "无法复制此项目" },
    { "COPYING", "正在复制" },
    { "O Cancel", "O 取消" },
    { "Copy completed", "复制完成" },
    { "Copy canceled", "复制已取消" },
    { "Copy failed", "复制失败" },
    { "OVERWRITE FILE?", "覆盖文件？" },
    { "A file with the same name already exists.", "已存在同名文件。" },
    { "X Replace", "X 覆盖" },
    { "O Cancel", "O 取消" },
    { "DELETE ITEM?", "删除项目？" },
    { "Files and empty directories can be deleted.", "可删除文件和空目录。" },
    { "X Delete", "X 删除" },
    { "O Cancel", "O 取消" },
    { "Item deleted", "项目已删除" },
    { "Cut", "剪切" },
    { "MOVING", "正在移动" },
    { "Move completed", "移动完成" },
    { "Move canceled", "移动已取消" },
    { "Move failed", "移动失败" },
    { "DELETE DIRECTORY TREE?", "递归删除目录？" },
    { "All nested items will be permanently deleted.", "目录内所有项目将被永久删除。" },
    { "X Delete tree", "X 递归删除" },
    { "O Cancel", "O 取消" },
    { "DELETING", "正在删除" },
    { "Delete completed", "删除完成" },
    { "Delete failed", "删除失败" },
    { "Please wait", "请稍候" },
    { "LOADING", "正在加载" },
    { "Waiting for jailbreak privileges...", "正在等待越狱权限..." },
    { "Kylin Core will grant filesystem access automatically.", "Kylin Core 将自动授予文件系统访问权限。" },
    { "RECENT OPERATIONS", "最近操作" },
    { "No recorded operations", "暂无操作记录" },
    { "O Close", "O 关闭" },
    { "New folder", "新建文件夹" },
    { "Rename", "重命名" },
    { "Copy", "复制" },
    { "Move", "移动" },
    { "Delete", "删除" },
    { "OK", "成功" },
    { "FAILED", "失败" },
    { "selected", "已选择" },
    { "Marked items are used by copy, cut, and delete.", "复制、剪切和删除会作用于已标记项目。" },
    { "PREVIEW", "预览" },
    { "INSTALLED PKGS", "已安装 PKG 列表" },
    { "No installed packages found", "未找到已安装的 PKG" },
    { "O Close", "O 关闭" },
    { "TITLE ID", "TITLE ID" },
    { "CONTENT ID", "CONTENT ID" },
    { "CPU", "CPU" },
    { "USB", "USB" },
    { "SYSTEM SETTINGS & UTILITIES", "系统设置与工具箱" },
    { "Application Language", "应用语言" },
    { "Temperature Display Unit", "温度显示单位" },
    { "Installed PKGs Dashboard", "已安装 PKG 仪表盘" },
    { "Recent Operations Log", "最近操作日志列表" },
    { "D-pad Up/Down: Navigate  Left/Right: Switch  X: Action  O: Save & Exit", "方向键 上下: 移动  左右: 切换  X: 选项执行  O: 保存并退出" },
    { "Enabled", "已开启" },
    { "Disabled", "已关闭" },
    { "Show", "显示" },
    { "Hide", "隐藏" },
    { "Celsius (°C)", "摄氏度 (°C)" },
    { "Fahrenheit (°F)", "华氏度 (°F)" },
    { "Press X to Open", "按 X 键打开" },
    { "KYLIN EXPLORER", "麒麟浏览器" },
    { "Quick Places", "快捷位置" },
    { "QUICK PLACES", "快捷位置" },
    { "Jump", "跳转" },
    { "No external devices found", "未找到外部设备" },
    { "Theme", "界面主题" },
    { "Kylin", "麒麟" },
    { "Night", "夜黑" },
};

static_assert(sizeof(kTranslations) / sizeof(kTranslations[0]) == static_cast<int>(TextId::Count),
              "TextId translation table must cover every text id");
}

Localizer::Localizer() : language_(Language::English) {
}

const char *Localizer::Get(TextId id) const {
    const Translation &translation = kTranslations[static_cast<int>(id)];
    return language_ == Language::SimplifiedChinese ? translation.chinese : translation.english;
}

const char *Localizer::LanguageName() const {
    return language_ == Language::SimplifiedChinese ? "简体中文" : "English";
}

void Localizer::ToggleLanguage() {
    language_ = language_ == Language::English ? Language::SimplifiedChinese : Language::English;
}

void Localizer::SetLanguage(Language lang) {
    language_ = lang;
}

Language Localizer::CurrentLanguage() const {
    return language_;
}
