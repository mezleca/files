#include "gtk_theme.hpp"
#include "key-value.hpp"

#include <SDL3/SDL.h>
#include <iostream>
#include <ui/backends/opengl/texture-loader.hpp>
#include <ui/backends/sdl/backend.hpp>
#include <ui/layout/resizable-container.hpp>
#include <ui/tree/node.hpp>
#include <ui/runtime.hpp>
#include <ui/ui.hpp>
#include <ui/widgets/image.hpp>
#include <ui/widgets/text.hpp>
#include <ui/widgets/button.hpp>
#include <ui/widgets/image.hpp>

using namespace ui;

constexpr std::array icons{
    std::string_view{"folder"},
    std::string_view{"application-x-addon"},
    std::string_view{"application-x-executable"}
};

class Content : public Container {
public:
    Content() : Container("Content") {
        set_scrollable(false);
        set_size({grow(), grow()});
    }
};

class DirectoryButton : public ButtonWidget {
public:
    DirectoryButton(std::string text) : ButtonWidget(text) {}

    void apply_theme_defaults(const Theme& theme) override {
        ButtonWidget::apply_theme_defaults(theme);

        configure_all_styles([](Style& style) {
            style.padding({8, 6});
            style.border(BORDER_NONE);
        });

        configure_style(StyleType::HOVER, [](Style& style){
            style.background_color({ 120, 120, 120, 255 });
        });
    }
};

enum class EntryType : uint8_t {
    FILE = 0,
    EXECUTABLE,
    FOLDER
};

static EntryType get_entry_type(const fs::path& entry) {
    if (fs::is_directory(entry)) {
        return EntryType::FOLDER;
    }

    auto status = fs::status(entry);
    auto perms = fs::perms(status.permissions());

    if ((perms & fs::perms::owner_exec) != fs::perms::none) {
        return EntryType::EXECUTABLE;
    }

    return EntryType::FILE;
}

class DirectoryEntry : public Container {
public:
    // NOTE: use empty id (auto) so these fuckers dont render on top of each other
    DirectoryEntry(fs::path path) : Container({}, StackDirection::Horizontal, "DirectoryEntry"), m_path(path) {
        set_spacing(10.0F);
        set_size({grow(), px(32)});
    }

    void build() {
        if (m_path.empty()) {
            add<TextWidget>("UNKNOWN");
            return;
        }

        auto& textures = surface().runtime().textures();
        auto entry_type = get_entry_type(m_path);

        switch (entry_type) {
            case EntryType::FILE:
                add<ImageWidget>(textures.find("application-x-addon")).set_size({px(16), px(16)});
                break;
            case EntryType::EXECUTABLE:
                add<ImageWidget>(textures.find("application-x-executable")).set_size({px(16), px(16)});
                break;
            case EntryType::FOLDER:
                add<ImageWidget>(textures.find("folder")).set_size({px(16), px(16)});
                break;
        }

        add<TextWidget>(m_path.string());
    }

    void apply_theme_defaults(const Theme& theme) override {
        Container::apply_theme_defaults(theme);

        configure_all_styles([](Style& style) {
            style.padding({8, 6});
            style.border(BORDER_NONE);
        });

        configure_style(StyleType::HOVER, [](Style& style){
            style.background_color({ 120, 120, 120, 200 });
        });
    }

private:
    fs::path m_path;
};

class UserDirectories : public ResizableContainer {
public:
    UserDirectories() : ResizableContainer("User Directories") {
        set_spacing(1.0F);
        set_size({px(240), grow()});
        set_scrollable(true);
        set_resize(ResizeAxes::X);
    }

    void apply_theme_defaults(const Theme& theme) override {
        Container::apply_theme_defaults(theme);

        configure_all_styles([&theme](Style& style) {
            style.background_color(theme.background_secondary_color);
            style.border(BORDER_RIGHT);
            style.border_color(theme.border_color);
            style.border_thickness(1.0F);
            style.padding({8.0F, 8.0F});
        });
    }

