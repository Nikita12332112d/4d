#include "gfx.h"

#include <cstdio>
#include <cstring>

const char* kSolidVs = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNrm;
layout(location = 2) in vec4 aCol;
uniform mat4 uViewProj;
out vec3 vPos;
out vec3 vNrm;
out vec4 vCol;
void main() {
    vPos = aPos;
    vNrm = aNrm;
    vCol = aCol;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)";

const char* kSolidFs = R"(#version 330 core
in vec3 vPos;
in vec3 vNrm;
in vec4 vCol;
uniform vec3 uEye;
out vec4 oColor;
void main() {
    vec3 n = normalize(vNrm);
    vec3 v = normalize(uEye - vPos);
    if (dot(n, v) < 0.0) n = -n;
    vec3 l = normalize(vec3(0.42, 0.78, 0.55));
    float diff = max(dot(n, l), 0.0);
    vec3 h = normalize(l + v);
    float spec = pow(max(dot(n, h), 0.0), 42.0);
    float rim = pow(1.0 - max(dot(n, v), 0.0), 2.5);
    vec3 col = vCol.rgb * (0.34 + 0.70 * diff) + vec3(1.0) * spec * 0.30 + vCol.rgb * rim * 0.60;
    oColor = vec4(col, vCol.a);
}
)";

const char* kGlowFs = R"(#version 330 core
in vec3 vPos;
in vec3 vNrm;
in vec4 vCol;
uniform vec3 uEye;
out vec4 oColor;
void main() {
    vec3 n = normalize(vNrm);
    vec3 v = normalize(uEye - vPos);
    // brightest where the shell turns away so the halo hugs the silhouette
    float edge = 1.0 - abs(dot(n, v));
    float a = pow(edge, 1.7) * vCol.a;
    oColor = vec4(vCol.rgb, a);
}
)";

const char* kTextVs = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec4 aCol;
uniform mat4 uProj;
out vec2 vUv;
out vec4 vCol;
void main() {
    vUv = aUv;
    vCol = aCol;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
)";

const char* kTextFs = R"(#version 330 core
in vec2 vUv;
in vec4 vCol;
uniform sampler2D uAtlas;
out vec4 oColor;
void main() {
    float a = texture(uAtlas, vUv).r;
    oColor = vec4(vCol.rgb, vCol.a * a);
}
)";

namespace {

GLuint compile(GLenum type, const char* src, char* log, int logSize) {
    const GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);

    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        if (log && logSize > 0) glGetShaderInfoLog(sh, logSize, nullptr, log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

}  // namespace

bool Shader::build(const char* vs, const char* fs, char* log, int logSize) {
    const GLuint v = compile(GL_VERTEX_SHADER, vs, log, logSize);
    if (!v) return false;
    const GLuint f = compile(GL_FRAGMENT_SHADER, fs, log, logSize);
    if (!f) {
        glDeleteShader(v);
        return false;
    }

    id_ = glCreateProgram();
    glAttachShader(id_, v);
    glAttachShader(id_, f);
    glLinkProgram(id_);
    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        if (log && logSize > 0) glGetProgramInfoLog(id_, logSize, nullptr, log);
        glDeleteProgram(id_);
        id_ = 0;
        return false;
    }
    return true;
}

void Shader::use() const { glUseProgram(id_); }

GLint Shader::loc(const char* name) const { return glGetUniformLocation(id_, name); }

void Mesh::clear() {
    verts.clear();
    idx.clear();
}

