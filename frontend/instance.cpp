#include <string>
#include <vector>

#define IM_RGB(r, g, b) ImVec4(((float)r / 255.0f), ((float)g / 255.0f), ((float)b / 255.0f), 1.0)

#include "instance.hpp"

#include "res/IconsMaterialSymbols.h"

#include "ee/ee_dis.h"
#include "iop/iop_dis.h"

#include "ps2_elf.h"
#include "ps2_iso9660.h"

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

struct ee_dis_state g_ee_dis_state;
struct iop_dis_state g_iop_dis_state;

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
        SDL_WINDOW_OPENGL
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
    style.WindowPadding         = ImVec2(8.0, 8.0);
    style.FramePadding          = ImVec2(5.0, 5.0);
    style.ItemSpacing           = ImVec2(8.0, 4.0);
    style.WindowBorderSize      = 0;
    style.ChildBorderSize       = 0;
    style.FrameBorderSize       = 0;
    style.PopupBorderSize       = 0;
    style.TabBorderSize         = 0;
    style.TabBarBorderSize      = 0;
    style.WindowRounding        = 4;
    style.ChildRounding         = 4;
    style.FrameRounding         = 4;
    style.PopupRounding         = 4;
    style.ScrollbarRounding     = 9;
    style.GrabRounding          = 2;
    style.TabRounding           = 4;
    style.WindowTitleAlign      = ImVec2(0.5, 0.5);
    style.DockingSeparatorSize  = 0;

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
    ps2_init_kputchar(lunar->ps2, kputchar_stub, NULL, kputchar_stub, NULL);
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
    
    lunar->pause = 0;

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

void print_highlighted(const char* buf) {
    using namespace ImGui;

    std::vector <std::string> tokens;

    std::string text;

    while (*buf) {
        text.clear();        

        if (isalpha(*buf)) {
            while (isalpha(*buf) || isdigit(*buf))
                text.push_back(*buf++);
        } else if (isxdigit(*buf) || (*buf == '-')) {
            while (isxdigit(*buf) || (*buf == 'x') || (*buf == '-'))
                text.push_back(*buf++);
        } else if (*buf == '$') {
            while (*buf == '$' || isdigit(*buf) || isalpha(*buf) || *buf == '_')
                text.push_back(*buf++);
        } else if (*buf == ',') {
            while (*buf == ',')
                text.push_back(*buf++);
        } else if (*buf == '(') {
            while (*buf == '(')
                text.push_back(*buf++);
        } else if (*buf == ')') {
            while (*buf == ')')
                text.push_back(*buf++);
        } else if (*buf == '<') {
            while (*buf != '>')
                text.push_back(*buf++);

            text.push_back(*buf++);
        } else if (*buf == '_') {
            text.push_back(*buf++);
        } else if (*buf == '.') {
            text.push_back(*buf++);
        } else {
            printf("unhandled char %c (%d) \"%s\"\n", *buf, *buf, buf);

            exit(1);
        }

        while (isspace(*buf))
            text.push_back(*buf++);

        tokens.push_back(text);
    }

    for (const std::string& t : tokens) {
        if (isalpha(t[0])) {
            TextColored(IM_RGB(211, 167, 30), t.c_str());
        } else if (isdigit(t[0]) || t[0] == '-') {
            TextColored(IM_RGB(138, 143, 226), t.c_str());
        } else if (t[0] == '$') {
            TextColored(IM_RGB(68, 169, 240), t.c_str());
        } else if (t[0] == '<') {
            TextColored(IM_RGB(89, 89, 89), t.c_str());
        } else {
            Text(t.c_str());
        }

        SameLine(0.0f, 0.0f);
    }

    NewLine();
}

