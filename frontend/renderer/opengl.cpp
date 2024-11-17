#include <cstdlib>
#include <cstdio>

#include "opengl.hpp"
#include "ee/gs.h"

const char* vert_shader_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "layout (location = 1) in vec3 col;\n"
    "uniform vec2 screen_size;\n"
    "out vec3 VertColor;\n"
    "out vec2 FragCoord;\n"
    "void main() {\n"
    "    VertColor = col / 255.0;\n"
    "    vec2 p = pos.xy;\n"
    "    p.x = ((pos.x * 2.0) - screen_size.x) / screen_size.x;\n"
    "    p.y = (screen_size.y - (pos.y * 2.0)) / screen_size.y;\n"
    "    FragCoord = p;\n"
    "    gl_Position = vec4(p, 0.0, 1.0);\n"
    "}";

const char* frag_shader_src =
    "#version 330 core\n"
    "in vec3 VertColor;\n"
    "in vec2 FragCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D vram;\n"
    "void main() {\n"
    "   vec2 c = vec2(FragCoord.x, FragCoord.y / 4.0);\n"
    "   vec4 v = texture(vram, c);\n"
    "   FragColor = vec4(v.xyz, 1.0f);\n"
    "}";

static void opengl_push_primitive(opengl_state* ctx, int type, int verts) {
    opengl_primitive p;

    p.count = verts;
    p.offset = ctx->verts.size();
    p.type = type;

    if (!ctx->primitives.size()) {
        ctx->primitives.push_back(p);

        return;
    }

    opengl_primitive& q = ctx->primitives.back();

    if (q.type == type) {
        q.count += verts;

        return;
    }

    ctx->primitives.push_back(p);
}

