#include "scene.h"

#include <vector>

const Color kPastColor{0.20f, 0.45f, 1.00f, 1.0f};
const Color kNowColor{1.00f, 1.00f, 1.00f, 1.0f};
const Color kFutureColor{1.00f, 0.46f, 0.14f, 1.0f};

namespace {

const int kSamples = 420;
const int kTubeSides = 12;

const float kBallRadius = 0.085f;
const float kFloorY = 0.0f;
const float kHalfWidth = 1.55f;
const float kRoomTop = 1.45f;

const float kTube = 0.030f;
const float kGlowScale = 2.1f;
const float kGlowAlpha = 0.85f;

const Color kGrid{0.16f, 0.19f, 0.26f, 1.0f};

struct ActorDef {
    const char* name;
    const char* hint;
};

const ActorDef kActors[] = {
    {"bouncing ball", "curved worldline"},
    {"rolling ball", "zig zag worldline"},
    {"resting rock", "straight worldline"},
};
const int kActors_N = 3;

float frac01(float v) { return v - std::floor(v); }

void addTube(Mesh& solid, Mesh& glow, const std::vector<Vec3>& pts, const std::vector<Color>& cols,
             float radius, int first, int count) {
    if (count < 2) return;
    solid.tube(&pts[first], &cols[first], count, radius, kTubeSides);

    std::vector<Color> hot(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        hot[i] = cols[first + i];
        hot[i].a = kGlowAlpha;
    }
    glow.tube(&pts[first], hot.data(), count, radius * kGlowScale, 8);
}

// a run of tube geometry with a gap pattern for the future half
void addPath(Mesh& solid, Mesh& glow, const std::vector<Vec3>& pts, const std::vector<Color>& cols,
             float radius, bool dashFuture, int dashStart) {
    if (!dashFuture) {
        addTube(solid, glow, pts, cols, radius, 0, static_cast<int>(pts.size()));
        return;
    }

    addTube(solid, glow, pts, cols, radius, 0, dashStart + 1);

    const int kOn = 8;
    const int kOff = 5;
    const int total = static_cast<int>(pts.size());
    int i = dashStart;
    while (i + 1 < total) {
        const int end = i + kOn < total ? i + kOn : total - 1;
        addTube(solid, glow, pts, cols, radius, i, end - i + 1);
        i = end + kOff;
    }
}

void addLine(Mesh& mesh, const Lens4& lens, Vec4 a, Vec4 b, Color col, float radius, int steps) {
    std::vector<Vec3> pts;
    std::vector<Color> cols;
    for (int i = 0; i <= steps; ++i) {
        pts.push_back(project4(lerp4(a, b, static_cast<float>(i) / steps), lens));
        cols.push_back(col);
    }
    mesh.tube(pts.data(), cols.data(), static_cast<int>(pts.size()), radius, 6);
}

// a rectangle standing in the xy plane at one instant
void addFrame(Mesh& mesh, const Lens4& lens, float w, Color col, float radius) {
    const Vec4 corners[5] = {
        Vec4{-kHalfWidth, kFloorY - 0.06f, 0.0f, w}, Vec4{kHalfWidth, kFloorY - 0.06f, 0.0f, w},
        Vec4{kHalfWidth, kRoomTop, 0.0f, w},         Vec4{-kHalfWidth, kRoomTop, 0.0f, w},
        Vec4{-kHalfWidth, kFloorY - 0.06f, 0.0f, w}};

    std::vector<Vec3> pts;
    std::vector<Color> cols;
    for (int i = 0; i < 5; ++i) {
        pts.push_back(project4(corners[i], lens));
        cols.push_back(col);
    }
    mesh.tube(pts.data(), cols.data(), 5, radius, 8);
}

void addGhostQuad(Mesh& mesh, const Lens4& lens, const Vec4 corners[4], Color col) {
    Vec3 rim[4];
    Vec4 mid{};
    for (int i = 0; i < 4; ++i) {
        rim[i] = project4(corners[i], lens);
        mid = mid + corners[i] * 0.25f;
    }
    mesh.disk(rim, 4, project4(mid, lens), col);
}

}  // namespace

const char* modeName(Mode m) {
    switch (m) {
        case Mode::Worldlines: return "WORLDLINES";
        case Mode::Tesseract: return "TESSERACT";
        case Mode::Filmstrip: return "FILM STRIP";
    }
    return "?";
}

const char* modeHint(Mode m) {
    switch (m) {
        case Mode::Worldlines:
            return "a ball bounces a ball rolls a rock rests - in 4d each one is a tube not a dot";
        case Mode::Tesseract:
            return "a 4d cube - thick white is the 3d shape it shows at the present w = 0";
        case Mode::Filmstrip:
            return "the same block cut into single moments - past and future are just other frames";
    }
    return "";
}

Mode nextMode(Mode m) {
    switch (m) {
        case Mode::Worldlines: return Mode::Tesseract;
        case Mode::Tesseract: return Mode::Filmstrip;
        case Mode::Filmstrip: return Mode::Worldlines;
    }
    return Mode::Worldlines;
}

