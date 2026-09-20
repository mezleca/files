#include "app/file_manager.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <memory>
#include <ui/backends/opengl/texture-loader.hpp>
#include <ui/backends/sdl/backend.hpp>
#include <ui/runtime.hpp>
#include <ui/ui.hpp>
#include <utility>

using namespace ui;

int main() {
#if defined(__linux__)
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }

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
        FileManagerApp app(surface);

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