static void show_ee_disassembly_view(lunar::instance* lunar) {
    using namespace ImGui;

    PushFont(lunar->font_code);

    if (BeginTable("table1", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
        TableSetupColumn("a", ImGuiTableColumnFlags_NoResize, 15.0f);
        TableSetupColumn("b", ImGuiTableColumnFlags_NoResize, 15.0f);
        TableSetupColumn("c", ImGuiTableColumnFlags_NoResize);

        for (int row = -64; row < 64; row++) {
            g_ee_dis_state.pc = lunar->ps2->ee->pc + (row * 4);

            TableNextRow();
            TableSetColumnIndex(0);

            PushFont(lunar->font_icons);

            TableSetColumnIndex(1);

            if (g_ee_dis_state.pc == lunar->ps2->ee->pc)
                Text(ICON_MS_CHEVRON_RIGHT " ");

            PopFont();

            TableSetColumnIndex(2);

            uint32_t opcode = ee_bus_read32(lunar->ps2->ee_bus, g_ee_dis_state.pc & 0x1fffffff);

            char buf[128];

            char addr_str[9]; sprintf(addr_str, "%08x", g_ee_dis_state.pc);
            char opcode_str[9]; sprintf(opcode_str, "%08x", opcode);
            char* disassembly = ee_disassemble(buf, opcode, &g_ee_dis_state);

            Text("%s ", addr_str); SameLine();
            TextDisabled("%s ", opcode_str); SameLine();

            char id[16];

            sprintf(id, "##%d", row);

            if (Selectable(id, false, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
            } SameLine();

            if (BeginPopupContextItem()) {
                PushFont(lunar->font_small_code);
                TextDisabled("0x%08x", g_ee_dis_state.pc);
                PopFont();

                PushFont(lunar->font_body);

                if (BeginMenu(ICON_MS_CONTENT_COPY "  Copy")) {
                    if (Selectable(ICON_MS_SORT "  Address")) {
                        SDL_SetClipboardText(addr_str);
                    }

                    if (Selectable(ICON_MS_SORT "  Opcode")) {
                        SDL_SetClipboardText(opcode_str);
                    }

                    if (Selectable(ICON_MS_SORT "  Disassembly")) {
                        SDL_SetClipboardText(disassembly);
                    }

                    ImGui::EndMenu();
                }

                PopFont();
                EndPopup();
            }

            if (true) {
                print_highlighted(disassembly);
            } else {
                Text(disassembly);
            }

            if (g_ee_dis_state.pc == lunar->ps2->ee->pc)
                SetScrollHereY(0.5f);
        }

        EndTable();
    }

    PopFont();
}

static void show_iop_disassembly_view(lunar::instance* lunar) {
    using namespace ImGui;

    PushFont(lunar->font_code);

    if (BeginTable("table2", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
        TableSetupColumn("a", ImGuiTableColumnFlags_NoResize, 15.0f);
        TableSetupColumn("b", ImGuiTableColumnFlags_NoResize, 15.0f);
        TableSetupColumn("c", ImGuiTableColumnFlags_NoResize);

        for (int row = -64; row < 64; row++) {
            g_iop_dis_state.addr = lunar->ps2->iop->pc + (row * 4);

            TableNextRow();
            TableSetColumnIndex(0);

            PushFont(lunar->font_icons);

            TableSetColumnIndex(1);

            if (g_iop_dis_state.addr == lunar->ps2->iop->pc)
                Text(ICON_MS_CHEVRON_RIGHT " ");

            PopFont();

            TableSetColumnIndex(2);

            uint32_t opcode = iop_bus_read32(lunar->ps2->iop_bus, g_iop_dis_state.addr & 0x1fffffff);

            char buf[128];

            char addr_str[9]; sprintf(addr_str, "%08x", g_iop_dis_state.addr);
            char opcode_str[9]; sprintf(opcode_str, "%08x", opcode);
            char* disassembly = iop_disassemble(buf, opcode, &g_iop_dis_state);

            Text("%s ", addr_str); SameLine();
            TextDisabled("%s ", opcode_str); SameLine();

            char id[16];

            sprintf(id, "##%d", row);

            if (Selectable(id, false, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
            } SameLine();

            if (BeginPopupContextItem()) {
                PushFont(lunar->font_small_code);
                TextDisabled("0x%08x", g_iop_dis_state.addr);
                PopFont();

                PushFont(lunar->font_body);

                if (BeginMenu(ICON_MS_CONTENT_COPY "  Copy")) {
                    if (Selectable(ICON_MS_SORT "  Address")) {
                        SDL_SetClipboardText(addr_str);
                    }

                    if (Selectable(ICON_MS_SORT "  Opcode")) {
                        SDL_SetClipboardText(opcode_str);
                    }

                    if (Selectable(ICON_MS_SORT "  Disassembly")) {
                        SDL_SetClipboardText(disassembly);
                    }

                    ImGui::EndMenu();
                }

                PopFont();
                EndPopup();
            }

            if (true) {
                print_highlighted(disassembly);
            } else {
                Text(disassembly);
            }

            if (g_iop_dis_state.addr == lunar->ps2->iop->pc)
                SetScrollHereY(0.5f);
        }

        EndTable();
    }

    PopFont();
}

void destroy(lunar::instance* lunar);

void update(lunar::instance* lunar) {
    if (!lunar->pause) {
        ps2_cycle(lunar->ps2);
    } else {
        update_window(lunar);
    }
}

void update_window(lunar::instance* lunar) {
    using namespace ImGui;

    // ImGuiIO& io = ImGui::GetIO();

    // ImGui_ImplSDLRenderer2_NewFrame();
    // ImGui_ImplSDL2_NewFrame();
    // NewFrame();

    // DockSpaceOverViewport(0, GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // PushFont(lunar->font_body);

    // if (Begin("Logs")) {
    //     if (BeginTabBar("logs_tabs")) {
    //         if (BeginTabItem("EE")) {
    //             Text("EE logs");

    //             EndTabItem();
    //         }

    //         if (BeginTabItem("IOP")) {
    //             Text("IOP logs");

    //             EndTabItem();
    //         }

    //         EndTabBar();
    //     }
    // } End();

    // if (Begin("EE control")) {
    //     if (Button(ICON_MS_STEP)) {
    //         if (!lunar->pause) {
    //             lunar->pause = true;
    //         } else {
    //             tick_ee(lunar);
    //         }
    //     } SameLine();

    //     if (Button(lunar->pause ? ICON_MS_PLAY_ARROW : ICON_MS_PAUSE)) {
    //         lunar->pause = !lunar->pause;
    //     }
    // } End();

    // if (Begin("IOP control")) {
    //     if (Button(ICON_MS_STEP)) {
    //         if (!lunar->pause) {
    //             lunar->pause = true;
    //         } else {
    //             tick_iop(lunar);
    //         }
    //     }
    // } End();

    // if (Begin("EE disassembly")) {
    //     show_ee_disassembly_view(lunar);
    // } End();

    // if (Begin("IOP disassembly")) {
    //     show_iop_disassembly_view(lunar);
    // } End();

    // PopFont();

    // Render();

    // // SDL_GL_SwapWindow(lunar->window);

    // SDL_RenderSetScale(lunar->renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    // SDL_SetRenderDrawColor(lunar->renderer, 0, 0, 0, 0xff);
    // SDL_RenderClear(lunar->renderer);

    // int width = ((lunar->ps2->gs->display2 >> 32) & 0xfff) + 1;
    // width /= ((lunar->ps2->gs->display2 >> 23) & 0xf) + 1;
    // int height = ((lunar->ps2->gs->display2 >> 44) & 0x7ff) + 1;
    // // int stride = ((lunar->ps2->gs->frame_1 >> 16) & 0x3f) * 64;
    // uint64_t base = ((lunar->ps2->gs->frame_1 & 0x1f) * 2048);

    // SDL_DestroyTexture(lunar->texture);
    // lunar->texture = SDL_CreateTexture(
    //     lunar->renderer,
    //     SDL_PIXELFORMAT_ABGR8888,
    //     SDL_TEXTUREACCESS_STREAMING,
    //     width, height
    // );
    // SDL_SetTextureScaleMode(lunar->texture, SDL_ScaleModeLinear);
    // SDL_UpdateTexture(lunar->texture, NULL, lunar->ps2->gs->vram + base, width * 4);
    // SDL_RenderCopy(lunar->renderer, lunar->texture, NULL, NULL);

    // ImGui_ImplSDLRenderer2_RenderDrawData(GetDrawData(), lunar->renderer);

    // SDL_RenderPresent(lunar->renderer);

    software_render(lunar->ps2->gs, lunar->ps2->gs->backend.udata);

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT: {
                lunar->open = false;
            } break;

            case SDL_KEYDOWN: {
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