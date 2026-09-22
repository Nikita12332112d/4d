#pragma once

#include <vector>

#include "gfx.h"

// ascii atlas baked from a system font with gdi
class TextRenderer {
public:
    bool create(int pixelHeight, char* log, int logSize);
    void destroy();

    void begin(int screenW, int screenH);
    void draw(float x, float y, const char* text, Color col, float scale = 1.0f);
    void flush();

    float charW(float scale = 1.0f) const { return cellW_ * scale; }
    float charH(float scale = 1.0f) const { return cellH_ * scale; }

private:
    void pushQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1,
                  Color col);

    Shader shader_;
    GLuint tex_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    size_t cap_ = 0;

    float cellW_ = 8.0f;
    float cellH_ = 16.0f;
    int atlasW_ = 0;
    int atlasH_ = 0;

    Mat4 proj_;
    std::vector<float> batch_;
};
