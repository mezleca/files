#pragma once

#include "../utils/key-value.hpp"

#include <functional>
#include <ui/layout/resizable-container.hpp>

class UserDirectories : public ui::ResizableContainer {
public:
    UserDirectories();

    void build(const KeyValueMap& entries);
    void set_on_click(std::function<void(const fs::path&)> callback);
    void apply_theme_defaults(const ui::Theme& theme) override;

private:
    std::function<void(const fs::path&)> m_on_click;
};
