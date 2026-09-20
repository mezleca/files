#include "file_manager.hpp"

#include "../ui/directory_list.hpp"
#include "../ui/user_directories.hpp"
#include "../utils/gtk_theme.hpp"
#include "../utils/key-value.hpp"

#include <iostream>
#include <string>
#include <ui/backends/opengl/texture-loader.hpp>
#include <ui/layout/container.hpp>
#include <ui/runtime.hpp>
#include <ui/ui.hpp>
#include <ui/widgets/text-input.hpp>
#include <utility>
#include <vector>

using namespace ui;

class Content : public Container {
public:
    Content() : Container("Content") {
        set_scrollable(false);
        set_size({grow(), grow()});
    }
};

FileManagerApp::FileManagerApp(UI& surface) : m_ui(surface) {
    load_icons();
    build_layout();

    const fs::path directories_path(kv_parser::resolve_path("$HOME/.config/user-dirs.dirs"));
    m_dirs->build(kv_parser::parse_file(directories_path));
    m_dirs->set_on_click([this](const fs::path& directory) { open_directory(directory, true); });

    m_dir_list->set_on_click([this](const fs::path& directory) { open_directory(directory, false); });
    m_dir_list->set_entries({}, false);
}

void FileManagerApp::load_icons() {
    static const std::vector<std::string> gtk_icons = {
        "folder",
        "application-x-addon",
        "application-x-executable",
        "application-x-sharedlib",
        "audio-x-generic",
        "font-x-generic",
        "image-x-generic",
        "package-x-generic",
        "system-search",
        "text-html",
        "text-x-generic",
        "text-x-script",
        "video-x-generic",
        "x-office-address-book",
        "x-office-calendar",
        "x-office-document",
        "x-office-presentation",
        "x-office-spreadsheet"
    };

    auto& texture_register = m_ui.runtime().textures();
    const auto settings = current_settings();
    const auto icon_theme = IconTheme::load(settings.icon_theme_name).value();

    for (const auto& name : gtk_icons) {
        const auto icon = icon_theme.find_icon(name, 16);
        if (!icon.has_value()) {
            std::cout << "[-] failed to load " << name << "\n";
            continue;
        }

        texture_register.add(name.data(), icon.value());
    }
}

void FileManagerApp::build_layout() {
    auto& content = m_ui.root().add<Content>();
    auto& container = content.add<Container>("container", StackDirection::Horizontal);

    auto& dirs_column = container.add<Container>("user-directories-column", StackDirection::Vertical);
    dirs_column.set_size({fit(), grow()});
    m_dirs = &dirs_column.add<UserDirectories>();

    auto& directory_column = container.add<Container>("directory-column", StackDirection::Vertical);
    directory_column.set_size({grow(), grow()});
    directory_column.set_spacing(8.0F);

    auto& search_bar = directory_column.add<Container>("search-bar");
    search_bar.set_size({grow(), fit()});
    const auto& theme = m_ui.theme();
    search_bar.configure_all_styles([&theme](Style& style) {
        style.background_color(theme.background_secondary_color)
            .border(BORDER_BOTTOM)
            .border_color(theme.header_border_color)
            .border_thickness(1.0F)
            .padding({12.0F, 10.0F});
    });

    auto& search_input = search_bar.add<TextInputWidget>(m_search_value, "search");
    search_input.set_size({grow(), px(42.0F)});
    search_input.set_icon(m_ui.runtime().textures().find("system-search"));

    m_dir_list = &directory_column.add<DirectoryList>();
}

void FileManagerApp::open_directory(const fs::path& directory, bool reset_buffer) {
    if (m_directory_manager.is_same_entry(directory, m_current_dir)) {
        return;
    }

    if (!m_directory_manager.is_directory(directory)) {
        if (reset_buffer) {
            m_dir_list->set_entries({}, false);
        }
        return;
    }

    if (reset_buffer || m_directory_buffer.empty()) {
        m_directory_buffer.clear();
        m_directory_buffer.push_back(directory);
        m_current_buffer_index = 0;
    } else {
        if (m_current_buffer_index + 1 < m_directory_buffer.size()) {
            m_directory_buffer.erase(m_directory_buffer.begin() + m_current_buffer_index + 1, m_directory_buffer.end());
        }

        m_directory_buffer.push_back(directory);
        m_current_buffer_index = m_directory_buffer.size() - 1;
    }

    m_current_dir = directory;
    m_dir_list->set_entries(m_directory_manager.list_entries(m_current_dir), true);
}
