#include <iostream>

#include "bitmap.hpp"
#include "renderer.hpp"


int WIDTH = 640;
int HEIGHT = 480;

int main(int argc, char **argv){
    Renderer renderer;
    renderer.Setup(WIDTH, HEIGHT);

    Bitmap frame(WIDTH, HEIGHT);
    frame.Fill({255, 255, 255});

    while (1) {
        auto event = renderer.PollEvent();
        bool quit = false;
        switch (event.type) {
        case SDL_EVENT_QUIT:
            quit = true;
            break;
        }
        if (quit)
            break;

        renderer.BlitBitmap(&frame);
    }

    return 0;
}
