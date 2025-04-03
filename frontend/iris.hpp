#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <deque>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>
#include "GL/gl3w.h"

#include "ps2.h"
#include "gs/renderer/renderer.hpp"

namespace iris {

enum : int {
    BKPT_CPU_EE,
    BKPT_CPU_IOP
};

struct breakpoint {
    uint32_t addr;
    int cpu;
    bool cond_r, cond_w, cond_x;
    int size;
    bool enabled;
};

struct move_animation {
    int frames;
    int frames_remaining;
    float source_x, source_y;
    float target_x, target_y;
    float x, y;
};

struct fade_animation {
    int frames;
    int frames_remaining;
    int source_alpha, target_alpha;
    int alpha;
};

struct notification {
    int type;
    int state;
    int frames;
    int frames_remaining;
    float width, height;
    float text_width, text_height;
    bool end;
    move_animation move;
    fade_animation fade;
    std::string text;
};

struct instance {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_GLContext gl_context = nullptr;

    struct ps2_state* ps2 = nullptr;

    unsigned int window_width = 960;
    unsigned int window_height = 720;
    unsigned int texture_width;
    unsigned int texture_height;
    uint32_t* texture_buf = nullptr;

    unsigned int renderer_backend = RENDERER_SOFTWARE_THREAD;
    renderer_state* ctx = nullptr;

    ImFont* font_small_code = nullptr;
    ImFont* font_code = nullptr;
    ImFont* font_small = nullptr;
    ImFont* font_heading = nullptr;
    ImFont* font_body = nullptr;
    ImFont* font_icons = nullptr;
    ImFont* font_icons_big = nullptr;

    std::string elf_path = "";
    std::string boot_path = "";
    std::string bios_path = "";
    std::string disc_path = "";
    std::string pref_path = "";
    std::string ini_path = "";

    bool open = false;
    bool pause = true;
    bool step = false;

    bool show_ee_control = false;
    bool show_ee_state = false;
    bool show_ee_logs = false;
    bool show_ee_interrupts = false;
    bool show_ee_dmac = false;
    bool show_iop_control = false;
    bool show_iop_state = false;
    bool show_iop_logs = false;
    bool show_iop_interrupts = false;
    bool show_iop_dma = false;
    bool show_gs_debugger = false;
    bool show_memory_viewer = false;
    bool show_status_bar = true;
    bool show_breakpoints = false;
    bool show_imgui_demo = false;

    // Special windows
    bool show_bios_setting_window = false;
    bool show_about_window = false;

    bool fullscreen = false;
    int aspect_mode = RENDERER_ASPECT_AUTO;
    bool bilinear = true;
    bool integer_scaling = false;
    float scale = 1.5f;
    int window_mode = 0;

    std::deque <std::string> recents;

    bool dump_to_file = true;
    std::string settings_path = "";

    int frames = 0;
    float fps = 0.0f;
    unsigned int ticks = 0;
    int menubar_height = 0;
    bool mute = false;

    bool limit_fps = true;
    float fps_cap = 60.0f;

    std::string loaded = "";

    std::vector <std::string> ee_log = { "" };
    std::vector <std::string> iop_log = { "" };

    std::vector <iris::breakpoint> breakpoints = {};
    std::deque <iris::notification> notifications = {};

    struct ds_state* ds = nullptr;
};

iris::instance* create();
void init(iris::instance* iris, int argc, const char* argv[]);
void close(iris::instance* iris);
void destroy(iris::instance* iris);
bool is_open(iris::instance* iris);

int init_audio(iris::instance* iris);
int init_settings(iris::instance* iris, int argc, const char* argv[]);
void cli_check_for_help_version(iris::instance* iris, int argc, const char* argv[]);
void close_settings(iris::instance* iris);

void update(iris::instance* iris);
void update_window(iris::instance* iris);

void show_main_menubar(iris::instance* iris);
void show_ee_control(iris::instance* iris);
void show_ee_state(iris::instance* iris);
void show_ee_logs(iris::instance* iris);
void show_ee_interrupts(iris::instance* iris);
void show_ee_dmac(iris::instance* iris);
void show_iop_control(iris::instance* iris);
void show_iop_state(iris::instance* iris);
void show_iop_logs(iris::instance* iris);
void show_iop_interrupts(iris::instance* iris);
void show_iop_dma(iris::instance* iris);
void show_gs_debugger(iris::instance* iris);
void show_memory_viewer(iris::instance* iris);
void show_status_bar(iris::instance* iris);
void show_breakpoints(iris::instance* iris);
void show_about_window(iris::instance* iris);
void show_bios_setting_window(iris::instance* iris);
// void show_gamelist(iris::instance* iris);

void handle_keydown_event(iris::instance* iris, SDL_KeyboardEvent& key);
void handle_keyup_event(iris::instance* iris, SDL_KeyboardEvent& key);
void handle_scissor_event(void* udata);
void handle_ee_tty_event(void* udata, char c);
void handle_iop_tty_event(void* udata, char c);

void handle_animations(iris::instance* iris);

void push_info(iris::instance* iris, std::string text);

}