#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>

#include "ee/gs.h"
#include "software.hpp"

#include <SDL.h>

#include "GL/gl3w.h"

static const char* default_vert_shader =
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "layout (location = 1) in vec2 uv;\n"
    "uniform vec2 screen_size;\n"
    "out vec2 FragCoord;\n"
    "out vec2 FragTexCoord;\n"
    "void main() {\n"
    "    FragCoord = (pos.xy + 1.0) / 2.0;\n"
    "    FragTexCoord = uv;\n"
    "    FragCoord.y = 1.0 - FragCoord.y;\n"
    "    gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
    "}";

static const char* default_frag_shader =
    "#version 330 core\n"
    "in vec2 FragCoord;\n"
    "in vec2 FragTexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D input_texture;\n"
    "uniform vec2 screen_size;\n"
    "void main() {\n"
    "    vec2 uv = vec2(FragTexCoord.x, 1.0 - FragTexCoord.y);\n"
    "    FragColor = texture(input_texture, uv);\n"
    "}";

static const char* frag_header =
    "#version 330 core\n"
    "in vec2 FragCoord;\n"
    "in vec2 FragTexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D input_texture;\n"
    "uniform vec2 screen_size;\n"
    "uniform int frame;\n";

static const int psmct32_clut_block[] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x40, 0x41, 0x42, 0x43,
    0x44, 0x45, 0x46, 0x47,
    0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f,
    0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f
};

