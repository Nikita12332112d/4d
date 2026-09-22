#pragma once

#include "gfx.h"
#include "math4d.h"

enum class Mode { Worldlines, Tesseract, Filmstrip };

struct World {
    float now = 0.0f;
    float horizon = 5.0f;     // half of the visible time window in seconds
    float blockHalf = 3.1f;   // the block always spans this much along w
    Mode mode = Mode::Worldlines;
    bool paused = false;
    bool dashedFuture = true;
};

struct SceneBuild {
    Mesh solid;
    Mesh glow;   // fat shells drawn additively
    Mesh ghost;  // translucent parts drawn last

    void clear() {
        solid.clear();
        glow.clear();
        ghost.clear();
    }
};

extern const Color kPastColor;
extern const Color kNowColor;
extern const Color kFutureColor;

const char* modeName(Mode m);
const char* modeHint(Mode m);
Mode nextMode(Mode m);

int actorCount();
const char* actorName(int i);
Vec4 actorPos(int i, float t);

Color timeColor(float frac);

void buildWorldlines(SceneBuild& out, const World& w, const Lens4& lens);
void buildTesseract(SceneBuild& out, const World& w, const Lens4& lens, float spin);
void buildFilmstrip(SceneBuild& out, const World& w, const Lens4& lens);
