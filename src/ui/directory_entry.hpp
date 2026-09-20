#pragma once

#include "../utils/common.hpp"

#include <functional>
#include <ui/layout/container.hpp>

class DirectoryEntry : public ui::Container {
public:
    explicit DirectoryEntry(fs::path path);

    DirectoryEntry& set_on_click(std::function<void(const fs::path&)> callback);
    void build();
    void apply_theme_defaults(const ui::Theme& theme) override;

private:
    void on_click(ui::UiEvent& event) override;

    fs::path m_path;
    std::function<void(const fs::path&)> m_on_click;
};
