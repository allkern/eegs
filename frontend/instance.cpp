#include <string>
#include <vector>

#include "instance.hpp"

#include "ps2_elf.h"
#include "ps2_iso9660.h"

#include "ee/ee_dis.h"
#include "iop/iop_dis.h"

#include "res/IconsMaterialSymbols.h"

#include "ee/renderer/opengl.hpp"
#include "ee/renderer/software.hpp"

#include <csignal>

namespace lunar {

uint32_t map_button(SDL_Keycode k) {
    if (k == SDLK_x     ) return DS_BT_CROSS;
    if (k == SDLK_a     ) return DS_BT_SQUARE;
    if (k == SDLK_w     ) return DS_BT_TRIANGLE;
    if (k == SDLK_d     ) return DS_BT_CIRCLE;
    if (k == SDLK_RETURN) return DS_BT_START;
    if (k == SDLK_s     ) return DS_BT_SELECT;
    if (k == SDLK_UP    ) return DS_BT_UP;
    if (k == SDLK_DOWN  ) return DS_BT_DOWN;
    if (k == SDLK_LEFT  ) return DS_BT_LEFT;
    if (k == SDLK_RIGHT ) return DS_BT_RIGHT;
    if (k == SDLK_q     ) return DS_BT_L1;
    if (k == SDLK_e     ) return DS_BT_R1;
    if (k == SDLK_1     ) return DS_BT_L2;
    if (k == SDLK_3     ) return DS_BT_R2;
    if (k == SDLK_z     ) return DS_BT_L3;
    if (k == SDLK_c     ) return DS_BT_R3;

    return 0;
}

void kputchar_stub(void* udata, char c) {
    putchar(c);
}

void ee_kputchar(void* udata, char c) {
    lunar::instance* lunar = (lunar::instance*)udata;
    
    if (c == '\r')
        return;

    if (c == '\n') {
        lunar->ee_log.push_back("");
    } else {
        lunar->ee_log.back().push_back(c);
    }
}

void iop_kputchar(void* udata, char c) {
    lunar::instance* lunar = (lunar::instance*)udata;
    
    if (c == '\r')
        return;

    if (c == '\n') {
        lunar->iop_log.push_back("");
    } else {
        lunar->iop_log.back().push_back(c);
    }
}

static const ImWchar icon_range[] = { ICON_MIN_MS, ICON_MAX_16_MS, 0 };

void handle_scissor_event(void* udata) {
    lunar::instance* lunar = (lunar::instance*)udata;

    // int scax0 = lunar->ps2->gs->scissor_1 & 0x3ff;
    // int scay0 = (lunar->ps2->gs->scissor_1 >> 32) & 0x3ff;
    // int scax1 = (lunar->ps2->gs->scissor_1 >> 16) & 0x3ff;
    // int scay1 = (lunar->ps2->gs->scissor_1 >> 48) & 0x3ff;

    // // printf("sca0=(%d,%d) sca1=(%d,%d) frame_1=%x\n",
    // //     scax0, scay0,
    // //     scax1, scay1,
    // //     lunar->ps2->gs->frame_1
    // // );
    
    // int width = (scax1 - scax0) + 1;
    // int height = (scay1 - scay0) + 1;

    software_set_size((software_state*)lunar->ps2->gs->backend.udata, 0, 0);

    // opengl_set_size(lunar->renderer_state, width, height, 1.5);

    // SDL_SetWindowSize(lunar->window, width, height);
}

lunar::instance* create();

lunar::instance* g_lunar = nullptr;

static const char *ee_cc_r2[] = {
    "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

void sigint_handler(int signal) {
    char buf[128];
    struct ee_dis_state ee_ds;
    struct iop_dis_state iop_ds;

    ee_ds.print_address = 1;
    ee_ds.print_opcode = 1;
    iop_ds.print_address = 1;
    iop_ds.print_opcode = 1;

    struct ee_state* ee = g_lunar->ps2->ee;

    FILE* file = fopen("ram.dump", "wb");

    fwrite(g_lunar->ps2->ee_ram->buf, 1, RAM_SIZE_32MB, file);
    fclose(file);

    for (int i = 0; i < 32; i++) {
        printf("%s: %08x %08x %08x %08x\n",
            ee_cc_r2[i],
            ee->r[i].u32[3],
            ee->r[i].u32[2],
            ee->r[i].u32[1],
            ee->r[i].u32[0]
        );
    }

    printf("hi: %08x %08x %08x %08x\n",
        ee->hi.u32[3],
        ee->hi.u32[2],
        ee->hi.u32[1],
        ee->hi.u32[0]
    );
    printf("lo: %08x %08x %08x %08x\n",
        ee->hi.u32[3],
        ee->hi.u32[2],
        ee->hi.u32[1],
        ee->hi.u32[0]
    );
    printf("pc: %08x\n", ee->pc);

    for (int i = -8; i <= 8; i++) {
        ee_ds.pc = ee->pc + (i * 4);

        if (ee->pc == ee_ds.pc) {
            printf("--> ");
        } else {
            printf("    ");
        }

        puts(ee_disassemble(buf, ee_bus_read32(ee->bus.udata, ee_ds.pc & 0x1fffffff), &ee_ds));
    }

    printf("intc: stat=%08x mask=%08x\n",
        g_lunar->ps2->ee_intc->stat,
        g_lunar->ps2->ee_intc->mask
    );

    printf("dmac: stat=%08x mask=%08x\n",
        g_lunar->ps2->ee_dma->stat & 0x3ff,
        (g_lunar->ps2->ee_dma->stat >> 16) & 0x3ff
    );

    printf("epc: %08x\n", ee->epc);

    printf("IOP:\n");

    struct iop_state* iop = g_lunar->ps2->iop;

    printf("r0=%08x at=%08x v0=%08x v1=%08x\n", iop->r[0] , iop->r[1] , iop->r[2] , iop->r[3] );
    printf("a0=%08x a1=%08x a2=%08x a3=%08x\n", iop->r[4] , iop->r[5] , iop->r[6] , iop->r[7] );
    printf("t0=%08x t1=%08x t2=%08x t3=%08x\n", iop->r[8] , iop->r[9] , iop->r[10], iop->r[11]);
    printf("t4=%08x t5=%08x t6=%08x t7=%08x\n", iop->r[12], iop->r[13], iop->r[14], iop->r[15]);
    printf("s0=%08x s1=%08x s2=%08x s3=%08x\n", iop->r[16], iop->r[17], iop->r[18], iop->r[19]);
    printf("s4=%08x s5=%08x s6=%08x s7=%08x\n", iop->r[20], iop->r[21], iop->r[22], iop->r[23]);
    printf("t8=%08x t9=%08x k0=%08x k1=%08x\n", iop->r[24], iop->r[25], iop->r[26], iop->r[27]);
    printf("gp=%08x sp=%08x fp=%08x ra=%08x\n", iop->r[28], iop->r[29], iop->r[30], iop->r[31]);
    printf("pc=%08x hi=%08x lo=%08x ep=%08x\n", iop->pc, iop->hi, iop->lo, iop->cop0_r[COP0_EPC]);

    for (int i = -8; i <= 8; i++) {
        iop_ds.addr = iop->pc + (i * 4);

        if (iop->pc == iop_ds.addr) {
            printf("--> ");
        } else {
            printf("    ");
        }

        puts(iop_disassemble(buf, iop_bus_read32(iop->bus.udata, iop_ds.addr & 0x1fffffff), &iop_ds));
    }

    printf("intc: stat=%08x mask=%08x ctrl=%08x\n",
        g_lunar->ps2->iop_intc->stat,
        g_lunar->ps2->iop_intc->mask,
        g_lunar->ps2->iop_intc->ctrl
    );

    exit(1);
}

void init(lunar::instance* lunar, int argc, const char* argv[]) {
    g_lunar = lunar;
    std::signal(SIGINT, sigint_handler);

    lunar->window = SDL_CreateWindow(
        "eegs 0.1",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        960, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    lunar->renderer = SDL_CreateRenderer(
        lunar->window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );

    lunar->texture = SDL_CreateTexture(
        lunar->renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        640, 480
    );

    SDL_SetTextureScaleMode(lunar->texture, SDL_ScaleModeLinear);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(lunar->window, lunar->renderer);
    ImGui_ImplSDLRenderer2_Init(lunar->renderer);

    // Init fonts
    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f;
    config.GlyphOffset = ImVec2(0.0f, 4.0f);

    lunar->font_small_code = io.Fonts->AddFontFromFileTTF("res/FiraCode-Regular.ttf", 12.0f);
    lunar->font_code    = io.Fonts->AddFontFromFileTTF("res/FiraCode-Regular.ttf", 16.0f);
    lunar->font_small   = io.Fonts->AddFontFromFileTTF("res/Roboto-Regular.ttf", 12.0f);
    lunar->font_heading = io.Fonts->AddFontFromFileTTF("res/Roboto-Regular.ttf", 18.0f);
    lunar->font_body    = io.Fonts->AddFontFromFileTTF("res/Roboto-Regular.ttf", 14.0f);
    lunar->font_icons   = io.Fonts->AddFontFromFileTTF("res/" FONT_ICON_FILE_NAME_MSR, 20.0f, &config, icon_range);

    io.FontDefault = lunar->font_body;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding           = ImVec2(8.0, 8.0);
    style.FramePadding            = ImVec2(5.0, 5.0);
    style.ItemSpacing             = ImVec2(8.0, 4.0);
    style.WindowBorderSize        = 0;
    style.ChildBorderSize         = 0;
    style.FrameBorderSize         = 0;
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

    // Init theme
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.29f, 0.35f, 0.39f, 1.00f);
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

    lunar->open = true;

    lunar->ps2 = ps2_create();

    ps2_init(lunar->ps2);
    ps2_init_kputchar(lunar->ps2, ee_kputchar, lunar, iop_kputchar, lunar);
    ps2_gs_init_callback(lunar->ps2->gs, GS_EVENT_VBLANK, (void (*)(void*))update_window, lunar);
    ps2_gs_init_callback(lunar->ps2->gs, GS_EVENT_SCISSOR, handle_scissor_event, lunar);

    lunar->ds = ds_sio2_attach(lunar->ps2->sio2, 0);

    // Initialize hardware renderer
    // lunar->renderer_state = new opengl_state;

    // lunar->ps2->gs->backend.render_point = opengl_render_point;
    // lunar->ps2->gs->backend.render_line = opengl_render_line;
    // lunar->ps2->gs->backend.render_line_strip = opengl_render_line_strip;
    // lunar->ps2->gs->backend.render_triangle = opengl_render_triangle;
    // lunar->ps2->gs->backend.render_triangle_strip = opengl_render_triangle_strip;
    // lunar->ps2->gs->backend.render_triangle_fan = opengl_render_triangle_fan;
    // lunar->ps2->gs->backend.render_sprite = opengl_render_sprite;
    // lunar->ps2->gs->backend.render = opengl_render;
    // lunar->ps2->gs->backend.udata = lunar->renderer_state;

    // opengl_init(lunar->renderer_state);

    // To-do: Implement backend constructor and destructor
    lunar->ps2->gs->backend.udata = new software_state;
    lunar->ps2->gs->backend.render_point = software_render_point;
    lunar->ps2->gs->backend.render_line = software_render_line;
    lunar->ps2->gs->backend.render_triangle = software_render_triangle;
    lunar->ps2->gs->backend.render_sprite = software_render_sprite;
    lunar->ps2->gs->backend.render = software_render;
    lunar->ps2->gs->backend.transfer_start = software_transfer_start;
    lunar->ps2->gs->backend.transfer_write = software_transfer_write;
    lunar->ps2->gs->backend.transfer_read = software_transfer_read;

    software_init((software_state*)lunar->ps2->gs->backend.udata, lunar->ps2->gs, lunar->window, lunar->renderer);

    // lunar->ps2->gs->vqi = 0;
    // lunar->ps2->gs->prim = 5;

    // int cx = 640 / 2;
    // int cy = 480 / 2;
    // int r = 200;
    // float p = 0.25f * M_PI;

    // lunar->ps2->gs->rgbaq = 0xff0000; gs_write_vertex(lunar->ps2->gs, (320 << 4) | (240 << 20), 0);

    // int m = 8;

    // for (int i = 0; i < m; i++) {
    //     uint64_t px = (r * sin(p + (float)i * (0.1f * M_PI))) + cx;
    //     uint64_t py = (r * cos(p + (float)i * (0.1f * M_PI))) + cy;

    //     uint32_t r = i * (255 / m);
    //     uint32_t g = 0;
    //     uint32_t b = 0xff - (i * (255 / m));

    //     lunar->ps2->gs->rgbaq = (r & 0xff) | (g << 8) | (b << 16);

    //     gs_write_vertex(lunar->ps2->gs, (px << 4) | (py << 20), 0);
    // }

    // lunar->ps2->gs->vqi = 0;
    // lunar->ps2->gs->prim = 4;

    // int sx = 50;
    // int sy = 240;

    // int dx = 25;
    // int dy = 50;

    // int q = 9;

    // for (int i = 0; i < q; i++) {
    //     uint32_t r = i * (255 / m);
    //     uint32_t g = 0;
    //     uint32_t b = 0xff - (i * (255 / m));

    //     lunar->ps2->gs->rgbaq = (r & 0xff) | (g << 8) | (b << 16);

    //     gs_write_vertex(lunar->ps2->gs, (sx << 4) | (sy << 20), 0);

    //     sx += dx;
    //     sy += (i & 1) ? -dy : dy;
    // }
    
    lunar->pause = true;

    lunar->elf_path = NULL;
    lunar->boot_path = NULL;
    lunar->bios_path = NULL;
    lunar->disc_path = NULL;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];

        if (a == "-x") {
            lunar->elf_path = argv[i+1];

            ++i;
        } else if (a == "-d") {
            lunar->boot_path = argv[i+1];

            ++i;
        } else if (a == "-b") {
            lunar->bios_path = argv[i+1];

            ++i;
        } else if (a == "-i") {
            lunar->disc_path = argv[i+1];

            ++i;
        } else {
            lunar->disc_path = argv[i];
        }
    }

