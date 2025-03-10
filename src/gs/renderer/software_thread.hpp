#pragma once

#include <vector>
#include <cstdio>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include <mutex>

#include <SDL.h>

#include "GL/gl3w.h"

#include "gs/gs.h"

#include "renderer.hpp"

struct render_data {
    int prim;
    struct ps2_gs gs;
};

struct software_thread_state {
    std::queue <render_data> render_queue;
    std::thread render_thr;
    std::mutex queue_mtx;
    std::mutex render_mtx;

    std::atomic <bool> end_signal;

    unsigned int sbp, dbp;
    unsigned int sbw, dbw;
    unsigned int spsm, dpsm;
    unsigned int ssax, ssay, dsax, dsay;
    unsigned int rrh, rrw;
    unsigned int dir, xdir;
    unsigned int sx, sy;
    unsigned int dx, dy, px;

    uint32_t psmct24_data;
    uint32_t psmct24_shift;

    SDL_Window* window = nullptr;
    struct ps2_gs* gs;

    int tex_w = 0;
    int tex_h = 0;
    int disp_w = 0;
    int disp_h = 0;
    int disp_fmt = 0;
    int aspect_mode = 0; // Native
    bool integer_scaling = false;
    float scale = 1.5;
    bool bilinear = true;

    GLuint vert_shader = 0;

    std::vector <GLuint> programs;

    GLuint default_program;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint tex = 0;
    GLuint fbo = 0;
    GLuint fb_vao = 0;
    GLuint fb_vbo = 0;
    GLuint fb_ebo = 0;
    GLuint fb_in_tex = 0;
    GLuint fb_out_tex = 0;

    unsigned int frame = 0;

    uint32_t* buf = nullptr;
};

void software_thread_init(void* udata, struct ps2_gs* gs, SDL_Window* window);
void software_thread_destroy(void* udata);
void software_thread_set_size(void* udata, int width, int height);
void software_thread_set_scale(void* udata, float scale);
void software_thread_set_aspect_mode(void* udata, int aspect_mode);
void software_thread_set_integer_scaling(void* udata, bool integer_scaling);
void software_thread_set_bilinear(void* udata, bool bilinear);
void software_thread_get_viewport_size(void* udata, int* w, int* h);
void software_thread_get_display_size(void* udata, int* w, int* h);
void software_thread_get_display_format(void* udata, int* fmt);
const char* software_thread_get_name(void* udata);
// void software_push_shader(software_state* ctx, const char* path);
// void software_pop_shader(software_state* ctx);

extern "C" {
void software_thread_render_point(struct ps2_gs* gs, void* udata);
void software_thread_render_line(struct ps2_gs* gs, void* udata);
void software_thread_render_triangle(struct ps2_gs* gs, void* udata);
void software_thread_render_sprite(struct ps2_gs* gs, void* udata);
void software_thread_render(struct ps2_gs* gs, void* udata);
void software_thread_transfer_start(struct ps2_gs* gs, void* udata);
void software_thread_transfer_write(struct ps2_gs* gs, void* udata);
void software_thread_transfer_read(struct ps2_gs* gs, void* udata);
}