    void build(const KeyValueMap& entries) {
        if (entries.size() == 0) {
            return;
        }

        clear();

        for (const auto& entry : entries) {
            fs::path full_path(kv_parser::resolve_path(entry.second));

            auto& dir = add<DirectoryButton>(full_path.filename());
            dir.set_on_click([this, full_path]{
                if (m_on_click) {
                    std::cout << "[+] clicked on " << full_path.string() << "\n";
                    m_on_click(full_path);
                }
            });
        }
    }

    void set_on_click(std::function<void(const fs::path& a)> callback) {
        m_on_click = std::move(callback);
    }

private:
    std::function<void(const fs::path&)> m_on_click;
};

class DirectoryList : public Container {
public:
    DirectoryList() : Container("Directory List", StackDirection::Vertical) {
        set_spacing(10.0F);
        set_size({grow(), grow()});
        set_scrollable(true);
    }

    void apply_theme_defaults(const Theme& theme) override {
        Container::apply_theme_defaults(theme);
    }

    void build() {
        if (m_entries.size() == 0) {
            std::cout << "[+] bulding default content" << "\n";
            build_default();
            return;
        }

        clear();
        for (const auto& entry : m_entries) {
            add<DirectoryEntry>(entry).build();
        }

        m_using_default = false;
    }

    void add_entry(fs::path path) {
        m_entries.push_back(path);
    }

    void clear_entries() {
        m_entries.clear();
        clear();
        m_using_default = false;
    }

protected:
    void build_default() {
        if (m_using_default) {
            return;
        }

        clear();
        add<TextWidget>("uhh, no folda");
        m_using_default = true;
    }

private:
    std::vector<fs::path> m_entries;
    bool m_using_default = false;
};

static bool is_same_entry(const fs::path& a, const fs::path& b) {
    if (a.empty() || b.empty()) {
        return false;
    }

    return fs::equivalent(a, b);
}

class App {
public:
    App(UI& ui) : m_ui(ui) {
        // register icons based on the current loaded gtk theme
        auto& texture_register = ui.runtime().textures();

        auto settings = current_settings();
        auto icon_theme = IconTheme::load(settings.icon_theme_name).value();

        for (const auto& name : icons) {
            auto icon = icon_theme.find_icon(name, 16);
            if (!icon.has_value()) {
                std::cout << "[-] failed to load " << name << "\n";
                continue;
            }

            texture_register.add(name.data(), icon.value());
        }

        auto& content = m_ui.root().add<Content>();
        auto& container = content.add<Container>("container", StackDirection::Horizontal);

        m_dirs = &container.add<UserDirectories>();
        m_dir_list = &container.add<DirectoryList>();

        fs::path dirs_path(kv_parser::resolve_path("$HOME/.config/user-dirs.dirs"));
        m_dirs->build(kv_parser::parse_file(dirs_path));

        m_dirs->set_on_click([&](const fs::path& dir){
            if (is_same_entry(dir, m_current_dir)) {
                return;
            }

            if (!fs::is_directory(dir)) {
                m_dir_list->clear_entries();
                m_dir_list->build();
                return;
            }

            m_current_dir = dir;
            m_dir_list->clear_entries();

            for (const auto& entry : fs::directory_iterator(m_current_dir)) {
                m_dir_list->add_entry(entry);
            }

            m_dir_list->build();
        });

        m_dir_list->build();
    }

private:
    UI& m_ui;

    UserDirectories* m_dirs;
    DirectoryList* m_dir_list;

    fs::path m_current_dir;
};

int main() {
#if defined(__linux__)
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("file manager", 700, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (window == nullptr) {
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    {
        RuntimeConfig runtime_config;
        runtime_config.texture_loader = std::make_unique<OpenGLTextureLoader>();
        Runtime runtime(std::move(runtime_config));

        auto backend = std::make_unique<SdlBackend>(window, context);

        UIConfig ui_config;
        ui_config.backend = std::move(backend);
        ui_config.enable_debugger = true;

        UI surface(runtime, std::move(ui_config));

        App app(surface);

        while (!surface.is_done()) {
            surface.process_events();

            surface.begin_frame();
            surface.update(ImGui::GetIO().DeltaTime);
            surface.draw();
            surface.end_frame();
        }
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