    if (!lunar->bios_path) {
        printf("lunar: Please specify a PS2 BIOS file\n");

        exit(1);
    }

    ps2_load_bios(lunar->ps2, lunar->bios_path);

    if (lunar->elf_path) {
        ps2_elf_load(lunar->ps2, lunar->elf_path);
    }

    if (lunar->boot_path) {
        ps2_boot_file(lunar->ps2, lunar->boot_path);
    }

    if (lunar->disc_path) {
        struct iso9660_state* iso = iso9660_open(lunar->disc_path);

        if (!iso) {
            printf("lunar: Couldn't open disc image \"%s\"\n", lunar->disc_path);

            exit(1);

            return;
        }

        char* boot_file = iso9660_get_boot_path(iso);

        if (!boot_file)
            return;

        ps2_boot_file(lunar->ps2, boot_file);
        ps2_cdvd_open(lunar->ps2->cdvd, lunar->disc_path);
    }
}

void destroy(lunar::instance* lunar);

void update(lunar::instance* lunar) {
    if (!lunar->pause) {
        ps2_cycle(lunar->ps2);
    } else {
        if (lunar->step) {
            ps2_cycle(lunar->ps2);

            lunar->step = false;
        }

        update_window(lunar);
    }
}

void update_window(lunar::instance* lunar) {
    using namespace ImGui;

    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    NewFrame();

    show_main_menubar(lunar);

    DockSpaceOverViewport(0, GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (lunar->show_ee_control) show_ee_control(lunar);
    if (lunar->show_ee_state) show_ee_state(lunar);
    if (lunar->show_ee_logs) show_ee_logs(lunar);
    if (lunar->show_iop_control) show_iop_control(lunar);
    if (lunar->show_iop_state) show_iop_state(lunar);
    if (lunar->show_iop_logs) show_iop_logs(lunar);
    if (lunar->show_gs_debugger) show_gs_debugger(lunar);

    Render();

    SDL_RenderSetScale(lunar->renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    SDL_SetRenderDrawColor(lunar->renderer, 0x18, 0x18, 0x18, 0xff);
    SDL_RenderClear(lunar->renderer);

    software_render(lunar->ps2->gs, lunar->ps2->gs->backend.udata);

    ImGui_ImplSDLRenderer2_RenderDrawData(GetDrawData(), lunar->renderer);
    
    SDL_RenderPresent(lunar->renderer);

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT: {
                lunar->open = false;
            } break;

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_0) {
                    ps2_iop_intc_irq(lunar->ps2->iop_intc, IOP_INTC_SPU2);

                    break;
                }
                uint16_t mask = map_button(event.key.keysym.sym);

                ds_button_press(lunar->ds, mask);
            } break;

            case SDL_KEYUP: {
                uint16_t mask = map_button(event.key.keysym.sym);

                ds_button_release(lunar->ds, mask);
            } break;
            
        }
    }
}

bool is_open(lunar::instance* lunar) {
    return lunar->open;
}

void close(lunar::instance* lunar) {
    using namespace ImGui;

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    DestroyContext();

    SDL_DestroyRenderer(lunar->renderer);
    SDL_DestroyWindow(lunar->window);
    SDL_Quit();

    ps2_destroy(lunar->ps2);
}

}