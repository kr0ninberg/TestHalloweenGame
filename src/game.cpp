#include "game.h"

#include <algorithm>
#include <cmath>

#include <imgui.h>

void Game::update(const Input& input, float dt)
{
    SDL_FPoint dir = input.move;
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 1.0f) {
        dir.x /= len;
        dir.y /= len;
    }

    playerPos_.x += dir.x * playerSpeed_ * dt;
    playerPos_.y += dir.y * playerSpeed_ * dt;
    playerPos_.x = std::clamp(playerPos_.x, 0.0f, float(kScreenW - kTileSize));
    playerPos_.y = std::clamp(playerPos_.y, 0.0f, float(kScreenH - kTileSize));
}

void Game::render(SDL_Renderer* renderer) const
{
    // Placeholder checkerboard to visualize the 16px tile grid.
    for (int ty = 0; ty * kTileSize < kScreenH; ++ty) {
        for (int tx = 0; tx * kTileSize < kScreenW; ++tx) {
            if ((tx + ty) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 56, 118, 64, 255);
            else
                SDL_SetRenderDrawColor(renderer, 64, 130, 72, 255);
            const SDL_FRect tile{float(tx * kTileSize), float(ty * kTileSize), float(kTileSize), float(kTileSize)};
            SDL_RenderFillRect(renderer, &tile);
        }
    }

    // Snap to whole pixels so the sprite never lands between canvas pixels.
    const SDL_FRect player{std::floor(playerPos_.x), std::floor(playerPos_.y), float(kTileSize), float(kTileSize)};
    SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255);
    SDL_RenderFillRect(renderer, &player);
}

void Game::debugUi()
{
    ImGui::Begin("Debug");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Player: %.1f, %.1f", playerPos_.x, playerPos_.y);
    ImGui::SliderFloat("Speed", &playerSpeed_, 10.0f, 300.0f);
    ImGui::End();
}
