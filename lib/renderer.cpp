#include "renderer.hpp"


Renderer::~Renderer() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Renderer::Setup(int width, int height) {
	SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Fluid Simulation", width, height, 0);
}

SDL_Event Renderer::PollEvent() {
    SDL_Event e;
    SDL_PollEvent(&e);
    return e;
}

void Renderer::BlitBitmap(Bitmap *b) {
    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);
    SDL_BlitSurface(b->Surface(), NULL, windowSurface, NULL);
    SDL_UpdateWindowSurface(window);
}
