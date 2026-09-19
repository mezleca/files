#include "gtk_theme.hpp"
#include "key-value.hpp"

#include <SDL3/SDL.h>
#include <array>
#include <cctype>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <ui/backends/opengl/texture-loader.hpp>
#include <ui/backends/sdl/backend.hpp>
#include <ui/layout/resizable-container.hpp>
#include <ui/layout/virtual-layout.hpp>
#include <ui/tree/node.hpp>
#include <ui/runtime.hpp>
#include <ui/ui.hpp>
#include <ui/widgets/image.hpp>
#include <ui/widgets/text.hpp>
#include <ui/widgets/button.hpp>
#include <ui/widgets/text-input.hpp>

using namespace ui;

constexpr std::array icons{
    std::string_view{"folder"},
    std::string_view{"application-x-addon"},
    std::string_view{"application-x-executable"},
    std::string_view{"application-x-sharedlib"},
    std::string_view{"audio-x-generic"},
    std::string_view{"font-x-generic"},
    std::string_view{"image-x-generic"},
    std::string_view{"package-x-generic"},
    std::string_view{"system-search"},
    std::string_view{"text-html"},
    std::string_view{"text-x-generic"},
    std::string_view{"text-x-script"},
    std::string_view{"video-x-generic"},
    std::string_view{"x-office-address-book"},
    std::string_view{"x-office-calendar"},
    std::string_view{"x-office-document"},
    std::string_view{"x-office-presentation"},
    std::string_view{"x-office-spreadsheet"}
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

        configure_style(StyleType::HOVER, [](Style& style) { style.background_color({120, 120, 120, 255}); });
    }
};

static std::string_view get_icon_name(const fs::path& entry) {
    if (fs::is_directory(entry)) {
        return "folder";
    }

    auto status = fs::status(entry);
    auto perms = fs::perms(status.permissions());

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

class DirectoryEntry : public Container {
public:
    DirectoryEntry(fs::path path) : Container({}, StackDirection::Horizontal, "DirectoryEntry"), m_path(path) {
        set_spacing(10.0F);
        set_size({grow(), px(32)});
        set_input_mode(InputMode::Target);
    }

    DirectoryEntry& set_on_click(std::function<void(const fs::path&)> callback) {
        m_on_click = std::move(callback);
        return *this;
    }

    void build() {
        if (m_path.empty()) {
            add<TextWidget>("UNKNOWN");
            return;
        }

        auto& textures = surface().runtime().textures();
        add<ImageWidget>(textures.find(get_icon_name(m_path))).set_size({px(16), px(16)});

        const std::string name = m_path.filename().string();
        add<TextWidget>(name.empty() ? m_path.string() : name);
    }

    void apply_theme_defaults(const Theme& theme) override {
        Container::apply_theme_defaults(theme);

        configure_all_styles([](Style& style) {
            style.padding({8, 6});
            style.border(BORDER_NONE);
        });

        configure_style(StyleType::HOVER, [](Style& style) { style.background_color({120, 120, 120, 200}); });
    }

private:
    void on_click(UiEvent&) override {
        if (m_on_click) {
            m_on_click(m_path);
        }
    }

    fs::path m_path;
    std::function<void(const fs::path&)> m_on_click;
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
            dir.set_on_click([this, full_path] {
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

class DirectoryList : public VirtualLayout {
public:
    DirectoryList() : VirtualLayout("Directory List", 32.0F) {
        set_spacing(10.0F);
        set_size({grow(), grow()});
    }

    void apply_theme_defaults(const Theme& theme) override {
        Container::apply_theme_defaults(theme);
    }

    void build(bool has_directory = false) {
        clear_built_entries();
        m_entry_widgets.clear();
        m_default_entry = nullptr;

        if (m_entries.empty()) {
            const char* message = has_directory ? "folder has no content" : "Hello :D";
            set_items(1, [this, message](std::size_t) -> Node& {
                if (m_default_entry == nullptr) {
                    m_default_entry = &add<TextWidget>(message);
                }

                return *m_default_entry;
            });
            return;
        }

        set_items(m_entries.size(), [this](std::size_t index) -> Node& {
            const auto found = m_entry_widgets.find(index);
            if (found != m_entry_widgets.end()) {
                return *found->second;
            }

            auto& entry = add<DirectoryEntry>(m_entries[index]);
            entry.build();
            entry.set_on_click([this](const fs::path& path) {
                if (m_on_click) {
                    m_pending_entry = path;
                }
            });
            m_entry_widgets.emplace(index, &entry);
            return entry;
        });
    }

    void add_entry(fs::path path) {
        m_entries.push_back(path);
    }

    void clear_entries() {
        m_entries.clear();
        clear_built_entries();
        m_entry_widgets.clear();
        m_default_entry = nullptr;
        m_pending_entry.reset();
        set_items(0);
    }

    void set_on_click(std::function<void(const fs::path&)> callback) {
        m_on_click = std::move(callback);
    }

protected:
    void on_update(float) override {
        if (!m_pending_entry.has_value()) {
            return;
        }

        fs::path path = std::move(*m_pending_entry);
        m_pending_entry.reset();
        if (m_on_click) {
            m_on_click(path);
        }
    }

private:
    void clear_built_entries() {
        while (!children().empty()) {
            remove(*children().back());
        }
    }

    std::vector<fs::path> m_entries;
    std::unordered_map<std::size_t, DirectoryEntry*> m_entry_widgets;
    TextWidget* m_default_entry = nullptr;
    std::function<void(const fs::path&)> m_on_click;
    std::optional<fs::path> m_pending_entry;
};

static bool is_same_entry(const fs::path& a, const fs::path& b) {
    if (a.empty() || b.empty()) {
        return false;
    }

    std::error_code error;
    return fs::equivalent(a, b, error);
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
        search_input.set_icon(texture_register.find("system-search"));

        m_dir_list = &directory_column.add<DirectoryList>();

        fs::path dirs_path(kv_parser::resolve_path("$HOME/.config/user-dirs.dirs"));
        m_dirs->build(kv_parser::parse_file(dirs_path));

        m_dirs->set_on_click([this](const fs::path& dir) { open_directory(dir, true); });

        m_dir_list->set_on_click([this](const fs::path& dir) { open_directory(dir, false); });

        m_dir_list->build();
    }

private:
    void open_directory(const fs::path& directory, bool reset_buffer) {
        if (is_same_entry(directory, m_current_dir)) {
            return;
        }

        if (!fs::is_directory(directory)) {
            if (reset_buffer) {
                m_dir_list->clear_entries();
                m_dir_list->build();
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
        m_dir_list->clear_entries();

        for (const auto& entry : fs::directory_iterator(m_current_dir)) {
            m_dir_list->add_entry(entry.path());
        }

        m_dir_list->build(true);
    }

    UI& m_ui;

    UserDirectories* m_dirs;
    DirectoryList* m_dir_list;

    std::string m_search_value;
    fs::path m_current_dir;
    std::vector<fs::path> m_directory_buffer;
    std::size_t m_current_buffer_index = 0;
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
