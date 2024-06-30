#include <algorithm>

#include "bitmap.hpp"


std::string Pixel::String() {
    return "Pixel{" +
        std::to_string(r) + ", " +
        std::to_string(g) + ", " +
        std::to_string(b) +
    "}";
}


Bitmap::Bitmap(int width, int height) {
    surface = SDL_CreateSurface(width, height,
        SDL_PIXELFORMAT_RGB24);
}

Bitmap::~Bitmap() {
    SDL_DestroySurface(surface);
    surface = nullptr;
}

int Bitmap::Width() const {
    return surface->w;
}

int Bitmap::Height() const {
    return surface->h;
}

Pixel* Bitmap::Pixels() {
    return (Pixel*) surface->pixels;
}

const Pixel* Bitmap::Pixels() const {
    return (const Pixel*) surface->pixels;
}

void Bitmap::Set(int r, int c, const Pixel &p) {
    int i = 3 * (r * Width() + c);
    ((Pixel*) surface->pixels)[i] = p;
}

void Bitmap::Fill(const Pixel &p) {
    std::fill(
        Pixels(),
        Pixels() + Width() * Height(),
        p
    );
}

SDL_Surface* Bitmap::Surface() {
    return surface;
}