void software_set_size(software_state* ctx, int width, int height) {
    int en1 = ctx->gs->pmode & 1;
    int en2 = (ctx->gs->pmode >> 1) & 1;

    uint64_t display, dispfb;

    if (en1) {
        display = ctx->gs->display1;
        dispfb = ctx->gs->dispfb1;
    } else if (en2) {
        display = ctx->gs->display2;
        dispfb = ctx->gs->dispfb2;
    }

    ctx->disp_fmt = (dispfb >> 15) & 0x1f;

    int magh = ((display >> 23) & 7) + 1;
    int magv = ((display >> 27) & 3) + 1;
    ctx->tex_w = (((display >> 32) & 0xfff) / magh) + 1;
    ctx->tex_h = (((display >> 44) & 0x7ff) / magv) + 1;

    if (((display >> 32) & 0xfff) == 0) {
        ctx->tex_w = 0;
        ctx->tex_h = 0;

        return;
    }

    // SMODE2 INT=1 FFMD=0
    // if ((ctx->gs->smode2 & 1) == 1) {
    //     ctx->tex_h /= 2;
    // }

    if (ctx->tex) {
        glDeleteTextures(1, &ctx->tex);
    }

    glGenTextures(1, &ctx->tex);
    glBindTexture(GL_TEXTURE_2D, ctx->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ctx->tex_w, ctx->tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // printf("software: width=%d height=%d format=%d display=%08x\n",
    //     ((display >> 32) & 0xfff),
    //     ((display >> 44) & 0x7ff),
    //     format, display
    // );
}

void software_set_scale(software_state* ctx, float scale) {
    ctx->scale = scale;
}

void software_set_aspect_mode(software_state* ctx, int aspect_mode) {
    ctx->aspect_mode = aspect_mode;
}

void software_set_integer_scaling(software_state* ctx, bool integer_scaling) {
    ctx->integer_scaling = integer_scaling;
}

void software_set_bilinear(software_state* ctx, bool bilinear) {
    ctx->bilinear = bilinear;
}

void software_get_viewport_size(software_state* ctx, int* w, int* h) {
    *w = ctx->tex_w;
    *h = ctx->tex_h;
}

void software_get_display_size(software_state* ctx, int* w, int* h) {
    *w = ctx->disp_w;
    *h = ctx->disp_h;
}

void software_get_display_format(software_state* ctx, int* fmt) {
    *fmt = ctx->disp_fmt;
}

void software_get_interlace_mode(software_state* ctx, int* interlace) {
    // *interlace = ctx->gs->
}

const char* software_get_name(software_state* ctx) {
    return "Software";
}

#define CLAMP(v, l, u) (((v) > (u)) ? (u) : (((v) < (l)) ? (l) : (v)))

static inline uint32_t gs_apply_function(struct ps2_gs* gs, uint32_t t, uint32_t f) {
    uint32_t pr, pg, pb, pa;
    uint32_t tr = t & 0xff;
    uint32_t tg = (t >> 8) & 0xff;
    uint32_t tb = (t >> 16) & 0xff;
    uint32_t ta = (t >> 24) & 0xff;
    uint32_t fr = f & 0xff;
    uint32_t fg = (f >> 8) & 0xff;
    uint32_t fb = (f >> 16) & 0xff;
    uint32_t fa = (f >> 24) & 0xff;

    switch (gs->ctx->tfx) {
        case GS_MODULATE: {
            pr = CLAMP((tr * fr) >> 7, 0, 255);
            pg = CLAMP((tg * fg) >> 7, 0, 255);
            pb = CLAMP((tb * fb) >> 7, 0, 255);
            pa = ta;

            // if (gs->ctx->tcc) {
            //     pa = CLAMP((ta * fa) >> 7, 0, 255);
            // } else {
            //     pa = fa;
            // }
        } break;
        case GS_DECAL: {
            pr = tr;
            pg = tg;
            pb = tb;
            pa = ta;
        } break;
        case GS_HIGHLIGHT: {
            pa = (f >> 24) & 0xff;
            pr = CLAMP(((tr * fr) >> 7) + pa, 0, 255);
            pg = CLAMP(((tg * fg) >> 7) + pa, 0, 255);
            pb = CLAMP(((tb * fb) >> 7) + pa, 0, 255);
        } break;
        case GS_HIGHLIGHT2: {
            switch (gs->ctx->tcc) {
                case 0: {
                    pa = (f >> 24) & 0xff;
                    pr = CLAMP(((tr * fr) >> 7) + pa, 0, 255);
                    pg = CLAMP(((tg * fg) >> 7) + pa, 0, 255);
                    pb = CLAMP(((tb * fb) >> 7) + pa, 0, 255);
                } break;

                case 1: {
                    pr = CLAMP(((tr * fr) >> 7) + fa, 0, 255);
                    pg = CLAMP(((tg * fg) >> 7) + fa, 0, 255);
                    pb = CLAMP(((tb * fb) >> 7) + fa, 0, 255);
                    pa = (t >> 24) & 0xff;
                } break;
            }
        } break;
    }

    return pr | (pg << 8) | (pb << 16) | (pa << 24);
}

enum : int {
    TR_FAIL,
    TR_PASS,
    TR_FB_ONLY,
    TR_ZB_ONLY,
    TR_RGB_ONLY
};

static inline uint32_t gs_read_fb(struct ps2_gs* gs, int x, int y) {
    switch (gs->ctx->fbpsm) {
        case GS_PSMCT32:
            return gs->vram[gs->ctx->fbp + x + (y * gs->ctx->fbw)];
        case GS_PSMCT24:
            return gs->vram[gs->ctx->fbp + x + (y * gs->ctx->fbw)] & 0xffffff;
        case GS_PSMCT16:
        case GS_PSMCT16S: {
            int shift = (x & 1) << 4;
            uint32_t mask = 0xffff << shift;
            uint32_t data = gs->vram[gs->ctx->fbp + (x >> 1) + (y * (gs->ctx->fbw >> 1))];

            return (data & mask) >> shift;
        }
    }

    return 0;
}

static inline uint32_t gs_read_zb(struct ps2_gs* gs, int x, int y) {
    switch (gs->ctx->zbpsm) {
        case GS_ZSMZ32:
            return gs->vram[(gs->ctx->zbp + x + (y * gs->ctx->fbw)) & 0xfffff];
        case GS_ZSMZ24:
            return gs->vram[gs->ctx->zbp + x + (y * gs->ctx->fbw)] & 0xffffff;
        case GS_ZSMZ16:
        case GS_ZSMZ16S: {
            int shift = (x & 1) << 4;
            uint32_t mask = 0xffff << shift;
            uint32_t data = gs->vram[gs->ctx->zbp + (x >> 1) + (y * (gs->ctx->fbw >> 1))];

            return (data & mask) >> shift;
        }
    }

    return 0;
}

static inline uint32_t gs_read_cb(struct ps2_gs* gs, int i) {
    // CSM2 should be the same?
    // not sure if the different arrangement actually
    // changes how address calculation is performed
    // maybe it only applies to the CLUT cache
    // - Edit: CSM2 uses the TEXCLUT reg, and it's
    // basically just like a normal buffer

    switch (gs->ctx->cbpsm) {
        case GS_PSMCT32: {
            int b = i >> 5;
            int p = psmct32_clut_block[i & 0x1f] + b;

            return gs->vram[gs->ctx->cbp + p];
        } break;
        case GS_PSMCT16:
        case GS_PSMCT16S: {
            int shift = (i & 1) << 4;

            uint32_t data = gs->vram[gs->ctx->cbp + (i >> 1)];

            return (data & (0xffff << shift)) >> shift;
        } break;
    }

    return 0;
}

static inline uint32_t gs_to_rgba32(struct ps2_gs* gs, uint32_t c, int fmt) {
    switch (fmt) {
        case GS_PSMCT32:
            return c;
        case GS_PSMCT24:
            return c | 0xff000000;
        case GS_PSMCT16:
        case GS_PSMCT16S: {
            return ((c & 0x001f) << 3) |
                   ((c & 0x03e0) << 6) |
                   ((c & 0x7c00) << 9) |
                   ((c & 0x8000) ? 0xff000000 : 0);
        } break;
        case GS_PSMT8:
        case GS_PSMT8H:
        case GS_PSMT4:
        case GS_PSMT4HL:
        case GS_PSMT4HH: {
            return gs_to_rgba32(gs, c, gs->ctx->cbpsm);
        } break;
    }

    return 0;
}

static inline uint32_t gs_from_rgba32(struct ps2_gs* gs, uint32_t c, int fmt) {
    switch (fmt) {
        case GS_PSMCT32:
            return c;
        case GS_PSMCT24:
            // To-do: Use TEXA
            return c & 0xffffff;
        case GS_PSMCT16:
        case GS_PSMCT16S: {
            // To-do: Use TEXA
            return ((c & 0x0000f8) >> 3) |
                   ((c & 0x00f800) >> 6) |
                   ((c & 0xf80000) >> 9) |
                   ((c & 0x80000000) ? 0x8000 : 0);
        } break;
        case GS_PSMT8:
        case GS_PSMT8H:
        case GS_PSMT4:
        case GS_PSMT4HL:
        case GS_PSMT4HH: {
            return gs_from_rgba32(gs, c, gs->ctx->cbpsm);
        } break;
    }

    return 0;
}

static inline uint32_t gs_read_tb(struct ps2_gs* gs, int u, int v) {
    switch (gs->ctx->tbpsm) {
        case GS_PSMCT32: {
            return gs->vram[(gs->ctx->tbp0 + u + (v * gs->ctx->tbw)) & 0xfffff];
        }
        case GS_PSMCT24:
            return gs->vram[gs->ctx->tbp0 + u + (v * gs->ctx->tbw)] & 0xffffff;
        case GS_PSMCT16:
        case GS_PSMCT16S: {
            int shift = (u & 1) << 4;

            uint32_t data = gs->vram[gs->ctx->tbp0 + (u >> 1) + (v * (gs->ctx->tbw >> 1))];

            return (data & (0xffff << shift)) >> shift;
        } break;
        case GS_PSMT8: {
            // 1 page = 128x64px block = 16x16px
            // 1 block = 256 bytes = 64 words
            // tbp = 0
            // <------------- 128 ------------->
            // +---+---+---+---+---+---+---+---+ ^
            // | 0 | 1 | 4 | 5 |16 |17 |20 |21 | |
            // +---+---+---+---+---+---+---+---+ |
            // | 2 | 3 | 6 | 7 |18 |19 |22 |23 | |
            // +---+---+---+---+---+---+---+---+ 64
            // | 8 | 9 |10 |11 |24 |25 |28 |29 | |
            // +---+---+---+---+---+---+---+---+ |
            // |12 |13 |14 |15 |26 |27 |30 |31 | |
            // +---+---+---+---+---+---+---+---+ v
            // tbp offset = unswizzle_block(u, v) * 64 words
            // blk offset = unswizzle_column(u, v)
            // addr = tbp + tbp offset + blk offset
            uint32_t addr = gs->ctx->tbp0 + (u >> 2) + (v * (gs->ctx->tbw >> 2));

            if (addr >= 0x100000)
                break;

            uint32_t data = gs->vram[gs->ctx->tbp0 + (u >> 2) + (v * (gs->ctx->tbw >> 2))];

            int shift = (u & 3) << 3;
            int index = (data & (0xff << shift)) >> shift;

            return gs_read_cb(gs, index);
        } break;
        case GS_PSMT8H: {
            uint32_t data = gs->vram[gs->ctx->tbp0 + u + (v * gs->ctx->tbw)];

            return gs_read_cb(gs, data >> 24);
        } break;
        case GS_PSMT4: {
            int shift = (u & 7) << 2;

            uint32_t addr = gs->ctx->tbp0 + (u >> 3) + (v * (gs->ctx->tbw >> 3));

            if (addr >= 0x100000)
                break;

            uint32_t data = gs->vram[addr];

            int index = (data & (0xf << shift)) >> shift;

            return gs_read_cb(gs, index);
        } break;
        case GS_PSMT4HL: {
            uint32_t data = gs->vram[gs->ctx->tbp0 + u + (v * gs->ctx->tbw)];

            return gs_read_cb(gs, (data >> 24) & 0xf);
        } break;
        case GS_PSMT4HH: {
            uint32_t data = gs->vram[gs->ctx->tbp0 + u + (v * gs->ctx->tbw)];

            return gs_read_cb(gs, data >> 28);
        } break;
    }

    return 0;
}

static inline void gs_write_fb(struct ps2_gs* gs, int x, int y, uint32_t c) {
    uint32_t f = gs_from_rgba32(gs, c, gs->ctx->fbpsm);

    // To-do: Implement FBMSK
    switch (gs->ctx->fbpsm) {
        case GS_PSMCT32: {
            gs->vram[gs->ctx->fbp + x + (y * gs->ctx->fbw)] = f;
        } break;

        case GS_PSMCT24: {
            gs->vram[gs->ctx->fbp + x + (y * gs->ctx->fbw)] = f | 0xff000000;
        } break;
        case GS_PSMCT16:
        case GS_PSMCT16S: {
            uint16_t* ptr = ((uint16_t*)&gs->vram[gs->ctx->fbp]) + x + (y * gs->ctx->fbw);

            *ptr = f;
        } break;
    }
}

static inline void gs_write_zb(struct ps2_gs* gs, int x, int y, uint32_t z) {
    if (gs->ctx->zbmsk)
        return;

    switch (gs->ctx->zbpsm) {
        case GS_ZSMZ32: {
            gs->vram[gs->ctx->zbp + x + (y * gs->ctx->fbw)] = z;
        } break;

        case GS_ZSMZ24: {
            gs->vram[gs->ctx->zbp + x + (y * gs->ctx->fbw)] = z;
        } break;
        case GS_ZSMZ16:
        case GS_ZSMZ16S: {
            uint16_t* ptr = (uint16_t*)&gs->vram[gs->ctx->zbp];

            *(ptr + x + (y * gs->ctx->fbw)) = z;
        } break;
    }
}

static inline int gs_test_scissor(struct ps2_gs* gs, int x, int y) {
    if (x < (int)gs->ctx->scax0 || y < (int)gs->ctx->scay0 ||
        x > (int)gs->ctx->scax1 || y > (int)gs->ctx->scay1) {
        return TR_FAIL;
    }

    return TR_PASS;
}

static inline int gs_test_pixel(struct ps2_gs* gs, int x, int y, uint32_t z, uint8_t a) {
    int tr = TR_PASS;

    // Alpha test
    if (gs->ctx->ate) {
        switch (gs->ctx->atst) {
            case 0: tr = TR_FAIL; break;
            case 2: tr = a < gs->ctx->aref; break;
            case 3: tr = a <= gs->ctx->aref; break;
            case 4: tr = a == gs->ctx->aref; break;
            case 5: tr = a >= gs->ctx->aref; break;
            case 6: tr = a > gs->ctx->aref; break;
            case 7: tr = a != gs->ctx->aref; break;
        }

        if (tr == TR_FAIL) {
            switch (gs->ctx->afail) {
                case 0: return TR_FAIL;
                case 1: tr = TR_FB_ONLY; break;
                case 2: tr = TR_ZB_ONLY; break;
                case 3: tr = TR_RGB_ONLY; break;
            }
        }
    }

    // Destination alpha test
    if (gs->ctx->date) {
        uint32_t s = gs_read_fb(gs, x, y);

        switch (gs->ctx->fbpsm) {
            case GS_PSMCT16:
            case GS_PSMCT16S:
                if (((s >> 15) & 1) != gs->ctx->datm) return TR_FAIL;
            break;
            case GS_PSMCT32:
                if (((s >> 31) & 1) != gs->ctx->datm) return TR_FAIL;
            break;
        }
    }

    // Depth test
    if (gs->ctx->zte) {
        uint32_t zb = gs_read_zb(gs, x, y);

        switch (gs->ctx->ztst) {
            case 0: return TR_FAIL;
            case 2: {
                if (z < zb) return TR_FAIL;
            } break;
            case 3: {
                if (z <= zb) return TR_FAIL;
            } break;
        }
    }

    return tr;
}

static inline int gs_clamp_u(struct ps2_gs* gs, int u) {
    switch (gs->ctx->wms) {
        case 0: {
            u %= gs->ctx->usize;
        } break;

        case 1: {
            u = (u < 0) ? 0 : ((u > (int)gs->ctx->usize) ? gs->ctx->usize : u);
        } break;

        case 2: {
            u = (u < (int)gs->ctx->minu) ? gs->ctx->minu : ((u > (int)gs->ctx->maxu) ? gs->ctx->maxu : u);
        } break;

        case 3: {
            int umsk = gs->ctx->minu;
            int ufix = gs->ctx->maxu;

            u = (u & umsk) | ufix;
        } break;
    }

    return u;
}

static inline int gs_clamp_v(struct ps2_gs* gs, int v) {
    switch (gs->ctx->wmt) {
        case 0: {
            v %= gs->ctx->vsize;
        } break;

        case 1: {
            v = (v < 0) ? 0 : ((v > (int)gs->ctx->vsize) ? gs->ctx->vsize : v);
        } break;

        case 2: {
            v = (v < (int)gs->ctx->minv) ? gs->ctx->minv : ((v > (int)gs->ctx->maxv) ? gs->ctx->maxv : v);
        } break;

        case 3: {
            int vmsk = gs->ctx->minv;
            int vfix = gs->ctx->maxv;

            v = (v & vmsk) | vfix;
        } break;
    }

    return v;
}

static inline uint32_t gs_alpha_blend(struct ps2_gs* gs, int x, int y, uint32_t s) {
    uint32_t d = gs_read_fb(gs, x, y);

    d = gs_to_rgba32(gs, d, gs->ctx->fbpsm);

    uint32_t av = (gs->ctx->a == 0) ? s : ((gs->ctx->a == 1) ? d : 0);
    uint32_t bv = (gs->ctx->b == 0) ? s : ((gs->ctx->b == 1) ? d : 0);
    uint32_t cv = (gs->ctx->c == 0) ? (s >> 24) : ((gs->ctx->c == 1) ? (d >> 24): gs->ctx->fix);
    uint32_t dv = (gs->ctx->d == 0) ? s : ((gs->ctx->d == 1) ? d : 0);

    // if (gs->tme)
    //     printf("Cd=%08x Cs=%08x a=%08x (%d) b=%08x (%d) c=%08x (%d) d=%08x (%d)\n", d, s,
    //     av, gs->ctx->a,
    //     bv, gs->ctx->b,
    //     cv, gs->ctx->c,
    //     dv, gs->ctx->d
    // );

    uint32_t ar = (av >> 0 ) & 0xff;
    uint32_t ag = (av >> 8 ) & 0xff;
    uint32_t ab = (av >> 16) & 0xff;
    uint32_t aa = (av >> 24) & 0xff;
    uint32_t br = (bv >> 0 ) & 0xff;
    uint32_t bg = (bv >> 8 ) & 0xff;
    uint32_t bb = (bv >> 16) & 0xff;
    uint32_t ba = (bv >> 24) & 0xff;
    uint32_t dr = (dv >> 0 ) & 0xff;
    uint32_t dg = (dv >> 8 ) & 0xff;
    uint32_t db = (dv >> 16) & 0xff;
    uint32_t da = (dv >> 24) & 0xff;

    uint32_t rr = (((ar - br) * cv) >> 7) + dr;
    uint32_t rg = (((ag - bg) * cv) >> 7) + dg;
    uint32_t rb = (((ab - bb) * cv) >> 7) + db;
    uint32_t ra = (((aa - ba) * cv) >> 7) + da;

    return (rr & 0xff) | ((rg & 0xff) << 8) | ((rb & 0xff) << 16) | ((ra & 0xff) << 24);
}

int software_compile_shader(const char* src, GLint type) {
    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int success;
    char infolog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infolog);

        printf("gs_opengl: Shader compilation failed\n%s", infolog);

        exit(1);

        return 0;
    }

    return shader;
}

