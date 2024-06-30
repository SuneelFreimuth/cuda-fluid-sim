#pragma once

#include <SDL3/SDL.h>

#include "bitmap.hpp"


class Renderer {
public:
    ~Renderer();
    
    void Setup(int width, int height);
    SDL_Event PollEvent();
    void BlitBitmap(Bitmap *b);

private:
    SDL_Window *window = nullptr;
};
