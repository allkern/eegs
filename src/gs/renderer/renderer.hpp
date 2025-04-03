#pragma once

#include <SDL.h>

#include "gs/gs.h"

enum : int {
    // Keeps aspect ratio by native resolution
    RENDERER_ASPECT_NATIVE,
    // Stretch to window (disregard aspect, disregard scale)
    RENDERER_ASPECT_STRETCH,
    // Stretch to window (keep aspect, disregard scale)
    RENDERER_ASPECT_STRETCH_KEEP,
    // Force 4:3
    RENDERER_ASPECT_4_3,
    // Force 16:9
    RENDERER_ASPECT_16_9,
    // Use NVRAM settings (same as SOFTWARE_ASPECT_STRETCH_KEEP for now)
    RENDERER_ASPECT_AUTO
};

enum : int {
    RENDERER_NULL = 0,
    RENDERER_SOFTWARE,
    RENDERER_SOFTWARE_THREAD
};

struct renderer_state {
    struct ps2_gs* gs = nullptr;
    void* udata = nullptr;

    void (*init)(void*, struct ps2_gs*, SDL_Window*) = nullptr;
    void (*destroy)(void*) = nullptr;
    void (*set_size)(void*, int, int) = nullptr;
    void (*set_scale)(void*, float) = nullptr;
    void (*set_aspect_mode)(void*, int) = nullptr;
    void (*set_integer_scaling)(void*, bool) = nullptr;
    void (*set_bilinear)(void*, bool) = nullptr;
    void (*get_viewport_size)(void*, int*, int*) = nullptr;
    void (*get_display_size)(void*, int*, int*) = nullptr;
    void (*get_display_format)(void*, int*) = nullptr;
    void (*set_window_rect)(void*, int, int, int, int) = nullptr;
    const char* (*get_name)(void*) = nullptr;
};

renderer_state* renderer_create(void);
void renderer_init_null(renderer_state* renderer, struct ps2_gs* gs, SDL_Window* window);
void renderer_init_software(renderer_state* renderer, struct ps2_gs* gs, SDL_Window* window);
void renderer_init_software_thread(renderer_state* renderer, struct ps2_gs* gs, SDL_Window* window);
void renderer_init_backend(renderer_state* renderer, struct ps2_gs* gs, SDL_Window* window, int id);
void renderer_set_size(renderer_state* renderer, int w, int h);
void renderer_set_scale(renderer_state* renderer, float scale);
void renderer_set_aspect_mode(renderer_state* renderer, int mode);
void renderer_set_integer_scaling(renderer_state* renderer, bool integer_scaling);
void renderer_set_bilinear(renderer_state* renderer, bool bilinear);
void renderer_get_viewport_size(renderer_state* renderer, int* w, int* h);
void renderer_get_display_size(renderer_state* renderer, int* w, int* h);
void renderer_get_display_format(renderer_state* renderer, int* fmt);
void renderer_set_window_rect(renderer_state* renderer, int x, int y, int w, int h);
const char* renderer_get_name(renderer_state* renderer);
void renderer_render(renderer_state* renderer);
void renderer_destroy(renderer_state* renderer);