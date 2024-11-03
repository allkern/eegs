#pragma once

#include <cstdint>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>

#include "ps2.h"

namespace lunar {

struct instance {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    struct ps2_state* ps2 = nullptr;

    unsigned int window_width;
    unsigned int window_height;
    unsigned int texture_width;
    unsigned int texture_height;
    uint32_t* texture_buf = nullptr;

    ImFont* font_small_code = nullptr;
    ImFont* font_code = nullptr;
    ImFont* font_small = nullptr;
    ImFont* font_heading = nullptr;
    ImFont* font_body = nullptr;
    ImFont* font_icons = nullptr;

    bool open = false;
    bool pause = true;
    bool step = false;
};

lunar::instance* create();
void init(lunar::instance* lunar, int argc, const char* argv[]);
void close(lunar::instance* lunar);
void destroy(lunar::instance* lunar);
bool is_open(lunar::instance* lunar);

void update(lunar::instance* lunar);
void update_window(lunar::instance* lunar);

}