#pragma once

#include "../utils/common.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <ui/layout/virtual-layout.hpp>
#include <unordered_map>
#include <vector>

class DirectoryEntry;

namespace ui {
    class TextWidget;
}

class DirectoryList : public ui::VirtualLayout {
public:
    DirectoryList();

    void set_entries(std::vector<fs::path> entries, bool has_directory = false);
    void set_on_click(std::function<void(const fs::path&)> callback);
    void apply_theme_defaults(const ui::Theme& theme) override;

protected:
    void on_update(float dt) override;

private:
    void clear_built_entries();

    std::vector<fs::path> m_entries;
    std::unordered_map<std::size_t, DirectoryEntry*> m_entry_widgets;
    ui::TextWidget* m_default_entry = nullptr;
    std::function<void(const fs::path&)> m_on_click;
    std::optional<fs::path> m_pending_entry;
};
