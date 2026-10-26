#pragma once

#include <SDL3/SDL.h>

constexpr int kScreenW = 320;
constexpr int kScreenH = 180;
constexpr int kTileSize = 16;

struct Input {
    SDL_FPoint move{}; // each axis in [-1, 1]
};

class Game {
public:
    void update(const Input& input, float dt);
    void render(SDL_Renderer* renderer) const;
    void debugUi();

private:
    SDL_FPoint playerPos_{152.0f, 82.0f};
    float playerSpeed_ = 80.0f; // pixels per second
};
