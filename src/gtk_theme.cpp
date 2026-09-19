#include "gtk_theme.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <unordered_map>

using Section = std::unordered_map<std::string, std::string>;
using IniFile = std::unordered_map<std::string, Section>;

static std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static std::optional<IniFile> read_ini(const fs::path& path) {
    std::ifstream input(path);

    if (!input) {
        return std::nullopt;
    }

    IniFile file;

    std::string section;
    std::string line;

    while (std::getline(input, line)) {
        line = trim(std::move(line));

        if (line.empty() || line.starts_with('#') || line.starts_with(';')) {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }

        const auto separator = line.find('=');
        if (section.empty() || separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        if (!key.empty()) {
            file[section][key] = trim(line.substr(separator + 1));
        }
    }

    return file;
}

static const std::string* ini_value(const IniFile& file, std::string_view section, std::string_view key) {
    const auto section_it = file.find(std::string(section));
    if (section_it == file.end()) {
        return nullptr;
    }
    const auto value_it = section_it->second.find(std::string(key));
    return value_it == section_it->second.end() ? nullptr : &value_it->second;
}

static void set_if_present(std::string& target, const IniFile& file, std::string_view section, std::string_view key) {
    const std::string* setting = ini_value(file, section, key);
    if (setting != nullptr && !setting->empty()) {
        target = *setting;
    }
}

static int integer_or(std::string_view value, int fallback) {
    int parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return result.ec == std::errc() && result.ptr == value.data() + value.size() ? parsed : fallback;
}

static int integer_value(const IniFile& file, std::string_view section, std::string_view key, int fallback) {
    const std::string* setting = ini_value(file, section, key);
    return setting == nullptr ? fallback : integer_or(*setting, fallback);
}

static std::vector<std::string> split_list(std::string_view value) {
    std::vector<std::string> result;
    std::size_t start = 0;

    while (start <= value.size()) {
        const std::size_t end = value.find(',', start);
        const std::string item = trim(std::string(value.substr(start, end - start)));

        if (!item.empty()) result.push_back(item);
        if (end == std::string_view::npos) break;

        start = end + 1;
    }

    return result;
}

static std::string environment(std::string_view name) {
    const char* value = std::getenv(std::string(name).c_str());
    return value == nullptr ? std::string{} : std::string(value);
}

static fs::path home_directory() {
    return environment("HOME");
}

static fs::path xdg_data_home() {
    const std::string configured = environment("XDG_DATA_HOME");
    return configured.empty() ? home_directory() / ".local/share" : fs::path(configured);
}

static fs::path xdg_config_home() {
    const std::string configured = environment("XDG_CONFIG_HOME");
    return configured.empty() ? home_directory() / ".config" : fs::path(configured);
}

static std::vector<fs::path> xdg_data_directories() {
    const std::string configured = environment("XDG_DATA_DIRS");
    const std::string directories = configured.empty() ? "/usr/local/share:/usr/share" : configured;

    std::vector<fs::path> result;
    std::size_t start = 0;

    while (start <= directories.size()) {
        const std::size_t end = directories.find(':', start);
        const std::string entry = directories.substr(start, end - start);

        if (!entry.empty()) result.emplace_back(entry);
        if (end == std::string::npos) break;

        start = end + 1;
    }

    return result;
}

static bool safe_relative_path(const fs::path& path) {
    if (path.empty() || path.is_absolute()) return false;

    return std::none_of(path.begin(), path.end(), [](const fs::path& component) { return component == ".."; });
}

static const char* extension(IconFormat format) {
    if (format == IconFormat::Png) return ".png";
    if (format == IconFormat::Svg) return ".svg";
    return ".xpm";
}

static std::optional<fs::path> icon_file(const fs::path& directory, const std::string& name, std::optional<IconFormat> format) {
    const auto find = [&](const char* extension) -> std::optional<fs::path> {
        const fs::path candidate = directory / (name + extension);
        return fs::is_regular_file(candidate) ? std::optional(candidate) : std::nullopt;
    };

    if (format) return find(extension(*format));

    for (const IconFormat candidate_format : {IconFormat::Png, IconFormat::Svg, IconFormat::Xpm}) {
        if (const auto candidate = find(extension(candidate_format))) return candidate;
    }
    return std::nullopt;
}

static bool matches_size(const IconDirectory& directory, int size, int scale) {
    if (directory.scale != scale) return false;
    if (directory.type == IconDirectoryType::Fixed) return directory.size == size;
    if (directory.type == IconDirectoryType::Scalable) return directory.min_size <= size && size <= directory.max_size;

    return directory.size - directory.threshold <= size && size <= directory.size + directory.threshold;
}

static int size_distance(const IconDirectory& directory, int size, int scale) {
    const int requested = size * scale;

    if (directory.type == IconDirectoryType::Fixed) {
        return std::abs(directory.size * directory.scale - requested);
    }

    const bool is_scalable = directory.type == IconDirectoryType::Scalable;
    const int minimum = (is_scalable ? directory.min_size : directory.size - directory.threshold) * directory.scale;
    const int maximum = (is_scalable ? directory.max_size : directory.size + directory.threshold) * directory.scale;

    if (requested < minimum) {
        return minimum - requested;
    }

    return requested > maximum ? requested - maximum : 0;
}

static std::string normalized_icon_name(std::string_view icon_name) {
    fs::path path(icon_name);
    if (!safe_relative_path(path) || path.has_parent_path()) {
        return {};
    }

    const std::string extension = path.extension().string();
    if (extension == ".png" || extension == ".svg" || extension == ".xpm") {
        path.replace_extension();
    }

    return path.string();
}

std::vector<fs::path> GtkTheme::search_paths() {
    std::vector<fs::path> paths;
    const fs::path home = home_directory();

    if (!home.empty()) {
        paths.push_back(home / ".themes");
    }

    paths.push_back(xdg_data_home() / "themes");
    for (const fs::path& directory : xdg_data_directories()) {
        paths.push_back(directory / "themes");
    }

    return paths;
}

std::optional<GtkTheme> GtkTheme::load(std::string_view theme_name, const std::vector<fs::path>& paths) {
    if (theme_name.empty() || !safe_relative_path(fs::path(theme_name))) {
        return std::nullopt;
    }

    for (const fs::path& base_path : paths) {
        const fs::path theme_path = base_path / theme_name;
        if (!fs::is_directory(theme_path)) {
            continue;
        }

        GtkTheme theme;
        theme.m_name = theme_name;
        theme.m_path = theme_path;

        const auto index = read_ini(theme_path / "index.theme");
        if (!index) {
            return theme;
        }

        set_if_present(theme.m_name, *index, "Desktop Entry", "Name");
        set_if_present(theme.m_comment, *index, "Desktop Entry", "Comment");
        set_if_present(theme.m_icon_theme_name, *index, "X-GNOME-Metatheme", "IconTheme");
        set_if_present(theme.m_cursor_theme_name, *index, "X-GNOME-Metatheme", "CursorTheme");
        set_if_present(theme.m_button_layout, *index, "X-GNOME-Metatheme", "ButtonLayout");

        return theme;
    }

    return std::nullopt;
}

const std::string& GtkTheme::name() const {
    return m_name;
}
const std::string& GtkTheme::comment() const {
    return m_comment;
}
const std::string& GtkTheme::icon_theme_name() const {
    return m_icon_theme_name;
}
const std::string& GtkTheme::cursor_theme_name() const {
    return m_cursor_theme_name;
}
const std::string& GtkTheme::button_layout() const {
    return m_button_layout;
}
const fs::path& GtkTheme::path() const {
    return m_path;
}

std::optional<GtkTheme> GtkTheme::load_current() {
    const GtkSettings settings = current_settings();
    return settings.theme_name.empty() ? std::nullopt : load(settings.theme_name);
}

std::optional<fs::path> GtkTheme::stylesheet(unsigned int gtk_major) const {
    const fs::path stylesheet = m_path / ("gtk-" + std::to_string(gtk_major) + ".0") / "gtk.css";
    return fs::is_regular_file(stylesheet) ? std::optional(stylesheet) : std::nullopt;
}

std::vector<fs::path> IconTheme::search_paths() {
    std::vector<fs::path> paths;
    const fs::path home = home_directory();
    if (!home.empty()) {
        paths.push_back(home / ".icons");
    }
    paths.push_back(xdg_data_home() / "icons");
    for (const fs::path& directory : xdg_data_directories()) {
        paths.push_back(directory / "icons");
    }
    paths.emplace_back("/usr/share/pixmaps");
    return paths;
}

std::optional<IconTheme> IconTheme::load(std::string_view theme_name, const std::vector<fs::path>& paths) {
    if (theme_name.empty() || !safe_relative_path(fs::path(theme_name))) {
        return std::nullopt;
    }

    for (const fs::path& base_path : paths) {
        const auto index = read_ini(base_path / theme_name / "index.theme");
        if (!index) {
            continue;
        }

        const std::string* directories = ini_value(*index, "Icon Theme", "Directories");
        if (directories == nullptr) {
            return std::nullopt;
        }

        IconTheme theme;
        theme.m_id = theme_name;
        theme.m_name = theme_name;
        theme.m_search_paths = paths;
        set_if_present(theme.m_name, *index, "Icon Theme", "Name");
        set_if_present(theme.m_comment, *index, "Icon Theme", "Comment");
        if (const std::string* inherits = ini_value(*index, "Icon Theme", "Inherits")) {
            theme.m_inherits = split_list(*inherits);
        }

        std::vector<std::string> names = split_list(*directories);
        if (const std::string* scaled = ini_value(*index, "Icon Theme", "ScaledDirectories")) {
            const std::vector<std::string> scaled_names = split_list(*scaled);
            names.insert(names.end(), scaled_names.begin(), scaled_names.end());
        }

        for (const std::string& name : names) {
            const fs::path path(name);
            if (!safe_relative_path(path)) {
                continue;
            }

            const int size = integer_value(*index, name, "Size", 0);
            if (size <= 0) {
                continue;
            }

            IconDirectory directory;
            directory.path = path;
            directory.size = size;
            directory.scale = std::max(1, integer_value(*index, name, "Scale", 1));
            directory.min_size = integer_value(*index, name, "MinSize", size);
            directory.max_size = integer_value(*index, name, "MaxSize", size);
            directory.threshold = std::max(0, integer_value(*index, name, "Threshold", 2));
            if (const std::string* context = ini_value(*index, name, "Context")) {
                directory.context = *context;
            }
            if (const std::string* type = ini_value(*index, name, "Type")) {
                directory.type = *type == "Fixed"      ? IconDirectoryType::Fixed
                                 : *type == "Scalable" ? IconDirectoryType::Scalable
                                                       : IconDirectoryType::Threshold;
            }
            theme.m_directories.push_back(std::move(directory));
        }
        return theme;
    }
    return std::nullopt;
}

std::optional<IconTheme> IconTheme::load_current() {
    const GtkSettings settings = current_settings();
    return settings.icon_theme_name.empty() ? std::nullopt : load(settings.icon_theme_name);
}

const std::string& IconTheme::name() const {
    return m_name;
}

const std::string& IconTheme::comment() const {
    return m_comment;
}

const std::vector<std::string>& IconTheme::inherits() const {
    return m_inherits;
}

const std::vector<IconDirectory>& IconTheme::directories() const {
    return m_directories;
}

std::optional<fs::path>
IconTheme::find_icon(std::string_view icon_name, int size, int scale, std::optional<IconFormat> format) const {
    const std::string name = normalized_icon_name(icon_name);
    if (name.empty() || size <= 0 || scale <= 0) {
        return std::nullopt;
    }

    std::vector<std::string> visited;
    if (const auto icon = find_in_theme(name, size, scale, format, visited)) {
        return icon;
    }

    // hicolor is the required final fallback after the inheritance tree.
    if (std::find(visited.begin(), visited.end(), "hicolor") == visited.end()) {
        if (const auto hicolor = load("hicolor", m_search_paths)) {
            if (const auto icon = hicolor->find_in_theme(name, size, scale, format, visited)) {
                return icon;
            }
        }
    }
    return find_fallback(name, format);
}

std::optional<fs::path> IconTheme::find_in_theme(
    const std::string& icon_name, int size, int scale, std::optional<IconFormat> format, std::vector<std::string>& visited
) const {
    if (std::find(visited.begin(), visited.end(), m_id) != visited.end()) {
        return std::nullopt;
    }
    visited.push_back(m_id);

    if (const auto icon = find_in_directories(icon_name, size, scale, format)) {
        return icon;
    }
    for (const std::string& parent_name : m_inherits) {
        const auto parent = load(parent_name, m_search_paths);
        if (parent) {
            if (const auto icon = parent->find_in_theme(icon_name, size, scale, format, visited)) {
                return icon;
            }
        }
    }
    return std::nullopt;
}

std::optional<fs::path>
IconTheme::find_in_directories(const std::string& icon_name, int size, int scale, std::optional<IconFormat> format) const {
    // an exact match in this theme wins over a closer icon in an inherited theme.
    for (const IconDirectory& directory : m_directories) {
        if (!matches_size(directory, size, scale)) {
            continue;
        }
        for (const fs::path& base_path : m_search_paths) {
            if (const auto icon = icon_file(base_path / m_id / directory.path, icon_name, format)) {
                return icon;
            }
        }
    }

    int closest_distance = std::numeric_limits<int>::max();
    std::optional<fs::path> closest;

    for (const IconDirectory& directory : m_directories) {
        const int distance = size_distance(directory, size, scale);
        if (distance >= closest_distance) {
            continue;
        }
        for (const fs::path& base_path : m_search_paths) {
            if (const auto icon = icon_file(base_path / m_id / directory.path, icon_name, format)) {
                closest = icon;
                closest_distance = distance;
                break;
            }
        }
    }

    return closest;
}

std::optional<fs::path> IconTheme::find_fallback(const std::string& icon_name, std::optional<IconFormat> format) const {
    for (const fs::path& path : m_search_paths) {
        if (const auto icon = icon_file(path, icon_name, format)) {
            return icon;
        }
    }
    return std::nullopt;
}

GtkSettings current_settings() {
    GtkSettings settings;

    for (const char* version : {"gtk-3.0", "gtk-4.0"}) {
        const auto file = read_ini(xdg_config_home() / version / "settings.ini");
        if (!file) {
            continue;
        }

        set_if_present(settings.theme_name, *file, "Settings", "gtk-theme-name");
        set_if_present(settings.icon_theme_name, *file, "Settings", "gtk-icon-theme-name");
        set_if_present(settings.cursor_theme_name, *file, "Settings", "gtk-cursor-theme-name");
        set_if_present(settings.font_name, *file, "Settings", "gtk-font-name");
        settings.cursor_size = integer_value(*file, "Settings", "gtk-cursor-theme-size", settings.cursor_size);
    }

    const std::string override = environment("GTK_THEME");
    if (!override.empty()) {
        const std::size_t separator = override.find(':');
        settings.theme_name = override.substr(0, separator);
        settings.theme_variant = separator == std::string::npos ? std::string{} : override.substr(separator + 1);
    }

    return settings;
}