int actorCount() { return kActors_N; }

const char* actorName(int i) { return kActors[i].name; }

Vec4 actorPos(int i, float t) {
    switch (i) {
        case 0: {
            // a real parabola between bounces
            const float period = 1.7f;
            const float height = 1.05f;
            const float p = frac01(t / period);
            return Vec4{-1.05f, kBallRadius + 4.0f * height * p * (1.0f - p), 0.0f, 0.0f};
        }
        case 1: {
            // constant speed with an elastic turn at each wall
            const float period = 5.0f;
            const float u = frac01(t / period);
            return Vec4{0.55f * (4.0f * std::fabs(u - 0.5f) - 1.0f), kBallRadius, 0.0f, 0.0f};
        }
        default: return Vec4{1.15f, kBallRadius, 0.0f, 0.0f};
    }
}

Color timeColor(float frac) {
    if (frac < -1.0f) frac = -1.0f;
    if (frac > 1.0f) frac = 1.0f;
    const Color c = frac < 0.0f ? mixColor(kPastColor, kNowColor, frac + 1.0f)
                                : mixColor(kNowColor, kFutureColor, frac);
    return scaleColor(c, 1.0f - 0.28f * std::fabs(frac));
}

void buildWorldlines(SceneBuild& out, const World& w, const Lens4& lens) {
    const float hs = w.blockHalf;

    // the floor is not a line but a sheet because it exists at every moment
    for (int i = -3; i <= 3; ++i) {
        const float x = kHalfWidth * i / 3.0f;
        addLine(out.solid, lens, Vec4{x, kFloorY, 0.0f, -hs}, Vec4{x, kFloorY, 0.0f, hs}, kGrid,
                0.008f, 24);
    }

    const int marks = static_cast<int>(w.horizon);
    for (int k = -marks; k <= marks; ++k) {
        const float ww = (k / w.horizon) * hs;
        addLine(out.solid, lens, Vec4{-kHalfWidth, kFloorY, 0.0f, ww},
                Vec4{kHalfWidth, kFloorY, 0.0f, ww}, kGrid, 0.008f, 4);
    }

    const Vec4 floorQuad[4] = {Vec4{-kHalfWidth, kFloorY, 0.0f, -hs},
                               Vec4{kHalfWidth, kFloorY, 0.0f, -hs},
                               Vec4{kHalfWidth, kFloorY, 0.0f, hs},
                               Vec4{-kHalfWidth, kFloorY, 0.0f, hs}};
    addGhostQuad(out.ghost, lens, floorQuad, Color{0.35f, 0.42f, 0.60f, 0.05f});

    // three instants shown as full slices of the block
    const float slices[3] = {-hs, 0.0f, hs};
    const Color slabs[3] = {kPastColor, kNowColor, kFutureColor};
    for (int s = 0; s < 3; ++s) {
        const float frac = static_cast<float>(s) - 1.0f;
        addFrame(out.solid, lens, slices[s], scaleColor(slabs[s], s == 1 ? 1.0f : 0.85f),
                 s == 1 ? 0.016f : 0.012f);
        const Vec4 quad[4] = {Vec4{-kHalfWidth, kFloorY - 0.06f, 0.0f, slices[s]},
                              Vec4{kHalfWidth, kFloorY - 0.06f, 0.0f, slices[s]},
                              Vec4{kHalfWidth, kRoomTop, 0.0f, slices[s]},
                              Vec4{-kHalfWidth, kRoomTop, 0.0f, slices[s]}};
        addGhostQuad(out.ghost, lens, quad,
                     Color{slabs[s].r, slabs[s].g, slabs[s].b, frac == 0.0f ? 0.07f : 0.06f});
    }

    std::vector<Vec3> pts;
    std::vector<Color> cols;
    pts.reserve(kSamples + 1);
    cols.reserve(kSamples + 1);

    for (int a = 0; a < kActors_N; ++a) {
        pts.clear();
        cols.clear();
        for (int i = 0; i <= kSamples; ++i) {
            const float frac = -1.0f + 2.0f * i / kSamples;
            Vec4 p = actorPos(a, w.now + frac * w.horizon);
            p.w = frac * hs;
            pts.push_back(project4(p, lens));
            cols.push_back(timeColor(frac));
        }
        addPath(out.solid, out.glow, pts, cols, kTube, w.dashedFuture, kSamples / 2);
    }

    // where each body actually is in the present
    for (int a = 0; a < kActors_N; ++a) {
        Vec4 p = actorPos(a, w.now);
        p.w = 0.0f;
        const Vec3 c = project4(p, lens);
        out.solid.sphere(c, kBallRadius, kNowColor, 16, 24);
        out.glow.sphere(c, kBallRadius * 1.7f, Color{1.0f, 1.0f, 1.0f, kGlowAlpha}, 12, 16);
    }
}

