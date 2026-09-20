#include "user_directories.hpp"

#include <iostream>
#include <string>
#include <ui/widgets/button.hpp>
#include <utility>

using namespace ui;

class DirectoryButton : public ButtonWidget {
public:
    explicit DirectoryButton(std::string text) : ButtonWidget(std::move(text)) {}

    void apply_theme_defaults(const Theme& theme) override {
        ButtonWidget::apply_theme_defaults(theme);

        configure_all_styles([](Style& style) {
            style.padding({8, 6});
            style.border(BORDER_NONE);
        });

        configure_style(StyleType::HOVER, [](Style& style) { style.background_color({120, 120, 120, 255}); });
    }
};

UserDirectories::UserDirectories() : ResizableContainer("User Directories") {
    set_spacing(1.0F);
    set_size({px(240), grow()});
    set_scrollable(true);
    set_resize(ResizeAxes::X);
}

void UserDirectories::build(const KeyValueMap& entries) {
    if (entries.empty()) {
        return;
    }

    clear();

    for (const auto& entry : entries) {
        fs::path full_path(kv_parser::resolve_path(entry.second));

        auto& directory = add<DirectoryButton>(full_path.filename());
        directory.set_on_click([this, full_path] {
            if (m_on_click) {
                std::cout << "[+] clicked on " << full_path.string() << "\n";
                m_on_click(full_path);
            }
        });
    }
}

void UserDirectories::set_on_click(std::function<void(const fs::path&)> callback) {
    m_on_click = std::move(callback);
}

void UserDirectories::apply_theme_defaults(const Theme& theme) {
    Container::apply_theme_defaults(theme);

    configure_all_styles([&theme](Style& style) {
        style.background_color(theme.background_secondary_color);
        style.border(BORDER_RIGHT);
        style.border_color(theme.border_color);
        style.border_thickness(1.0F);
        style.padding({8.0F, 8.0F});
    });
}
