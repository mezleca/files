#include "directory_entry.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <ui/runtime.hpp>
#include <ui/ui.hpp>
#include <ui/widgets/image.hpp>
#include <ui/widgets/text.hpp>
#include <utility>

using namespace ui;

static std::string_view get_icon_name(const fs::path& entry) {
    if (fs::is_directory(entry)) {
        return "folder";
    }

    const auto status = fs::status(entry);
    const auto perms = status.permissions();

    if ((perms & fs::perms::owner_exec) != fs::perms::none) {
        return "application-x-executable";
    }

    std::string extension = entry.extension().string();
    for (char& character : extension) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }

    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".gif" || extension == ".bmp" ||
        extension == ".webp" || extension == ".svg" || extension == ".avif" || extension == ".tif" || extension == ".tiff" ||
        extension == ".ico") {
        return "image-x-generic";
    }

    if (extension == ".mp3" || extension == ".flac" || extension == ".wav" || extension == ".ogg" || extension == ".oga" ||
        extension == ".opus" || extension == ".m4a" || extension == ".aac") {
        return "audio-x-generic";
    }

    if (extension == ".mp4" || extension == ".mkv" || extension == ".avi" || extension == ".mov" || extension == ".webm" ||
        extension == ".mpeg" || extension == ".mpg" || extension == ".ogv") {
        return "video-x-generic";
    }

    if (extension == ".zip" || extension == ".tar" || extension == ".gz" || extension == ".bz2" || extension == ".xz" ||
        extension == ".7z" || extension == ".rar" || extension == ".zst" || extension == ".deb" || extension == ".rpm" ||
        extension == ".iso") {
        return "package-x-generic";
    }

    if (extension == ".ttf" || extension == ".otf" || extension == ".woff" || extension == ".woff2") {
        return "font-x-generic";
    }

    if (extension == ".html" || extension == ".htm") {
        return "text-html";
    }

    if (extension == ".sh" || extension == ".bash" || extension == ".zsh" || extension == ".fish" || extension == ".py" ||
        extension == ".rb" || extension == ".pl" || extension == ".lua" || extension == ".js" || extension == ".ts") {
        return "text-x-script";
    }

    if (extension == ".pdf" || extension == ".doc" || extension == ".docx" || extension == ".odt" || extension == ".rtf" ||
        extension == ".tex" || extension == ".epub") {
        return "x-office-document";
    }

    if (extension == ".xls" || extension == ".xlsx" || extension == ".ods" || extension == ".csv") {
        return "x-office-spreadsheet";
    }

    if (extension == ".ppt" || extension == ".pptx" || extension == ".odp") {
        return "x-office-presentation";
    }

    if (extension == ".ics") {
        return "x-office-calendar";
    }

    if (extension == ".vcf") {
        return "x-office-address-book";
    }

    if (extension == ".so" || extension == ".dll" || extension == ".dylib") {
        return "application-x-sharedlib";
    }

    if (extension == ".txt" || extension == ".md" || extension == ".rst" || extension == ".log" || extension == ".json" ||
        extension == ".xml" || extension == ".yaml" || extension == ".yml" || extension == ".toml" || extension == ".ini" ||
        extension == ".conf" || extension == ".cfg" || extension == ".c" || extension == ".h" || extension == ".cc" ||
        extension == ".cpp" || extension == ".hpp" || extension == ".java") {
        return "text-x-generic";
    }

    return "application-x-addon";
}

DirectoryEntry::DirectoryEntry(fs::path path)
    : Container({}, StackDirection::Horizontal, "DirectoryEntry"), m_path(std::move(path)) {
    set_spacing(10.0F);
    set_size({grow(), px(32)});
    set_input_mode(InputMode::Target);
}

DirectoryEntry& DirectoryEntry::set_on_click(std::function<void(const fs::path&)> callback) {
    m_on_click = std::move(callback);
    return *this;
}

void DirectoryEntry::build() {
    if (m_path.empty()) {
        add<TextWidget>("UNKNOWN");
        return;
    }

    auto& textures = surface().runtime().textures();
    add<ImageWidget>(textures.find(get_icon_name(m_path))).set_size({px(16), px(16)});

    const std::string name = m_path.filename().string();
    add<TextWidget>(name.empty() ? m_path.string() : name);
}

void DirectoryEntry::apply_theme_defaults(const Theme& theme) {
    Container::apply_theme_defaults(theme);

    configure_all_styles([](Style& style) {
        style.padding({8, 6});
        style.border(BORDER_NONE);
    });

    configure_style(StyleType::HOVER, [](Style& style) { style.background_color({120, 120, 120, 200}); });
}

void DirectoryEntry::on_click(UiEvent&) {
    if (m_on_click) {
        m_on_click(m_path);
    }
}
