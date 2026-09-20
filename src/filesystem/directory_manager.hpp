#pragma once

#include "../utils/common.hpp"

#include <vector>

class DirectoryManager {
public:
    std::vector<fs::path> list_entries(const fs::path& directory) const;
    bool is_directory(const fs::path& path) const;
    bool is_same_entry(const fs::path& left, const fs::path& right) const;
};
