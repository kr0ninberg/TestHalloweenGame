#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "game.h"

namespace {

constexpr Uint64 kTickNs = SDL_NS_PER_SECOND / 60;
constexpr float kTickSeconds = 1.0f / 60.0f;
constexpr Uint64 kMaxFrameNs = kTickNs * 5; // cap catch-up after a stall

struct App {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* canvas = nullptr; // game renders here at kScreenW x kScreenH
    Uint64 lastNs = 0;
    Uint64 accumulatorNs = 0;
    bool showDebug = true;
    Game game;
};

Input readInput()
{
    const bool* k = SDL_GetKeyboardState(nullptr);
    Input input;
    input.move.x = float((k[SDL_SCANCODE_D] || k[SDL_SCANCODE_RIGHT]) - (k[SDL_SCANCODE_A] || k[SDL_SCANCODE_LEFT]));
    input.move.y = float((k[SDL_SCANCODE_S] || k[SDL_SCANCODE_DOWN]) - (k[SDL_SCANCODE_W] || k[SDL_SCANCODE_UP]));
    return input;
}

// Largest integer multiple of the canvas that fits the output, centered.
SDL_FRect integerScaledRect(SDL_Renderer* renderer)
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    const int scale = std::max(1, std::min(w / kScreenW, h / kScreenH));
    const int dw = kScreenW * scale;
    const int dh = kScreenH * scale;
    return {float((w - dw) / 2), float((h - dh) / 2), float(dw), float(dh)};
}

} // namespace

SDL_AppResult SDL_AppInit(void** appstate, int, char**)
{
    SDL_SetAppMetadata("game1", "0.1", "dev.game1");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto* app = new App;
    *appstate = app;

    if (!SDL_CreateWindowAndRenderer("game1", kScreenW * 4, kScreenH * 4,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &app->window, &app->renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(app->renderer, 1);

    app->canvas = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                    kScreenW, kScreenH);
    if (!app->canvas) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetTextureScaleMode(app->canvas, SDL_SCALEMODE_NEAREST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplSDL3_InitForSDLRenderer(app->window, app->renderer);
    ImGui_ImplSDLRenderer3_Init(app->renderer);

    app->lastNs = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* app = static_cast<App*>(appstate);
    ImGui_ImplSDL3_ProcessEvent(event);

    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F1 && !event->key.repeat)
        app->showDebug = !app->showDebug;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* app = static_cast<App*>(appstate);
    SDL_Renderer* renderer = app->renderer;

    const Uint64 now = SDL_GetTicksNS();
    app->accumulatorNs = std::min(app->accumulatorNs + (now - app->lastNs), kMaxFrameNs);
    app->lastNs = now;

    const Input input = ImGui::GetIO().WantCaptureKeyboard ? Input{} : readInput();
    while (app->accumulatorNs >= kTickNs) {
        app->game.update(input, kTickSeconds);
        app->accumulatorNs -= kTickNs;
    }

    SDL_SetRenderTarget(renderer, app->canvas);
    app->game.render(renderer);

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    const SDL_FRect dst = integerScaledRect(renderer);
    SDL_RenderTexture(renderer, app->canvas, nullptr, &dst);

    // ImGui draws at native resolution, on top of the scaled canvas.
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    if (app->showDebug)
        app->game.debugUi();
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult)
{
    auto* app = static_cast<App*>(appstate);
    if (!app)
        return;

    if (ImGui::GetCurrentContext()) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    SDL_DestroyTexture(app->canvas);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    delete app;
}
