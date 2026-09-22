#pragma once

#include <vector>

#include "glcore.h"
#include "math4d.h"

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

inline Color mixColor(Color a, Color b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return Color{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t,
                 a.a + (b.a - a.a) * t};
}

inline Color scaleColor(Color c, float k) { return Color{c.r * k, c.g * k, c.b * k, c.a}; }

struct Vertex {
    Vec3 pos;
    Vec3 nrm;
    Color col;
};

class Shader {
public:
    bool build(const char* vs, const char* fs, char* log, int logSize);
    void use() const;
    GLint loc(const char* name) const;
    GLuint id() const { return id_; }

private:
    GLuint id_ = 0;
};

// cpu side geometry rebuilt every frame
struct Mesh {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    void clear();
    void tube(const Vec3* pts, const Color* cols, int count, float radius, int sides);
    void sphere(Vec3 center, float radius, Color col, int rings, int sectors);
    void disk(const Vec3* rim, int count, Vec3 center, Color col);
};

class GpuMesh {
public:
    void create();
    void destroy();
    void upload(const Mesh& mesh);
    void draw() const;

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLsizei count_ = 0;
    size_t vboCap_ = 0;
    size_t eboCap_ = 0;
};

extern const char* kSolidVs;
extern const char* kSolidFs;
extern const char* kGlowFs;
extern const char* kTextVs;
extern const char* kTextFs;