char* software_read_file(const char* path) {
    FILE* file = fopen(path, "rb");

    unsigned int size;

    fseek(file, 0, SEEK_END);

    size = ftell(file);

    fseek(file, 0, SEEK_SET);

    char* buf = (char*)malloc(size + 1);

    buf[size] = '\0';

    if (!fread(buf, 1, size, file)) {
        printf("software: Couldn't read shader source\n");

        return buf;
    }

    return buf;
}

void software_init_default_shader(software_state* ctx) {
    ctx->vert_shader = software_compile_shader(default_vert_shader, GL_VERTEX_SHADER);
    GLuint frag_shader = software_compile_shader(default_frag_shader, GL_FRAGMENT_SHADER);

    char infolog[512];
    int success;

    ctx->default_program = glCreateProgram();

    glAttachShader(ctx->default_program, ctx->vert_shader);
    glAttachShader(ctx->default_program, frag_shader);
    glLinkProgram(ctx->default_program);
    glGetProgramiv(ctx->default_program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(ctx->default_program, 512, NULL, infolog);

        printf("opengl: Program linking failed\n%s", infolog);

        exit(1);
    }
}

void software_push_shader(software_state* ctx, const char* path) {
    char* src = software_read_file(path);
    char* hdr = (char*)malloc(strlen(src) + strlen(frag_header) + 1);

    strcpy(hdr, frag_header);
    strcat(hdr, src);

    int shader = software_compile_shader(hdr, GL_FRAGMENT_SHADER);

    char infolog[512];
    int success;

    GLuint program = glCreateProgram();

    glAttachShader(program, ctx->vert_shader);
    glAttachShader(program, shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infolog);

        printf("opengl: Program linking failed\n%s", infolog);

        exit(1);
    }

    ctx->programs.push_back(program);

    free(src);
    free(hdr);
}

