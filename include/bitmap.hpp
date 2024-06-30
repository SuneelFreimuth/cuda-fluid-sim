#pragma once

#include <string>

#include "SDL3/SDL.h"


// RGB24 pixel: https://wiki.libsdl.org/SDL3/SDL_PixelFormatEnum
struct Pixel {
    uint8_t r, g, b;

    std::string String();
};


class Bitmap {
public:
    Bitmap(int width, int height);
    ~Bitmap();

    int Width() const;
    int Height() const;
    Pixel* Pixels();
    const Pixel* Pixels() const;
    SDL_Surface* Surface();
    const SDL_Surface* Surface() const;

    void Set(int r, int c, const Pixel& p);
    void Fill(const Pixel& p);

private:
    SDL_Surface *surface;
};
