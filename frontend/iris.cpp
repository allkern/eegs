#include <filesystem>
#include <csignal>
#include <chrono>
#include <string>
#include <vector>
#include <string>

#include "iris.hpp"
#include "platform.hpp"

#include "ee/ee_dis.h"
#include "iop/iop_dis.h"

#include "res/IconsMaterialSymbols.h"
#include "tfd/tinyfiledialogs.h"

#include "gs/renderer/software.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <SDL_filesystem.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include "config.hpp"

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

namespace iris {

static const ImWchar icon_range[] = { ICON_MIN_MS, ICON_MAX_16_MS, 0 };

iris::instance* create();

void update_title(iris::instance* iris) {
    char buf[128];

    std::string base = "";

    if (iris->loaded.size()) {
        base = std::filesystem::path(iris->loaded).filename().string();
    }

    sprintf(buf, base.size() ? "Iris (eegs 0.1) | %s" : "Iris (eegs 0.1)",
        base.c_str()
    );

    SDL_SetWindowTitle(iris->window, buf);
}

void init(iris::instance* iris, int argc, const char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    gl3wInit();

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    cli_check_for_help_version(iris, argc, argv);

    iris->window = SDL_CreateWindow(
        "Iris (eegs 0.1)",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        960, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    init_platform(iris);
    init_audio(iris);

    iris->gl_context = SDL_GL_CreateContext(iris->window);
    SDL_GL_MakeCurrent(iris->window, iris->gl_context);
    SDL_GL_SetSwapInterval(1);

    ImGui::CreateContext();
    IMGUI_CHECKVERSION();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (std::filesystem::exists("portable")) {
        iris->pref_path = "./";
    } else {
        char* pref = SDL_GetPrefPath("Allkern", "Iris");

        iris->pref_path = std::string(pref);

        SDL_free(pref);
    }

    iris->ini_path = iris->pref_path + "imgui.ini";

    io.IniFilename = iris->ini_path.c_str();
 
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(iris->window, iris->gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Init fonts
    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f;
    config.GlyphOffset = ImVec2(0.0f, 4.0f);

    iris->font_small_code = io.Fonts->AddFontFromFileTTF("res/FiraCode-Regular.ttf", 12.0f);
    iris->font_code       = io.Fonts->AddFontFromFileTTF("res/FiraCode-Regular.ttf", 16.0f);
    iris->font_small      = io.Fonts->AddFontFromFileTTF("res/Roboto-Regular.ttf", 12.0f);
    iris->font_heading    = io.Fonts->AddFontFromFileTTF("res/Roboto-Regular.ttf", 20.0f);
    iris->font_body       = io.Fonts->AddFontFromFileTTF("res/Roboto-Regular.ttf", 16.0f);
    iris->font_icons      = io.Fonts->AddFontFromFileTTF("res/" FONT_ICON_FILE_NAME_MSR, 20.0f, &config, icon_range);

    io.FontDefault = iris->font_body;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding           = ImVec2(8.0, 8.0);
    style.FramePadding            = ImVec2(5.0, 5.0);
    style.ItemSpacing             = ImVec2(8.0, 4.0);
    style.WindowBorderSize        = 0;
    style.ChildBorderSize         = 0;
    style.FrameBorderSize         = 1;
    style.PopupBorderSize         = 0;
    style.TabBorderSize           = 0;
    style.TabBarBorderSize        = 0;
    style.WindowRounding          = 6;
    style.ChildRounding           = 4;
    style.FrameRounding           = 4;
    style.PopupRounding           = 4;
    style.ScrollbarRounding       = 9;
    style.GrabRounding            = 2;
    style.TabRounding             = 4;
    style.WindowTitleAlign        = ImVec2(0.5, 0.5);
    style.DockingSeparatorSize    = 0;
    style.SeparatorTextBorderSize = 1;
    style.SeparatorTextPadding    = ImVec2(20, 0);

    // Init theme
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.07f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.10f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.10f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.20f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.29f, 0.35f, 0.39f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.20f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.39f, 0.47f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.49f, 0.59f, 0.65f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.13f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.20f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.29f, 0.35f, 0.39f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.13f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.20f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.29f, 0.35f, 0.39f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.23f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.33f, 0.39f, 0.43f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.38f, 0.46f, 0.51f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.15f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.00f, 0.30f, 0.25f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.00f, 0.39f, 0.32f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.23f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.26f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.10f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.15f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.15f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.15f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    iris->open = true;
    iris->ps2 = ps2_create();

    ps2_init(iris->ps2);
    ps2_init_kputchar(iris->ps2, handle_ee_tty_event, iris, handle_iop_tty_event, iris);
    ps2_gs_init_callback(iris->ps2->gs, GS_EVENT_VBLANK, (void (*)(void*))update_window, iris);
    ps2_gs_init_callback(iris->ps2->gs, GS_EVENT_SCISSOR, handle_scissor_event, iris);

    iris->ds = ds_sio2_attach(iris->ps2->sio2, 0);

    iris->ctx = renderer_create();
    iris->renderer_backend = RENDERER_NULL;

    renderer_init_backend(iris->ctx, iris->ps2->gs, iris->window, RENDERER_NULL);

    iris->ticks = SDL_GetTicks();
    iris->pause = true;
    
    init_settings(iris, argc, argv);
    
    renderer_init_backend(iris->ctx, iris->ps2->gs, iris->window, iris->renderer_backend);
    renderer_set_scale(iris->ctx, iris->scale);
    renderer_set_aspect_mode(iris->ctx, iris->aspect_mode);
    renderer_set_bilinear(iris->ctx, iris->bilinear);
    renderer_set_integer_scaling(iris->ctx, iris->integer_scaling);

    // Note:
    // Crashes on Windows for some reason?
    // ImGui reports the font is not loaded, but it should be.
    // Possible ImGui issue?

    // ImGui::PushFont(iris->font_body);

    // if (iris->renderer_backend == RENDERER_NULL) {
    //     push_info(iris, "Renderer is set to \"Null\", no output will be displayed");
    // }

    // ImGui::PopFont();
}

void destroy(iris::instance* iris);

void update(iris::instance* iris) {
    if (!iris->pause) {
        ps2_cycle(iris->ps2);

        // for (const breakpoint& b : iris->breakpoints) {
        //     if (iris->ps2->ee->pc == b.addr)
        //         iris->pause = true;
        // }
    } else {
        if (iris->step) {
            ps2_cycle(iris->ps2);

            iris->step = false;
        }

        update_window(iris);
    }
}

void update_time(iris::instance* iris) {
    int t = SDL_GetTicks() - iris->ticks;

    if (t < 500)
        return;

    if (iris->fps == 0.0f) {
        iris->fps = (float)iris->frames;
    } else {
        iris->fps += (float)iris->frames;
        iris->fps /= 2.0f;
    }

    iris->ticks = SDL_GetTicks();
    iris->frames = 0;
}

void sleep_limiter(iris::instance* iris) {
    uint32_t ticks = (1.0f / iris->fps_cap) * 1000.0f;
    uint32_t now = SDL_GetTicks();

    while ((SDL_GetTicks() - now) < ticks);
}

void update_window(iris::instance* iris) {
    using namespace ImGui;

    // Only limit FPS to 60 when paused
    if (iris->limit_fps && iris->pause)
        sleep_limiter(iris);

    update_title(iris);
    update_time(iris);

    ImGuiIO& io = ImGui::GetIO();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    NewFrame();

    show_main_menubar(iris);

    DockSpaceOverViewport(0, GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (iris->show_ee_control) show_ee_control(iris);
    if (iris->show_ee_state) show_ee_state(iris);
    if (iris->show_ee_logs) show_ee_logs(iris);
    if (iris->show_ee_interrupts) show_ee_interrupts(iris);
    if (iris->show_ee_dmac) show_ee_dmac(iris);
    if (iris->show_iop_control) show_iop_control(iris);
    if (iris->show_iop_state) show_iop_state(iris);
    if (iris->show_iop_logs) show_iop_logs(iris);
    if (iris->show_iop_interrupts) show_iop_interrupts(iris);
    if (iris->show_iop_dma) show_iop_dma(iris);
    if (iris->show_gs_debugger) show_gs_debugger(iris);
    if (iris->show_memory_viewer) show_memory_viewer(iris);
    if (iris->show_status_bar) show_status_bar(iris);
    if (iris->show_breakpoints) show_breakpoints(iris);
    if (iris->show_about_window) show_about_window(iris);
    if (iris->show_imgui_demo) ShowDemoWindow(&iris->show_imgui_demo);
    if (iris->show_bios_setting_window) show_bios_setting_window(iris);

    if (iris->pause) {
        int width, height;

        SDL_GetWindowSize(iris->window, &width, &height);

        auto ts = CalcTextSize(ICON_MS_PAUSE);

        GetBackgroundDrawList()->AddText(
            ImVec2(
                width - ts.x * 1.5f,
                iris->menubar_height + ts.x * 0.5f
            ),
            0xffffffff,
            ICON_MS_PAUSE
        );
    }

    handle_animations(iris);

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.11, 0.11, 0.11, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render display texture
    renderer_render(iris->ctx);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(iris->window);

    iris->frames++;

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT: {
                iris->open = false;
            } break;

            case SDL_KEYDOWN: {
                handle_keydown_event(iris, event.key);
            } break;

            case SDL_KEYUP: {
                handle_keyup_event(iris, event.key);
            } break;
        }
    }
}

bool is_open(iris::instance* iris) {
    return iris->open;
}

void close(iris::instance* iris) {
    using namespace ImGui;

    renderer_destroy(iris->ctx);

    destroy_platform(iris);
    close_settings(iris);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    DestroyContext();

    SDL_GL_DeleteContext(iris->gl_context);
    SDL_DestroyWindow(iris->window);
    SDL_Quit();

    ps2_destroy(iris->ps2);
}

}