#pragma once

#include "../filesystem/directory_manager.hpp"
#include "../utils/common.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ui {
    class UI;
}

class DirectoryList;
class UserDirectories;

class FileManagerApp {
public:
    explicit FileManagerApp(ui::UI& surface);

    FileManagerApp(const FileManagerApp&) = delete;
    FileManagerApp& operator=(const FileManagerApp&) = delete;
    FileManagerApp(FileManagerApp&&) = delete;
    FileManagerApp& operator=(FileManagerApp&&) = delete;

private:
    void load_icons();
    void build_layout();
    void open_directory(const fs::path& directory, bool reset_buffer);

    ui::UI& m_ui;
    DirectoryManager m_directory_manager;
    UserDirectories* m_dirs = nullptr;
    DirectoryList* m_dir_list = nullptr;
    std::string m_search_value;
    fs::path m_current_dir;
    std::vector<fs::path> m_directory_buffer;
    std::size_t m_current_buffer_index = 0;
};