void software_init(software_state* ctx, struct ps2_gs* gs, SDL_Window* window) {
    ctx->window = window;
    ctx->gs = gs;
    ctx->scale = 1.5f;

    gl3wInit();

    // Initialize default shaders
    software_init_default_shader(ctx);

    software_push_shader(ctx, "shaders/flip.frag");
    software_push_shader(ctx, "shaders/encoder.frag");
    software_push_shader(ctx, "shaders/decoder.frag");
    software_push_shader(ctx, "shaders/smooth.frag");

    // Initialize framebuffer VAO
    static const GLfloat fb_vertices[] = {
        // positions         // texture coords
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,   // top right
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,   // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,   // bottom left
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f    // top left 
    };

    static const GLuint fb_indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &ctx->fb_vao);
    glGenBuffers(1, &ctx->fb_vbo);
    glGenBuffers(1, &ctx->fb_ebo);

    glBindVertexArray(ctx->fb_vao);

    glBindBuffer(GL_ARRAY_BUFFER, ctx->fb_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fb_vertices), fb_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->fb_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fb_indices), fb_indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

static inline void software_vram_blit(struct ps2_gs* gs, software_state* ctx) {
    for (int y = 0; y < (int)ctx->rrh; y++) {
        uint32_t src = ctx->sbp + ctx->ssax + (ctx->ssay * ctx->sbw) + (y * ctx->sbw);
        uint32_t dst = ctx->dbp + ctx->dsax + (ctx->dsay * ctx->dbw) + (y * ctx->dbw);

        memcpy(gs->vram + dst, gs->vram + src, ctx->rrw * sizeof(uint32_t));
    }
}

extern "C" void software_render_point(struct ps2_gs* gs, void* udata) {
    struct gs_vertex vert = gs->vq[0];

    vert.x -= gs->ctx->ofx;
    vert.y -= gs->ctx->ofy;

    if (!gs_test_scissor(gs, vert.x, vert.y))
        return;

    int tr = gs_test_pixel(gs, vert.x, vert.y, vert.z, vert.a);

    if (tr == TR_FAIL)
        return;

    gs_write_fb(gs, vert.x, vert.y, vert.rgbaq & 0xffffffff);
}

extern "C" void software_render_line(struct ps2_gs* gs, void* udata) {
}

#define EDGE(a, b, c) ((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))
#define MIN2(a, b) ((((int)a) < ((int)b)) ? ((int)a) : ((int)b))
#define MIN3(a, b, c) (MIN2(MIN2(a, b), c))
#define MAX2(a, b) ((((int)a) > ((int)b)) ? ((int)a) : ((int)b))
#define MAX3(a, b, c) (MAX2(MAX2(a, b), c))

#define IS_TOPLEFT(a, b) ((b.y > a.y) || ((a.y == b.y) && (b.x < a.x)))