void Mesh::tube(const Vec3* pts, const Color* cols, int count, float radius, int sides) {
    if (count < 2 || sides < 3) return;

    const unsigned int base = static_cast<unsigned int>(verts.size());

    // parallel transport keeps the ring from twisting along the path
    Vec3 tangent = normalize(pts[1] - pts[0]);
    Vec3 ref = std::fabs(tangent.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
    Vec3 up = normalize(ref - tangent * dot(ref, tangent));

    for (int i = 0; i < count; ++i) {
        const Vec3 prev = pts[i > 0 ? i - 1 : 0];
        const Vec3 next = pts[i + 1 < count ? i + 1 : count - 1];
        Vec3 t = next - prev;
        if (length(t) < 1e-7f) t = tangent;
        t = normalize(t);

        up = normalize(up - t * dot(up, t));
        const Vec3 side = normalize(cross(t, up));

        for (int j = 0; j < sides; ++j) {
            const float a = 2.0f * kPi * j / sides;
            const Vec3 off = up * std::cos(a) + side * std::sin(a);
            Vertex v;
            v.pos = pts[i] + off * radius;
            v.nrm = off;
            v.col = cols[i];
            verts.push_back(v);
        }
        tangent = t;
    }

    for (int i = 0; i + 1 < count; ++i) {
        for (int j = 0; j < sides; ++j) {
            const unsigned int a = base + i * sides + j;
            const unsigned int b = base + i * sides + (j + 1) % sides;
            const unsigned int c = a + sides;
            const unsigned int d = b + sides;
            idx.push_back(a);
            idx.push_back(c);
            idx.push_back(b);
            idx.push_back(b);
            idx.push_back(c);
            idx.push_back(d);
        }
    }
}

void Mesh::sphere(Vec3 center, float radius, Color col, int rings, int sectors) {
    const unsigned int base = static_cast<unsigned int>(verts.size());

    for (int r = 0; r <= rings; ++r) {
        const float phi = kPi * r / rings;
        for (int s = 0; s <= sectors; ++s) {
            const float theta = 2.0f * kPi * s / sectors;
            const Vec3 n{std::sin(phi) * std::cos(theta), std::cos(phi),
                         std::sin(phi) * std::sin(theta)};
            Vertex v;
            v.pos = center + n * radius;
            v.nrm = n;
            v.col = col;
            verts.push_back(v);
        }
    }

    const int stride = sectors + 1;
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            const unsigned int a = base + r * stride + s;
            const unsigned int b = a + stride;
            idx.push_back(a);
            idx.push_back(b);
            idx.push_back(a + 1);
            idx.push_back(a + 1);
            idx.push_back(b);
            idx.push_back(b + 1);
        }
    }
}

void Mesh::disk(const Vec3* rim, int count, Vec3 center, Color col) {
    if (count < 3) return;

    const Vec3 nrm = normalize(cross(rim[1] - rim[0], rim[2] - rim[0]));
    const unsigned int base = static_cast<unsigned int>(verts.size());

    Vertex mid;
    mid.pos = center;
    mid.nrm = nrm;
    mid.col = col;
    verts.push_back(mid);

    for (int i = 0; i < count; ++i) {
        Vertex v;
        v.pos = rim[i];
        v.nrm = nrm;
        v.col = col;
        verts.push_back(v);
    }

    for (int i = 0; i < count; ++i) {
        idx.push_back(base);
        idx.push_back(base + 1 + i);
        idx.push_back(base + 1 + (i + 1) % count);
    }
}

void GpuMesh::create() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    const GLsizei stride = sizeof(Vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(12));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(24));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void GpuMesh::destroy() {
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    ebo_ = vbo_ = vao_ = 0;
}

void GpuMesh::upload(const Mesh& mesh) {
    count_ = static_cast<GLsizei>(mesh.idx.size());
    if (count_ == 0) return;

    const size_t vBytes = mesh.verts.size() * sizeof(Vertex);
    const size_t iBytes = mesh.idx.size() * sizeof(unsigned int);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    if (vBytes > vboCap_) {
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vBytes), mesh.verts.data(),
                     GL_DYNAMIC_DRAW);
        vboCap_ = vBytes;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vBytes), mesh.verts.data());
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    if (iBytes > eboCap_) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(iBytes), mesh.idx.data(),
                     GL_DYNAMIC_DRAW);
        eboCap_ = iBytes;
    } else {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(iBytes),
                        mesh.idx.data());
    }

    glBindVertexArray(0);
}

void GpuMesh::draw() const {
    if (count_ == 0) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
