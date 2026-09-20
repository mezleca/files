#include "directory_manager.hpp"

#include <system_error>

std::vector<fs::path> DirectoryManager::list_entries(const fs::path& directory) const {
    std::vector<fs::path> entries;
    for (const auto& entry : fs::directory_iterator(directory)) {
        entries.push_back(entry.path());
    }

    return entries;
}

bool DirectoryManager::is_directory(const fs::path& path) const {
    return fs::is_directory(path);
}

bool DirectoryManager::is_same_entry(const fs::path& left, const fs::path& right) const {
    if (left.empty() || right.empty()) {
        return false;
    }

    std::error_code error;
    return fs::equivalent(left, right, error);
}
