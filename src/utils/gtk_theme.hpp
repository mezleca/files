#pragma once

#include "./common.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// gtk theme resolver
// kinda limited but should be enough to get icons n shit

/**
 * gtk settings resolved from gtk-3.0 and gtk-4.0 settings.ini files.
 */
struct GtkSettings {
    std::string theme_name;
    std::string theme_variant;
    std::string icon_theme_name;
    std::string cursor_theme_name;
    std::string font_name;
    int cursor_size = 0;
};

/**
 * metadata and stylesheet paths for a gtk theme directory.
 */
class GtkTheme {
public:
  /** returns ~/.themes, xdg data theme directories, then system theme
     * directories. */
    static std::vector<fs::path> search_paths();

  /** loads a theme directory and its optional index.theme metadata. */
    static std::optional<GtkTheme> load(std::string_view theme_name, const std::vector<fs::path>& paths = search_paths());

  /** loads the gtk theme selected by current_settings(). */
    static std::optional<GtkTheme> load_current();

    const std::string& name() const;
    const std::string& comment() const;
    const std::string& icon_theme_name() const;
    const std::string& cursor_theme_name() const;
    const std::string& button_layout() const;
    const fs::path& path() const;

  /** returns gtk-<major>.0/gtk.css when that stylesheet exists. */
    std::optional<fs::path> stylesheet(unsigned int gtk_major) const;

private:
    std::string m_name;
    std::string m_comment;
    std::string m_icon_theme_name;
    std::string m_cursor_theme_name;
    std::string m_button_layout;
    fs::path m_path;
};

/**
 * size behaviour declared by an icon theme directory.
 * Fixed accepts one nominal size, Scalable accepts a range, and Threshold
 * accepts sizes within threshold pixels of its nominal size.
 */
enum class IconDirectoryType {
    Fixed,
    Scalable,
    Threshold,
};

/** the image formats accepted by the icon theme specification. */
enum class IconFormat {
    Png,
    Svg,
    Xpm,
};

/**
 * a directory declared by Directories or ScaledDirectories in index.theme.
 * context may be Actions, Animations, Applications, Categories, Devices,
 * Emblems, Emotes, FileSystems, International, MimeTypes, Places, Status,
 * or Stock as defined by the icon theme spec.
 */
struct IconDirectory {
    fs::path path;
    std::string context;
    IconDirectoryType type = IconDirectoryType::Threshold;
    int size = 0;
    int scale = 1;
    int min_size = 0;
    int max_size = 0;
    int threshold = 2;
};

/**
 * a freedesktop icon theme loaded from index.theme.
 * lookup supports the png, svg, and xpm formats required by the spec.
 */
class IconTheme {
public:
  /** returns the freedesktop icon base directories and /usr/share/pixmaps. */
    static std::vector<fs::path> search_paths();

  /** loads the first index.theme found for theme_name in paths. */
    static std::optional<IconTheme> load(std::string_view theme_name, const std::vector<fs::path>& paths = search_paths());

  /** loads the icon theme selected by current_settings(). */
    static std::optional<IconTheme> load_current();

    const std::string& name() const;
    const std::string& comment() const;
    const std::vector<std::string>& inherits() const;
    const std::vector<IconDirectory>& directories() const;
  /**
     * resolves icon_name for its nominal size and display scale.
     * format limits the result to one image format when provided.
     * checks the theme, its inherited themes, hicolor, then unthemed icons.
     */
    std::optional<fs::path>
    find_icon(std::string_view icon_name, int size, int scale = 1, std::optional<IconFormat> format = std::nullopt) const;

private:
    std::string m_id;
    std::string m_name;
    std::string m_comment;
    std::vector<std::string> m_inherits;
    std::vector<IconDirectory> m_directories;
    std::vector<fs::path> m_search_paths;

    std::optional<fs::path> find_in_theme(
        const std::string& icon_name, int size, int scale, std::optional<IconFormat> format, std::vector<std::string>& visited
    ) const;
    std::optional<fs::path>
    find_in_directories(const std::string& icon_name, int size, int scale, std::optional<IconFormat> format) const;
    std::optional<fs::path> find_fallback(const std::string& icon_name, std::optional<IconFormat> format) const;
};

/** reads the effective gtk settings available without linking against gtk or
 * glib. */
GtkSettings current_settings();