extern "C" void software_render_triangle(struct ps2_gs* gs, void* udata) {
    struct gs_vertex v0 = gs->vq[0];
    struct gs_vertex v1 = gs->vq[1];
    struct gs_vertex v2 = gs->vq[2];

    if (EDGE(v0, v1, v2) < 0) {
        v1 = gs->vq[2];
        v2 = gs->vq[1];
    }

    v0.x -= gs->ctx->ofx;
    v1.x -= gs->ctx->ofx;
    v2.x -= gs->ctx->ofx;
    v0.y -= gs->ctx->ofy;
    v1.y -= gs->ctx->ofy;
    v2.y -= gs->ctx->ofy;

    int xmin = MIN3(v0.x, v1.x, v2.x);
    int ymin = MIN3(v0.y, v1.y, v2.y);
    int xmax = MAX3(v0.x, v1.x, v2.x);
    int ymax = MAX3(v0.y, v1.y, v2.y);

    xmin = MAX2(xmin, gs->ctx->scax0);
    ymin = MAX2(ymin, gs->ctx->scay0);
    xmax = MIN2(xmax, gs->ctx->scax1);
    ymax = MIN2(ymax, gs->ctx->scay1);

    int a01 = (int)v0.y - (int)v1.y, b01 = (int)v1.x - (int)v0.x;
    int a12 = (int)v1.y - (int)v2.y, b12 = (int)v2.x - (int)v1.x;
    int a20 = (int)v2.y - (int)v0.y, b20 = (int)v0.x - (int)v2.x;

    struct gs_vertex p;
    p.x = xmin;
    p.y = ymin;

    int bias0 = IS_TOPLEFT(v1, v2) ? 0 : -1;
    int bias1 = IS_TOPLEFT(v2, v0) ? 0 : -1;
    int bias2 = IS_TOPLEFT(v0, v1) ? 0 : -1;
    int w0_row = EDGE(v1, v2, p) + bias0;
    int w1_row = EDGE(v2, v0, p) + bias1;
    int w2_row = EDGE(v0, v1, p) + bias2;

    // printf("triangle: v0=(%d,%d,%d) v1=(%d,%d,%d) v2=(%d,%d,%d) min=(%d,%d) max=(%d,%d) iip=%d tme=%d fst=%d abe=%d tfx=%d tcc=%d zte=%d\n",
    //     v0.x, v0.y, v0.z,
    //     v1.x, v1.y, v1.z,
    //     v2.x, v2.y, v2.z,
    //     xmin, ymin,
    //     xmax, ymax,
    //     gs->iip,
    //     gs->tme,
    //     gs->fst,
    //     gs->abe,
    //     gs->ctx->tfx,
    //     gs->ctx->tcc,
    //     gs->ctx->zte
    // );

    // printf("triangle: v0=(%d,%d,%d) v1=(%d,%d,%d) v2=(%d,%d,%d) min=(%d,%d) max=(%d,%d) sca0=(%d,%d) sca1=(%d,%d) offset=(%d,%d)\n",
    //     v0.x, v0.y, v0.z,
    //     v1.x, v1.y, v1.z,
    //     v2.x, v2.y, v2.z,
    //     xmin, ymin,
    //     xmax, ymax,
    //     gs->ctx->scax0, gs->ctx->scay0,
    //     gs->ctx->scax1, gs->ctx->scay1,
    //     gs->ctx->ofx, gs->ctx->ofy
    // );

    // printf("ZB format=%x zbp=%x zmsk=%d zte=%d ztst=%d\n",
    //     gs->ctx->zbpsm,
    //     gs->ctx->zbp,
    //     gs->ctx->zbmsk,
    //     gs->ctx->zte,
    //     gs->ctx->ztst
    // );

    int area = EDGE(v0, v1, v2);

    for (p.y = ymin; p.y <= ymax; p.y++) {
        // Barycentric coordinates at start of row
        int w0 = w0_row;
        int w1 = w1_row;
        int w2 = w2_row;

        for (p.x = xmin; p.x <= xmax; p.x++) {
            // If p is on or inside all edges, render pixel.
            if ((w0 | w1 | w2) < 0) {
                w0 += a12;
                w1 += a20;
                w2 += a01;

                continue;
            }

            // Calculate interpolation weights
            float iw0 = (float)w0 / (float)area;
            float iw1 = (float)w1 / (float)area;
            float iw2 = (float)w2 / (float)area;

            uint32_t fr, fg, fb, fa;

            if (gs->iip) {
                fr = v0.r * iw0 + v1.r * iw1 + v2.r * iw2;
                fg = v0.g * iw0 + v1.g * iw1 + v2.g * iw2;
                fb = v0.b * iw0 + v1.b * iw1 + v2.b * iw2;
                fa = v0.a * iw0 + v1.a * iw1 + v2.a * iw2;
            } else {
                fr = v2.r;
                fg = v2.g;
                fb = v2.b;
                fa = v2.a;
            }

            if (gs->tme) {
                float u, v;

                if (gs->fst) {
                    u = v0.u * iw0 + v1.u * iw1 + v2.u * iw2;
                    v = v0.v * iw0 + v1.v * iw1 + v2.v * iw2;
                } else {
                    float s = v0.s * iw0 + v1.s * iw1 + v2.s * iw2;
                    float t = v0.t * iw0 + v1.t * iw1 + v2.t * iw2;
                    float q = v0.q * iw0 + v1.q * iw1 + v2.q * iw2;

                    u = (s / q) * gs->ctx->usize;
                    v = (t / q) * gs->ctx->vsize;
                }

                u = gs_clamp_u(gs, u);
                v = gs_clamp_v(gs, v);

                uint32_t f = fr | (fg << 8) | (fb << 16);
                uint32_t t = gs_read_tb(gs, u, v);
                t = gs_to_rgba32(gs, t, gs->ctx->tbpsm);
                t = gs_apply_function(gs, t, f);

                fr = t & 0xff;
                fg = (t >> 8) & 0xff;
                fb = (t >> 16) & 0xff;
                fa = t >> 24;
            }

            int fz = (float)v0.z * iw0 + (float)v1.z * iw1 + (float)v2.z * iw2;

            int tr = gs_test_pixel(gs, p.x, p.y, fz, fa);

            if (tr == TR_FAIL) {
                w0 += a12;
                w1 += a20;
                w2 += a01;

                continue;
            }

            // To-do: Alpha blending

            uint32_t fc = fr | (fg << 8) | (fb << 16) | (fa << 24);

            switch (tr) {
                case TR_FB_ONLY: gs_write_fb(gs, p.x, p.y, fc); break;
                case TR_ZB_ONLY: gs_write_zb(gs, p.x, p.y, fz); break;
                // To-do: case TR_RGB_ONLY: gs_write_fb(gs, x, y, c); break;
                case TR_PASS: {
                    gs_write_fb(gs, p.x, p.y, fc);
                    gs_write_zb(gs, p.x, p.y, fz);
                } break;
            }

            // One step to the right
            w0 += a12;
            w1 += a20;
            w2 += a01;
        }

        // One row step
        w0_row += b12;
        w1_row += b20;
        w2_row += b01;
    }
}