int compile_shader(const char* src, GLint type) {
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

static inline opengl_vertex get_vertex_from_vq(struct ps2_gs* gs, int i) {
    opengl_vertex v;

    v.x = (gs->vq[i].xyz2 & 0xffff) >> 4;
    v.y = (gs->vq[i].xyz2 & 0xffff0000) >> 20;
    v.z = gs->vq[i].xyz2 >> 32;
    v.r = gs->vq[i].rgbaq & 0xff;
    v.g = (gs->vq[i].rgbaq >> 8) & 0xff;
    v.b = (gs->vq[i].rgbaq >> 16) & 0xff;

    // v.x += (gs->xyoffset_1 & 0xffff) >> 4;
    // v.y += (gs->xyoffset_1 & 0xffff0000) >> 20;

    return v;
}

char* opengl_read_file(const char* path) {
    FILE* file = fopen(path, "rb");

    unsigned int size;

    fseek(file, 0, SEEK_END);

    size = ftell(file);

    fseek(file, 0, SEEK_SET);

    char* buf = (char*)malloc(size + 1);

    buf[size] = '\0';

    fread(buf, 1, size, file);

    return buf;
}

void opengl_init(opengl_state* ctx) {
    gl3wInit();

    char* vert_shader_src1 = opengl_read_file("shaders/default.vert");
    char* frag_shader_src1 = opengl_read_file("shaders/default.frag");

    int vert_shader = compile_shader(vert_shader_src1, GL_VERTEX_SHADER);
    int frag_shader = compile_shader(frag_shader_src1, GL_FRAGMENT_SHADER);

    char infolog[512];
    int success;

    ctx->width = 640;
    ctx->height = 480;
    ctx->program = glCreateProgram();

    glAttachShader(ctx->program, vert_shader);
    glAttachShader(ctx->program, frag_shader);
    glLinkProgram(ctx->program);
    glGetProgramiv(ctx->program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(ctx->program, 512, NULL, infolog);

        printf("opengl: Program linking failed\n%s", infolog);

        exit(1);
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    glGenBuffers(1, &ctx->vbo);
    glGenVertexArrays(1, &ctx->vao);

    glGenTextures(1, &ctx->texture);
    glBindTexture(GL_TEXTURE_2D, ctx->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void opengl_set_size(opengl_state* ctx, int width, int height, float scale) {
    ctx->width = width;
    ctx->height = height;

    glViewport(0, 0, 640 * scale, 480 * scale);
}

extern "C" void opengl_render_point(struct ps2_gs* gs, void* udata) {
    opengl_state* ctx = (opengl_state*)udata;

    opengl_push_primitive(ctx, GL_POINTS, 1);

    ctx->verts.push_back(get_vertex_from_vq(gs, 0));
}

extern "C" void opengl_render_line(struct ps2_gs* gs, void* udata) {
    opengl_state* ctx = (opengl_state*)udata;

    opengl_push_primitive(ctx, GL_LINES, 2);

    ctx->verts.push_back(get_vertex_from_vq(gs, 0));
    ctx->verts.push_back(get_vertex_from_vq(gs, 1));
}

extern "C" void opengl_render_line_strip(struct ps2_gs* gs, void* udata) {

}

extern "C" void opengl_render_triangle(struct ps2_gs* gs, void* udata) {
    opengl_state* ctx = (opengl_state*)udata;

    opengl_vertex v0 = get_vertex_from_vq(gs, 0);
    opengl_vertex v1 = get_vertex_from_vq(gs, 1);
    opengl_vertex v2 = get_vertex_from_vq(gs, 2);

    // printf("v0=(%f,%f,%f) v1=(%f,%f,%f) v2=(%f,%f,%f)\n",
    //     v0.x, v0.y, v0.z,
    //     v1.x, v1.y, v1.z,
    //     v2.x, v2.y, v2.z
    // );

    opengl_push_primitive(ctx, GL_TRIANGLES, 3);

    ctx->verts.push_back(v0);
    ctx->verts.push_back(v1);
    ctx->verts.push_back(v2);
}

extern "C" void opengl_render_triangle_strip(struct ps2_gs* gs, void* udata) {
}

extern "C" void opengl_render_triangle_fan(struct ps2_gs* gs, void* udata) {

}

extern "C" void opengl_render_sprite(struct ps2_gs* gs, void* udata) {
    opengl_state* ctx = (opengl_state*)udata;

    opengl_push_primitive(ctx, GL_TRIANGLES, 6);

    opengl_vertex v0 = get_vertex_from_vq(gs, 0);
    opengl_vertex v1 = get_vertex_from_vq(gs, 1);

    printf("opengl: Sprite at v0=(%f,%f,%f) v1=(%f,%f,%f)\n",
        v0.x, v0.y, v0.z,
        v1.x, v1.y, v1.z
    );

    ctx->verts.push_back({ v0.x, v0.y, 0.0f, v0.r, v0.g, v0.b });
    ctx->verts.push_back({ v1.x, v0.y, 0.0f, v0.r, v0.g, v0.b });
    ctx->verts.push_back({ v0.x, v1.y, 0.0f, v0.r, v0.g, v0.b });
    ctx->verts.push_back({ v1.x, v0.y, 0.0f, v1.r, v1.g, v1.b });
    ctx->verts.push_back({ v1.x, v1.y, 0.0f, v1.r, v1.g, v1.b });
    ctx->verts.push_back({ v0.x, v1.y, 0.0f, v1.r, v1.g, v1.b });
}

extern "C" void opengl_render(struct ps2_gs* gs, void* udata) {
    opengl_state* ctx = (opengl_state*)udata;

    // glClearColor(0.0, 0.0, 0.0, 1.0);
    // glClear(GL_COLOR_BUFFER_BIT);

    // Send VRAM
    glBindTexture(GL_TEXTURE_2D, ctx->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, gs->vram);

    glBindVertexArray(ctx->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBufferData(GL_ARRAY_BUFFER, ctx->verts.size() * sizeof(opengl_vertex), ctx->verts.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int screen_size = glGetUniformLocation(ctx->program, "screen_size");
    int vram = glGetUniformLocation(ctx->program, "vram");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx->texture);
    glUseProgram(ctx->program);
    glUniform2f(screen_size, ctx->width, ctx->height);
    glUniform1i(vram, 0);

    for (opengl_primitive& p : ctx->primitives) {
        glDrawArrays(p.type, p.offset, p.count);
    }

    ctx->primitives.clear();
    ctx->verts.clear();

    // opengl_push_primitive(ctx, GL_TRIANGLES, 6);

    opengl_vertex v0 = { 0.0, 0.0, 0.0 };
    opengl_vertex v1 = { 639.0, 479.0, 0.0 };

    // printf("opengl: Sprite at v0=(%f,%f,%f) v1=(%f,%f,%f)\n",
    //     v0.x, v0.y, v0.z,
    //     v1.x, v1.y, v1.z
    // );

    // ctx->verts.push_back({ v0.x, v0.y, 0.0f, v0.r, v0.g, v0.b });
    // ctx->verts.push_back({ v1.x, v0.y, 0.0f, v0.r, v0.g, v0.b });
    // ctx->verts.push_back({ v0.x, v1.y, 0.0f, v0.r, v0.g, v0.b });
    // ctx->verts.push_back({ v1.x, v0.y, 0.0f, v1.r, v1.g, v1.b });
    // ctx->verts.push_back({ v1.x, v1.y, 0.0f, v1.r, v1.g, v1.b });
    // ctx->verts.push_back({ v0.x, v1.y, 0.0f, v1.r, v1.g, v1.b });
}