void buildTesseract(SceneBuild& out, const World& w, const Lens4& lens, float spin) {
    (void)w;
    const float e = 0.80f;

    Rot4 model;
    model.xw = spin * 0.32f;
    model.yz = spin * 0.21f;
    model.zw = 0.35f * std::sin(spin * 0.17f);

    Vec4 v[16];
    for (int i = 0; i < 16; ++i) {
        const Vec4 p{(i & 1) ? e : -e, (i & 2) ? e : -e, (i & 4) ? e : -e, (i & 8) ? e : -e};
        v[i] = rotate4(p, model);
    }

    for (int i = 0; i < 16; ++i) {
        for (int bit = 0; bit < 4; ++bit) {
            const int j = i ^ (1 << bit);
            if (j < i) continue;

            const int kSteps = 10;
            std::vector<Vec3> pts;
            std::vector<Color> cols;
            for (int s = 0; s <= kSteps; ++s) {
                const float t = static_cast<float>(s) / kSteps;
                const Vec4 p = lerp4(v[i], v[j], t);
                pts.push_back(project4(p, lens));
                cols.push_back(scaleColor(timeColor(p.w / e), 0.75f));
            }
            out.solid.tube(pts.data(), cols.data(), static_cast<int>(pts.size()), 0.013f, 8);
        }
    }

    for (int i = 0; i < 16; ++i) {
        out.solid.sphere(project4(v[i], lens), 0.032f, timeColor(v[i].w / e), 12, 16);
    }

    // cut the hypercube with w = 0 to get the solid a 3d observer calls now
    const int pairs[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    const float si[4] = {-e, e, e, -e};
    const float sj[4] = {-e, -e, e, e};

    for (int pi = 0; pi < 6; ++pi) {
        const int i = pairs[pi][0];
        const int j = pairs[pi][1];
        int k = -1;
        int l = -1;
        for (int t = 0; t < 4; ++t) {
            if (t == i || t == j) continue;
            if (k < 0) k = t; else l = t;
        }

        for (int fk = 0; fk < 2; ++fk) {
            for (int fl = 0; fl < 2; ++fl) {
                Vec4 corner[4];
                for (int c = 0; c < 4; ++c) {
                    float co[4];
                    co[i] = si[c];
                    co[j] = sj[c];
                    co[k] = fk ? e : -e;
                    co[l] = fl ? e : -e;
                    corner[c] = rotate4(Vec4{co[0], co[1], co[2], co[3]}, model);
                }

                Vec4 hit[2];
                int hits = 0;
                for (int c = 0; c < 4 && hits < 2; ++c) {
                    const Vec4& p = corner[c];
                    const Vec4& q = corner[(c + 1) % 4];
                    if ((p.w > 0.0f) == (q.w > 0.0f)) continue;
                    hit[hits++] = lerp4(p, q, p.w / (p.w - q.w));
                }
                if (hits < 2) continue;

                const Vec3 pts[2] = {project4(hit[0], lens), project4(hit[1], lens)};
                const Color cols[2] = {kNowColor, kNowColor};
                const Color hot[2] = {Color{1.0f, 1.0f, 1.0f, kGlowAlpha},
                                      Color{1.0f, 1.0f, 1.0f, kGlowAlpha}};
                out.solid.tube(pts, cols, 2, 0.040f, 12);
                out.glow.tube(pts, hot, 2, 0.040f * kGlowScale, 8);
            }
        }
    }
}

void buildFilmstrip(SceneBuild& out, const World& w, const Lens4& lens) {
    const int kFrames = 7;
    const float hs = w.blockHalf;

    for (int f = 0; f < kFrames; ++f) {
        const float frac = -1.0f + 2.0f * f / (kFrames - 1);
        const float ww = frac * hs;
        const float t = w.now + frac * w.horizon;
        const Color col = timeColor(frac);
        const bool present = f == kFrames / 2;

        addFrame(out.solid, lens, ww, scaleColor(col, present ? 1.0f : 0.8f),
                 present ? 0.016f : 0.011f);

        const Vec4 quad[4] = {Vec4{-kHalfWidth, kFloorY - 0.06f, 0.0f, ww},
                              Vec4{kHalfWidth, kFloorY - 0.06f, 0.0f, ww},
                              Vec4{kHalfWidth, kRoomTop, 0.0f, ww},
                              Vec4{-kHalfWidth, kRoomTop, 0.0f, ww}};
        addGhostQuad(out.ghost, lens, quad, Color{col.r, col.g, col.b, present ? 0.09f : 0.06f});

        addLine(out.solid, lens, Vec4{-kHalfWidth, kFloorY, 0.0f, ww},
                Vec4{kHalfWidth, kFloorY, 0.0f, ww}, scaleColor(col, 0.45f), 0.010f, 4);

        for (int a = 0; a < kActors_N; ++a) {
            Vec4 p = actorPos(a, t);
            p.w = ww;
            const Vec3 c = project4(p, lens);
            out.solid.sphere(c, kBallRadius, col, 14, 20);
            out.glow.sphere(c, kBallRadius * 1.6f, Color{col.r, col.g, col.b, kGlowAlpha}, 10, 14);
        }
    }
}