extern "C" void software_render_sprite(struct ps2_gs* gs, void* udata) {
    struct gs_vertex v0 = gs->vq[0];
    struct gs_vertex v1 = gs->vq[1];

    v0.x -= gs->ctx->ofx;
    v1.x -= gs->ctx->ofx;
    v0.y -= gs->ctx->ofy;
    v1.y -= gs->ctx->ofy;

    int xmin = (int32_t)std::min(v0.x, v1.x);
    int ymin = (int32_t)std::min(v0.y, v1.y);
    int xmax = (int32_t)std::max(v0.x, v1.x);
    int ymax = (int32_t)std::max(v0.y, v1.y);

    int z = v1.z;
    int a = v1.a;

    // printf("gs: Sprite at (%d,%d-%d,%d) min=(%d,%d) max=(%d,%d) zte=%d ztst=%d zmsk=%d z=%d zbp=%x fbp=%x tme=%d abe=%d rgba=%08lx scissor=(%d,%d-%d,%d) ctx=%d\n",
    //     v0.x, v0.y,
    //     v1.x, v1.y,
    //     xmin, ymin,
    //     xmax, ymax,
    //     gs->ctx->zte,
    //     gs->ctx->ztst,
    //     gs->ctx->zbmsk,
    //     z,
    //     gs->ctx->zbp,
    //     gs->ctx->fbp,
    //     gs->tme,
    //     gs->abe,
    //     v1.rgbaq & 0xffffffff,
    //     gs->ctx->scax0, gs->ctx->scay0,
    //     gs->ctx->scax1, gs->ctx->scay1,
    //     gs->ctxt
    // );

    // if (gs->tme) {
    //     printf("gs: TB format=%d (0x%02x) tbp=%x tbw=%d uv=(%d,%d) TEX w=%d h=%d CB format=%d cbp=%x csm=%d tfx=%d rgba=%08lx abe=%d FB format=%d fbw=%d fba=%ld\n",
    //         gs->ctx->tbpsm,
    //         gs->ctx->tbpsm,
    //         gs->ctx->tbp0,
    //         gs->ctx->tbw,
    //         v0.u,
    //         v0.v,
    //         gs->ctx->usize,
    //         gs->ctx->vsize,
    //         gs->ctx->cbpsm,
    //         gs->ctx->cbp,
    //         gs->ctx->csm,
    //         gs->ctx->tfx,
    //         v1.rgbaq & 0xffffffff,
    //         gs->abe,
    //         gs->ctx->fbpsm,
    //         gs->ctx->fbw,
    //         gs->ctx->fba// ,
    //         // gs->ctx->a,
    //         // gs->ctx->b,
    //         // gs->ctx->c,
    //         // gs->ctx->d,
    //         // gs->ctx->tcc,
    //         // gs->ctx->fix
    //     );
    // }

    // if (!gs->fst) {
    //     printf("gs: stq0=(%f,%f,%f) stq1=(%f,%f,%f)\n",
    //         v0.s, v0.t, v0.q,
    //         v1.s, v1.t, v1.q
    //     );
    // } else {
    //     printf("gs: uv0=(%d,%d) uv1=(%d,%d)\n",
    //         v0.u, v0.v,
    //         v1.u, v1.v
    //     );
    // }

    float u = v0.u;
    float v = v0.v;

    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            if (!gs_test_scissor(gs, x, y))
                continue;

            uint32_t c = v1.rgbaq & 0xffffffff;

            if (gs->tme) {
                float tx = (float)(x - xmin) / (float)(xmax - xmin);
                float ty = (float)(y - ymin) / (float)(ymax - ymin);

                if (gs->fst) {
                    u = v0.u + (v1.u - v0.u) * tx;
                    v = v0.v + (v1.v - v0.v) * ty;
                } else {
                    u = v0.s + (v1.s - v0.s) * tx;
                    v = v0.t + (v1.t - v0.t) * ty;
                    
                    u *= gs->ctx->usize;
                    v *= gs->ctx->vsize;
                }

                u = gs_clamp_u(gs, u);
                v = gs_clamp_v(gs, v);

                uint32_t f = c;

                c = gs_read_tb(gs, u, v);
                c = gs_to_rgba32(gs, c, gs->ctx->tbpsm);
                c = gs_apply_function(gs, c, f);

                a = c >> 24;
            }

            int tr = gs_test_pixel(gs, x, y, z, a);

            if (tr == TR_FAIL)
                continue;

            if (gs->abe) {
                c = gs_alpha_blend(gs, x, y, c);
            }

            switch (tr) {
                case TR_FB_ONLY: gs_write_fb(gs, x, y, c); break;
                case TR_ZB_ONLY: gs_write_zb(gs, x, y, z); break;
                // To-do: case TR_RGB_ONLY: gs_write_fb(gs, x, y, c); break;
                case TR_PASS: {
                    gs_write_fb(gs, x, y, c);
                    gs_write_zb(gs, x, y, z);
                } break;
            }
        }
    }
}

