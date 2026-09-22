#pragma once

#include <cmath>

constexpr float kPi = 3.14159265358979f;

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return Vec3{a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return Vec3{a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float k) { return Vec3{a.x * k, a.y * k, a.z * k}; }

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(Vec3 a, Vec3 b) {
    return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }

inline Vec3 normalize(Vec3 a) {
    const float len = length(a);
    return len > 1e-8f ? a * (1.0f / len) : Vec3{0.0f, 1.0f, 0.0f};
}

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

inline Vec4 operator+(Vec4 a, Vec4 b) { return Vec4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vec4 operator-(Vec4 a, Vec4 b) { return Vec4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline Vec4 operator*(Vec4 a, float k) { return Vec4{a.x * k, a.y * k, a.z * k, a.w * k}; }
inline Vec4 lerp4(Vec4 a, Vec4 b, float t) { return a + (b - a) * t; }

// 4d has six independent rotation planes
struct Rot4 {
    float xy = 0.0f;
    float xz = 0.0f;
    float xw = 0.0f;
    float yz = 0.0f;
    float yw = 0.0f;
    float zw = 0.0f;
};

inline void spin(float& a, float& b, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const float na = a * c - b * s;
    const float nb = a * s + b * c;
    a = na;
    b = nb;
}

inline Vec4 rotate4(Vec4 p, const Rot4& r) {
    spin(p.x, p.y, r.xy);
    spin(p.x, p.z, r.xz);
    spin(p.x, p.w, r.xw);
    spin(p.y, p.z, r.yz);
    spin(p.y, p.w, r.yw);
    spin(p.z, p.w, r.zw);
    return p;
}

// how the fourth axis collapses into the three we can draw
struct Lens4 {
    Rot4 rot;
    float eye = 40.0f;  // large values approach an orthographic slide
};

inline Vec3 project4(Vec4 p, const Lens4& lens) {
    const Vec4 q = rotate4(p, lens.rot);
    float k = 1.0f;
    if (lens.eye < 500.0f) {
        const float d = lens.eye - q.w;
        k = d > 0.2f ? lens.eye / d : lens.eye / 0.2f;
    }
    return Vec3{q.x * k, q.y * k, q.z * k};
}

struct Mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 out;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a.m[k * 4 + r] * b.m[c * 4 + k];
            out.m[c * 4 + r] = sum;
        }
    }
    return out;
}

inline Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
    Mat4 out;
    const float f = 1.0f / std::tan(fovY * 0.5f);
    for (int i = 0; i < 16; ++i) out.m[i] = 0.0f;
    out.m[0] = f / aspect;
    out.m[5] = f;
    out.m[10] = (zFar + zNear) / (zNear - zFar);
    out.m[11] = -1.0f;
    out.m[14] = 2.0f * zFar * zNear / (zNear - zFar);
    return out;
}

inline Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    const Vec3 f = normalize(center - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 out;
    out.m[0] = s.x;  out.m[4] = s.y;  out.m[8] = s.z;   out.m[12] = -dot(s, eye);
    out.m[1] = u.x;  out.m[5] = u.y;  out.m[9] = u.z;   out.m[13] = -dot(u, eye);
    out.m[2] = -f.x; out.m[6] = -f.y; out.m[10] = -f.z; out.m[14] = dot(f, eye);
    out.m[3] = 0.0f; out.m[7] = 0.0f; out.m[11] = 0.0f; out.m[15] = 1.0f;
    return out;
}

inline Mat4 ortho2d(float w, float h) {
    Mat4 out;
    for (int i = 0; i < 16; ++i) out.m[i] = 0.0f;
    out.m[0] = 2.0f / w;
    out.m[5] = -2.0f / h;
    out.m[10] = -1.0f;
    out.m[12] = -1.0f;
    out.m[13] = 1.0f;
    out.m[15] = 1.0f;
    return out;
}
