#include "glcore.h"

#define GL_DEFINE(type, name) type name = nullptr;
GL_FUNC_TABLE(GL_DEFINE)
#undef GL_DEFINE

namespace {

void* glAddress(const char* name) {
    void* p = reinterpret_cast<void*>(wglGetProcAddress(name));
    const bool bad = p == nullptr || p == reinterpret_cast<void*>(1) ||
                     p == reinterpret_cast<void*>(2) || p == reinterpret_cast<void*>(3) ||
                     p == reinterpret_cast<void*>(-1);
    if (!bad) return p;

    // older entry points live in the dll itself
    static HMODULE lib = LoadLibraryA("opengl32.dll");
    return lib ? reinterpret_cast<void*>(GetProcAddress(lib, name)) : nullptr;
}

}  // namespace

bool loadGlFunctions(const char** missing) {
    bool ok = true;

#define ST_LOAD(type, name)                             \
    name = reinterpret_cast<type>(glAddress(#name));    \
    if (!name && ok) {                                  \
        ok = false;                                     \
        if (missing) *missing = #name;                  \
    }
    GL_FUNC_TABLE(ST_LOAD)
#undef ST_LOAD

    return ok;
}