extern "C" void software_render(struct ps2_gs* gs, void* udata) {
    software_state* ctx = (software_state*)udata;

    int en1 = ctx->gs->pmode & 1;

    uint64_t dispfb = en1 ? ctx->gs->dispfb1 : ctx->gs->dispfb2;

    // if ((gs->csr & 0x2000) && en2) {
    //     display = ctx->gs->display2;
    //     dispfb = ctx->gs->dispfb2;
    // }

    // gs->csr ^= 0x2000;

    uint32_t dfbp = (dispfb & 0x1ff) << 11;
    uint32_t* ptr = &gs->vram[dfbp];

    // printf("fbp=%x fbw=%d fbpsm=%d stride=%d dy=%d\n", dfbp, dfbw, dfbpsm, stride, dy);

    if (!ctx->tex_w)
        return;

    SDL_Rect size, rect;

    rect.w = ctx->tex_w;
    rect.h = ctx->tex_h;

    SDL_GetWindowSize(ctx->window, &size.w, &size.h);

    float scale = ctx->integer_scaling ? floorf(ctx->scale) : ctx->scale;

    switch (ctx->aspect_mode) {
        case SOFTWARE_ASPECT_NATIVE: {
            rect.w *= scale;
            rect.h *= scale;
        } break;

        case SOFTWARE_ASPECT_4_3: {
            rect.w *= scale;
            rect.h = (float)rect.w * (3.0f / 4.0f);
        } break;

        case SOFTWARE_ASPECT_16_9: {
            rect.w *= scale;
            rect.h = (float)rect.w * (9.0f / 16.0f);
        } break;

        case SOFTWARE_ASPECT_STRETCH: {
            rect.w = size.w;
            rect.h = size.h;
        } break;

        case SOFTWARE_ASPECT_AUTO:
        case SOFTWARE_ASPECT_STRETCH_KEEP: {
            rect.h = size.h;
            rect.w = (float)rect.h * (4.0f / 3.0f);
        } break;
    }

    ctx->disp_w = rect.w;
    ctx->disp_h = rect.h;

    rect.x = (size.w / 2) - (rect.w / 2);
    rect.y = (size.h / 2) - (rect.h / 2);

    float x0 = (rect.x / ((float)size.w / 2.0f)) - 1.0f;
    float y0 = (rect.y / ((float)size.h / 2.0f)) - 1.0f;
    float x1 = ((rect.x + rect.w) / ((float)size.w / 2.0f)) - 1.0f;
    float y1 = ((rect.y + rect.h) / ((float)size.h / 2.0f)) - 1.0f;

    GLfloat vertices[] = {
        // positions   // texture coords
        x1, y1, 0.0f,  1.0f, 1.0f,   // top right
        x1, y0, 0.0f,  1.0f, 0.0f,   // bottom right
        x0, y0, 0.0f,  0.0f, 0.0f,   // bottom left
        x0, y1, 0.0f,  0.0f, 1.0f    // top left 
    };

    GLuint indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &ctx->vao);
    glGenBuffers(1, &ctx->vbo);
    glGenBuffers(1, &ctx->ebo);

    glBindVertexArray(ctx->vao);

    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int internal_format, format;

    switch (ctx->disp_fmt) {
        case GS_PSMCT32:
        case GS_PSMCT24: {
            internal_format = GL_RGBA;
            format = GL_UNSIGNED_BYTE;
        } break;
        case GS_PSMCT16:
        case GS_PSMCT16S: {
            internal_format = GL_RGBA;
            format = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        } break;
    }

    // Update screen texture
    glBindTexture(GL_TEXTURE_2D, ctx->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, ctx->tex_w, ctx->tex_h, 0, internal_format, format, ptr);

    if (!ctx->programs.size()) {
        // No shaders present, use default shader
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx->tex);

        glViewport(0, 0, size.w, size.h);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(ctx->default_program);

        glUniform1i(glGetUniformLocation(ctx->default_program, "input_texture"), 0);
        glUniform2f(glGetUniformLocation(ctx->default_program, "screen_size"), (float)rect.w, (float)rect.h);
        glUniform1i(glGetUniformLocation(ctx->default_program, "frame"), ctx->frame++);

        glBindVertexArray(ctx->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        return;
    } else if (ctx->programs.size() == 1) {
        // Only 1 shader present, use default framebuffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx->tex);

        glViewport(0, 0, size.w, size.h);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(ctx->programs[0]);

        glUniform1i(glGetUniformLocation(ctx->programs[0], "input_texture"), 0);
        glUniform2f(glGetUniformLocation(ctx->programs[0], "screen_size"), (float)rect.w, (float)rect.h);
        glUniform1i(glGetUniformLocation(ctx->programs[0], "frame"), ctx->frame++);

        glBindVertexArray(ctx->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        return;
    }

    // More than 1 shader present, create framebuffer and do render graph
    if (ctx->fb_out_tex) {
        glDeleteTextures(1, &ctx->fb_out_tex);
    }

    glGenTextures(1, &ctx->fb_out_tex);
    glBindTexture(GL_TEXTURE_2D, ctx->fb_out_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, rect.w, rect.h, 0, internal_format, format, NULL);

    if (ctx->fb_in_tex) {
        glDeleteTextures(1, &ctx->fb_in_tex);
    }

    glGenTextures(1, &ctx->fb_in_tex);
    glBindTexture(GL_TEXTURE_2D, ctx->fb_in_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, rect.w, rect.h, 0, internal_format, format, NULL);

    if (ctx->fbo) {
        glDeleteFramebuffers(1, &ctx->fbo);
    }

    glGenFramebuffers(1, &ctx->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->fb_out_tex, 0); 

    // first pass
    // Bind screen texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx->tex);

    glViewport(0, 0, rect.w, rect.h);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo);
    glUseProgram(ctx->programs[0]);

    glUniform1i(glGetUniformLocation(ctx->programs[0], "input_texture"), 0);
    glUniform2f(glGetUniformLocation(ctx->programs[0], "screen_size"), (float)ctx->tex_w, (float)ctx->tex_h);
    glUniform1i(glGetUniformLocation(ctx->programs[0], "frame"), ctx->frame);

    glBindVertexArray(ctx->fb_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    int output_tex, input_tex;

    // "middle" passes
    for (int i = 1; i < (int)ctx->programs.size() - 1; i++) {
        // Bind framebuffer texture for middle and last passes
        input_tex = (i & 1) ? ctx->fb_out_tex : ctx->fb_in_tex;
        output_tex = (i & 1) ? ctx->fb_in_tex : ctx->fb_out_tex;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_tex, 0);
    
        glUseProgram(ctx->programs[i]);

        glUniform1i(glGetUniformLocation(ctx->programs[i], "input_texture"), 0);
        glUniform2f(glGetUniformLocation(ctx->programs[i], "screen_size"), (float)rect.w, (float)rect.h);
        glUniform1i(glGetUniformLocation(ctx->programs[i], "frame"), ctx->frame);

        glBindVertexArray(ctx->fb_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, output_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ctx->bilinear ? GL_LINEAR : GL_NEAREST);

    // last pass
    glViewport(0, 0, size.w, size.h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(ctx->programs.back());

    glUniform1i(glGetUniformLocation(ctx->programs.back(), "input_texture"), 0);
    glUniform2f(glGetUniformLocation(ctx->programs.back(), "screen_size"), (float)rect.w, (float)rect.h);
    glUniform1i(glGetUniformLocation(ctx->programs.back(), "frame"), ctx->frame++);

    glBindVertexArray(ctx->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

static inline int gs_pixels_to_size(int fmt, int px) {
    switch (fmt) {
        case GS_PSMCT32:
        case GS_PSMCT24:
        case GS_PSMT8H:
        case GS_PSMT4HL:
        case GS_PSMT4HH:
            return px;
        case GS_PSMCT16:
        case GS_PSMCT16S:
            return px >> 1;
        case GS_PSMT8:
            return px >> 2;
        case GS_PSMT4:
            return px >> 3;
    }
    return px;
}

extern "C" void software_transfer_start(struct ps2_gs* gs, void* udata) {
    software_state* ctx = (software_state*)udata;

    ctx->dbp = (gs->bitbltbuf >> 32) & 0x3fff;
    ctx->dbw = (gs->bitbltbuf >> 48) & 0x3f;
    ctx->dpsm = (gs->bitbltbuf >> 56) & 0x3f;
    ctx->sbp = (gs->bitbltbuf >> 0) & 0x3fff;
    ctx->sbw = (gs->bitbltbuf >> 16) & 0x3f;
    ctx->spsm = (gs->bitbltbuf >> 24) & 0x3f;
    ctx->dsax = (gs->trxpos >> 32) & 0x7ff;
    ctx->dsay = (gs->trxpos >> 48) & 0x7ff;
    ctx->ssax = (gs->trxpos >> 0) & 0x7ff;
    ctx->ssay = (gs->trxpos >> 16) & 0x7ff;
    ctx->dir = (gs->trxpos >> 59) & 3;
    ctx->rrw = (gs->trxreg >> 0) & 0xfff;
    ctx->rrh = (gs->trxreg >> 32) & 0xfff;
    ctx->xdir = gs->trxdir & 3;
    ctx->dx = 0;
    ctx->dy = 0;
    ctx->sx = 0;
    ctx->sy = 0;
    ctx->px = 0;

    ctx->dbp <<= 6;
    ctx->dbw <<= 6;
    ctx->sbp <<= 6;
    ctx->sbw <<= 6;

    uint32_t dbw = ctx->dbw;

    ctx->dbw = gs_pixels_to_size(ctx->dpsm, ctx->dbw);
    ctx->sbw = gs_pixels_to_size(ctx->spsm, ctx->sbw);

    ctx->psmct24_data = 0;
    ctx->psmct24_shift = 0;

    printf("dbp=%x dbw=%d (%d) dpsm=%02x dsa=(%d,%d) rr=(%d,%d)\n",
        ctx->dbp,
        ctx->dbw, dbw,
        ctx->dpsm,
        ctx->dsax,
        ctx->dsay,
        ctx->rrw,
        ctx->rrh
    );

    if (ctx->xdir == 2) {
        software_vram_blit(gs, ctx);
    }
}

static inline void gs_write_psmt4(struct ps2_gs* gs, software_state* ctx, uint32_t index) {
    uint32_t addr = ctx->dbp + ctx->dsax + (ctx->dsay * ctx->dbw);

    addr += (ctx->dx >> 3) + (ctx->dy * ctx->dbw);
    
    uint32_t mask = 0xf << ((ctx->dx & 7) * 4);
    uint32_t data = gs->vram[addr] & ~mask;

    gs->vram[addr] = data | (index << ((ctx->dx & 7) * 4));

    ctx->dx++;

    if (ctx->dx == ctx->rrw) {
        ctx->dx = 0;
        ctx->dy++;
    }
}

static inline void gs_write_psmt8(struct ps2_gs* gs, software_state* ctx, uint32_t index) {
    uint32_t addr = ctx->dbp + ctx->dsax + (ctx->dsay * ctx->dbw);

    addr += (ctx->dx >> 2) + (ctx->dy * ctx->dbw);
    
    uint32_t mask = 0xff << ((ctx->dx & 3) * 8);
    uint32_t data = gs->vram[addr] & ~mask;

    gs->vram[addr] = data | (index << ((ctx->dx & 3) * 8));

    ctx->dx++;

    if (ctx->dx == ctx->rrw) {
        ctx->dx = 0;
        ctx->dy++;
    }
}

static inline void gs_store_hwreg_psmt4(struct ps2_gs* gs, software_state* ctx) {
    for (int i = 0; i < 16; i++) {
        uint64_t index = (gs->hwreg >> (i * 4)) & 0xf;

        gs_write_psmt4(gs, ctx, index);
    }
}

static inline void gs_store_hwreg_psmt8(struct ps2_gs* gs, software_state* ctx) {
    for (int i = 0; i < 8; i++) {
        uint64_t index = (gs->hwreg >> (i * 8)) & 0xff;

        gs_write_psmt8(gs, ctx, index);
    }
}

static inline void gs_store_hwreg_psmct32(struct ps2_gs* gs, software_state* ctx) {
    uint32_t addr = ctx->dbp + ctx->dsax + (ctx->dsay * ctx->dbw);

    uint64_t data[2] = {
        gs->hwreg & 0xffffffff,
        gs->hwreg >> 32
    };

    for (int i = 0; i < 2; i++) {
        gs->vram[(addr + ctx->dx++ + (ctx->dy * ctx->dbw)) & 0xfffff] = data[i];

        if (ctx->dx == ctx->rrw) {
            ctx->dx = 0;
            ctx->dy++;
        }
    }
}

static inline void gs_psmct24_write(struct ps2_gs* gs, software_state* ctx, uint32_t data) {
    uint32_t addr = ctx->dbp + ctx->dsax + (ctx->dsay * ctx->dbw);

    gs->vram[addr + (ctx->dx++) + (ctx->dy * ctx->dbw)] = data;

    if (ctx->dx == ctx->rrw) {
        ctx->dx = 0;
        ++ctx->dy;
    }
}

static inline void gs_store_hwreg_psmct24(struct ps2_gs* gs, software_state* ctx) {
    switch (ctx->psmct24_shift++) {
        case 0: {
            gs_psmct24_write(gs, ctx, gs->hwreg & 0xffffff);
            gs_psmct24_write(gs, ctx, (gs->hwreg >> 24) & 0xffffff);
            
            ctx->psmct24_data = gs->hwreg >> 48;
        } break;

        case 1: {
            gs_psmct24_write(gs, ctx, ctx->psmct24_data | ((gs->hwreg & 0xff) << 16));
            gs_psmct24_write(gs, ctx, (gs->hwreg >> 8) & 0xffffff);
            gs_psmct24_write(gs, ctx, (gs->hwreg >> 32) & 0xffffff);

            ctx->psmct24_data = gs->hwreg >> 56;
        } break;

        case 2: {
            gs_psmct24_write(gs, ctx, ctx->psmct24_data | ((gs->hwreg & 0xffff) << 8));
            gs_psmct24_write(gs, ctx, (gs->hwreg >> 16) & 0xffffff);
            gs_psmct24_write(gs, ctx, (gs->hwreg >> 40) & 0xffffff);

            ctx->psmct24_shift = 0;
        } break;
    }
}

static inline void gs_write_psmct16(struct ps2_gs* gs, software_state* ctx, uint32_t index) {
    uint32_t addr = ctx->dbp + ctx->dsax + (ctx->dsay * ctx->dbw);

    addr += (ctx->dx >> 1) + (ctx->dy * ctx->dbw);

    uint32_t mask = 0xffff << ((ctx->dx & 1) * 16);
    uint32_t data = gs->vram[addr] & ~mask;

    gs->vram[addr] = data | (index << ((ctx->dx & 1) * 16));

    ctx->dx++;

    if (ctx->dx == ctx->rrw) {
        ctx->dx = 0;
        ctx->dy++;
    }
}

static inline void gs_store_hwreg_psmct16(struct ps2_gs* gs, software_state* ctx) {
    for (int i = 0; i < 4; i++) {
        uint64_t p = (gs->hwreg >> (i * 16)) & 0xffff;

        gs_write_psmct16(gs, ctx, p);
    }
}

extern "C" void software_transfer_write(struct ps2_gs* gs, void* udata) {
    software_state* ctx = (software_state*)udata;

    switch (ctx->dpsm) {
        case GS_PSMCT32: {
            gs_store_hwreg_psmct32(gs, ctx);
        } break;

        case GS_PSMCT24: {
            gs_store_hwreg_psmct24(gs, ctx);
        } break;

        case GS_PSMCT16:
        case GS_PSMCT16S: {
            gs_store_hwreg_psmct16(gs, ctx);
        } break;

        case GS_PSMT8: {
            gs_store_hwreg_psmt8(gs, ctx);
        } break;

        case GS_PSMT4: {
            gs_store_hwreg_psmt4(gs, ctx);
        } break;

        default: {
            gs_store_hwreg_psmct32(gs, ctx);
        } break;
    }
}

extern "C" void software_transfer_read(struct ps2_gs* gs, void* udata) {
    gs->hwreg = 0;
}
