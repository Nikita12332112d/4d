#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "gfx.h"
#include "glcore.h"
#include "math4d.h"
#include "scene.h"
#include "text.h"

namespace {

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x0001

#define MOUSE_X(lp) (static_cast<int>(static_cast<short>(LOWORD(lp))))
#define MOUSE_Y(lp) (static_cast<int>(static_cast<short>(HIWORD(lp))))

typedef BOOL(WINAPI* PFN_wglChoosePixelFormatARB)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
typedef HGLRC(WINAPI* PFN_wglCreateContextAttribsARB)(HDC, HGLRC, const int*);
typedef BOOL(WINAPI* PFN_wglSwapIntervalEXT)(int);

// void* hop keeps compilers quiet about the PROC cast
template <typename T>
T wglLoad(const char* name) {
    return reinterpret_cast<T>(reinterpret_cast<void*>(wglGetProcAddress(name)));
}

PFN_wglChoosePixelFormatARB wglChoosePixelFormatARB = nullptr;
PFN_wglCreateContextAttribsARB wglCreateContextAttribsARB = nullptr;
PFN_wglSwapIntervalEXT wglSwapIntervalEXT = nullptr;

struct App {
    HWND hwnd = nullptr;
    HDC dc = nullptr;
    HGLRC rc = nullptr;
    int width = 1280;
    int height = 800;
    bool running = true;

    Shader solid;
    Shader glow;
    GpuMesh gpuSolid;
    GpuMesh gpuGlow;
    GpuMesh gpuGhost;
    TextRenderer text;

    SceneBuild build;
    World world;
    Lens4 lens;

    Vec3 target{0.0f, 0.0f, 0.0f};
    float yaw = 0.62f;
    float pitch = 0.42f;
    float dist = 7.4f;
    float fov = 0.58f;  // a long lens keeps parallel slices looking parallel
    float spin = 0.0f;

    bool orbiting = false;
    bool turning4 = false;   // right button rotates xw and yw
    bool turningZw = false;  // middle button rotates zw
    POINT lastMouse{0, 0};
};

App g_app;

void setupView(App& app, Mode mode) {
    app.lens.rot = Rot4{};
    app.target = Vec3{0.0f, 0.0f, 0.0f};
    switch (mode) {
        case Mode::Worldlines:
            // send the time axis onto the z axis of the scene
            app.lens.rot.zw = kPi * 0.5f;
            app.lens.eye = 1e9f;
            app.target = Vec3{0.0f, 0.55f, 0.0f};
            app.yaw = 0.92f;
            app.pitch = 0.26f;
            app.dist = 9.6f;
            break;
        case Mode::Tesseract:
            app.lens.eye = 4.2f;
            app.yaw = 0.60f;
            app.pitch = 0.35f;
            app.dist = 7.6f;
            break;
        case Mode::Filmstrip:
            app.lens.rot.zw = kPi * 0.5f;
            app.lens.eye = 1e9f;
            app.target = Vec3{0.0f, 0.55f, 0.0f};
            app.yaw = 0.72f;
            app.pitch = 0.20f;
            app.dist = 10.6f;
            break;
    }
}

Vec3 cameraEye(const App& app) {
    return app.target + Vec3{app.dist * std::cos(app.pitch) * std::sin(app.yaw),
                             app.dist * std::sin(app.pitch),
                             app.dist * std::cos(app.pitch) * std::cos(app.yaw)};
}

bool worldToScreen(Vec3 p, const Mat4& viewProj, int vx, int vy, int vw, int vh, float& sx,
                   float& sy) {
    const float x = viewProj.m[0] * p.x + viewProj.m[4] * p.y + viewProj.m[8] * p.z + viewProj.m[12];
    const float y = viewProj.m[1] * p.x + viewProj.m[5] * p.y + viewProj.m[9] * p.z + viewProj.m[13];
    const float w = viewProj.m[3] * p.x + viewProj.m[7] * p.y + viewProj.m[11] * p.z + viewProj.m[15];
    if (w <= 0.01f) return false;

    sx = vx + (x / w * 0.5f + 0.5f) * vw;
    sy = vy + (1.0f - (y / w * 0.5f + 0.5f)) * vh;
    return true;
}

void drawMesh(App& app, const Mat4& viewProj, Vec3 eye) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    app.solid.use();
    glUniformMatrix4fv(app.solid.loc("uViewProj"), 1, GL_FALSE, viewProj.m);
    glUniform3f(app.solid.loc("uEye"), eye.x, eye.y, eye.z);
    glDisable(GL_BLEND);
    app.gpuSolid.upload(app.build.solid);
    app.gpuSolid.draw();

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // additive shells give the worldlines a soft halo
    app.glow.use();
    glUniformMatrix4fv(app.glow.loc("uViewProj"), 1, GL_FALSE, viewProj.m);
    glUniform3f(app.glow.loc("uEye"), eye.x, eye.y, eye.z);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    app.gpuGlow.upload(app.build.glow);
    app.gpuGlow.draw();

    app.solid.use();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    app.gpuGhost.upload(app.build.ghost);
    app.gpuGhost.draw();

    glDepthMask(GL_TRUE);
}

void drawHud(App& app) {
    const float lh = app.text.charH();
    char buf[200];

    app.text.draw(24.0f, 24.0f, modeName(app.world.mode), Color{0.90f, 0.94f, 1.0f, 1.0f}, 1.3f);

    float y = 24.0f + app.text.charH(1.3f) + 8.0f;
    app.text.draw(24.0f, y, modeHint(app.world.mode), Color{0.62f, 0.68f, 0.80f, 1.0f}, 0.9f);

    y += lh * 1.5f;
    std::snprintf(buf, sizeof(buf), "t %+0.2f s    window +-%0.0f s    %s", app.world.now,
                  app.world.horizon, app.world.paused ? "[paused]" : "[running]");
    app.text.draw(24.0f, y, buf, Color{0.70f, 0.76f, 0.88f, 1.0f});

    y += lh * 1.5f;
    app.text.draw(24.0f, y, "PAST", kPastColor);
    app.text.draw(24.0f + app.text.charW() * 6.0f, y, "NOW  w = 0", kNowColor);
    app.text.draw(24.0f + app.text.charW() * 18.0f, y, "FUTURE  dashed", kFutureColor);

    const Color dim{0.58f, 0.63f, 0.74f, 1.0f};
    const float base = app.height - lh * 4.6f;
    app.text.draw(24.0f, base,
                  "left drag  orbit camera     right drag  rotate 4D  XW YW     wheel  zoom", dim);
    app.text.draw(24.0f, base + lh * 1.2f,
                  "middle drag  rotate ZW     Q E  XW     A D  YW     Z C  ZW", dim);
    app.text.draw(24.0f, base + lh * 2.4f,
                  "Tab  mode     Space  pause     , .  step time     [ ]  time window", dim);
    app.text.draw(24.0f, base + lh * 3.6f, "F  dashed future     R  reset view     Esc  quit", dim);
}

void drawLabels(App& app, const Mat4& viewProj) {
    if (app.world.mode != Mode::Worldlines) return;

    for (int a = 0; a < actorCount(); ++a) {
        Vec4 p = actorPos(a, app.world.now);
        p.w = 0.0f;
        float sx = 0.0f;
        float sy = 0.0f;
        if (!worldToScreen(project4(p, app.lens), viewProj, 0, 0, app.width, app.height, sx, sy)) {
            continue;
        }
        app.text.draw(sx + 16.0f, sy - 10.0f, actorName(a), Color{0.84f, 0.88f, 0.98f, 1.0f}, 0.85f);
    }
}

void render(App& app) {
    glViewport(0, 0, app.width, app.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app.text.begin(app.width, app.height);

    app.build.clear();
    switch (app.world.mode) {
        case Mode::Worldlines: buildWorldlines(app.build, app.world, app.lens); break;
        case Mode::Tesseract: buildTesseract(app.build, app.world, app.lens, app.spin); break;
        case Mode::Filmstrip: buildFilmstrip(app.build, app.world, app.lens); break;
    }

    const Vec3 eye = cameraEye(app);
    const float aspect = static_cast<float>(app.width) / app.height;
    const Mat4 viewProj =
        perspective(app.fov, aspect, 0.05f, 100.0f) * lookAt(eye, app.target, Vec3{0, 1, 0});

    drawMesh(app, viewProj, eye);
    drawLabels(app, viewProj);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawHud(app);
    app.text.flush();
}

void update(App& app, float dt) {
    if (!app.world.paused) app.world.now += dt;
    app.spin += dt;

    if (GetForegroundWindow() != app.hwnd) return;

    const float turn = 1.1f * dt;
    if (GetAsyncKeyState('Q') & 0x8000) app.lens.rot.xw -= turn;
    if (GetAsyncKeyState('E') & 0x8000) app.lens.rot.xw += turn;
    if (GetAsyncKeyState('A') & 0x8000) app.lens.rot.yw -= turn;
    if (GetAsyncKeyState('D') & 0x8000) app.lens.rot.yw += turn;
    if (GetAsyncKeyState('Z') & 0x8000) app.lens.rot.zw -= turn;
    if (GetAsyncKeyState('C') & 0x8000) app.lens.rot.zw += turn;
}

void onKey(App& app, WPARAM key) {
    switch (key) {
        case VK_ESCAPE: app.running = false; break;
        case VK_TAB:
            app.world.mode = nextMode(app.world.mode);
            setupView(app, app.world.mode);
            break;
        case VK_SPACE: app.world.paused = !app.world.paused; break;
        case '1': app.world.mode = Mode::Worldlines; setupView(app, app.world.mode); break;
        case '2': app.world.mode = Mode::Tesseract; setupView(app, app.world.mode); break;
        case '3': app.world.mode = Mode::Filmstrip; setupView(app, app.world.mode); break;
        case 'F': app.world.dashedFuture = !app.world.dashedFuture; break;
        case 'R': setupView(app, app.world.mode); break;
        case VK_OEM_4: app.world.horizon = app.world.horizon > 1.5f ? app.world.horizon - 0.5f : 1.0f; break;
        case VK_OEM_6: app.world.horizon = app.world.horizon < 16.0f ? app.world.horizon + 0.5f : 16.0f; break;
        case VK_OEM_COMMA: app.world.now -= 0.15f; break;
        case VK_OEM_PERIOD: app.world.now += 0.15f; break;
        default: break;
    }
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    App* app = &g_app;
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            app->running = false;
            return 0;
        case WM_SIZE:
            app->width = LOWORD(lp) > 1 ? LOWORD(lp) : 1;
            app->height = HIWORD(lp) > 1 ? HIWORD(lp) : 1;
            return 0;
        case WM_KEYDOWN:
            onKey(*app, wp);
            return 0;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            if (msg == WM_LBUTTONDOWN) app->orbiting = true;
            if (msg == WM_RBUTTONDOWN) app->turning4 = true;
            if (msg == WM_MBUTTONDOWN) app->turningZw = true;
            app->lastMouse.x = MOUSE_X(lp);
            app->lastMouse.y = MOUSE_Y(lp);
            SetCapture(hwnd);
            return 0;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            if (msg == WM_LBUTTONUP) app->orbiting = false;
            if (msg == WM_RBUTTONUP) app->turning4 = false;
            if (msg == WM_MBUTTONUP) app->turningZw = false;
            if (!app->orbiting && !app->turning4 && !app->turningZw) ReleaseCapture();
            return 0;
        case WM_MOUSEMOVE: {
            const int x = MOUSE_X(lp);
            const int y = MOUSE_Y(lp);
            const float dx = static_cast<float>(x - app->lastMouse.x);
            const float dy = static_cast<float>(y - app->lastMouse.y);

            if (app->orbiting) {
                app->yaw += dx * 0.008f;
                app->pitch += dy * 0.008f;
                const float limit = kPi * 0.49f;
                if (app->pitch > limit) app->pitch = limit;
                if (app->pitch < -limit) app->pitch = -limit;
            }
            if (app->turning4) {
                app->lens.rot.xw += dx * 0.006f;
                app->lens.rot.yw += dy * 0.006f;
            }
            if (app->turningZw) {
                app->lens.rot.zw += dx * 0.006f;
            }

            app->lastMouse.x = x;
            app->lastMouse.y = y;
            return 0;
        }
        case WM_MOUSEWHEEL: {
            const short delta = static_cast<short>(HIWORD(wp));
            app->dist *= delta > 0 ? 0.90f : 1.11f;
            if (app->dist < 1.2f) app->dist = 1.2f;
            if (app->dist > 40.0f) app->dist = 40.0f;
            return 0;
        }
        default: break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

bool loadWglExtensions() {
    WNDCLASSA wc;
    std::memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "st4d_dummy";
    RegisterClassA(&wc);

    HWND dummy = CreateWindowExA(0, wc.lpszClassName, "", 0, 0, 0, 1, 1, nullptr, nullptr,
                                 wc.hInstance, nullptr);
    if (!dummy) return false;
    HDC dc = GetDC(dummy);

    PIXELFORMATDESCRIPTOR pfd;
    std::memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    const int fmt = ChoosePixelFormat(dc, &pfd);
    SetPixelFormat(dc, fmt, &pfd);

    HGLRC rc = wglCreateContext(dc);
    wglMakeCurrent(dc, rc);

    wglChoosePixelFormatARB = wglLoad<PFN_wglChoosePixelFormatARB>("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB =
        wglLoad<PFN_wglCreateContextAttribsARB>("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = wglLoad<PFN_wglSwapIntervalEXT>("wglSwapIntervalEXT");

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(rc);
    ReleaseDC(dummy, dc);
    DestroyWindow(dummy);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);

    return wglCreateContextAttribsARB != nullptr;
}

bool createWindow(App& app, bool hidden) {
    WNDCLASSA wc;
    std::memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.lpszClassName = "st4d_window";
    RegisterClassA(&wc);

    RECT r{0, 0, app.width, app.height};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    app.hwnd = CreateWindowExA(0, wc.lpszClassName, "spacetime4d - past present and future at once",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                               r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);
    if (!app.hwnd) return false;
    app.dc = GetDC(app.hwnd);

    int fmt = 0;
    UINT numFormats = 0;
    if (wglChoosePixelFormatARB) {
        const int attribs[] = {WGL_DRAW_TO_WINDOW_ARB,
                               GL_TRUE,
                               WGL_SUPPORT_OPENGL_ARB,
                               GL_TRUE,
                               WGL_DOUBLE_BUFFER_ARB,
                               GL_TRUE,
                               WGL_ACCELERATION_ARB,
                               WGL_FULL_ACCELERATION_ARB,
                               WGL_PIXEL_TYPE_ARB,
                               WGL_TYPE_RGBA_ARB,
                               WGL_COLOR_BITS_ARB,
                               32,
                               WGL_DEPTH_BITS_ARB,
                               24,
                               WGL_STENCIL_BITS_ARB,
                               8,
                               WGL_SAMPLE_BUFFERS_ARB,
                               GL_TRUE,
                               WGL_SAMPLES_ARB,
                               4,
                               0};
        wglChoosePixelFormatARB(app.dc, attribs, nullptr, 1, &fmt, &numFormats);
    }

    PIXELFORMATDESCRIPTOR pfd;
    std::memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    if (numFormats == 0) fmt = ChoosePixelFormat(app.dc, &pfd);
    DescribePixelFormat(app.dc, fmt, sizeof(pfd), &pfd);
    SetPixelFormat(app.dc, fmt, &pfd);

    const int ctxAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                              WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                              WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                              0};
    app.rc = wglCreateContextAttribsARB(app.dc, nullptr, ctxAttribs);
    if (!app.rc) return false;
    wglMakeCurrent(app.dc, app.rc);

    if (wglSwapIntervalEXT) wglSwapIntervalEXT(1);
    ShowWindow(app.hwnd, hidden ? SW_HIDE : SW_SHOW);
    return true;
}

bool saveBmp(const char* path, int w, int h) {
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    const int rowPad = (4 - (w * 3) % 4) % 4;
    const int dataSize = (w * 3 + rowPad) * h;

    FILE* f = std::fopen(path, "wb");
    if (!f) return false;

    unsigned char header[54];
    std::memset(header, 0, sizeof(header));
    header[0] = 'B';
    header[1] = 'M';
    const int fileSize = 54 + dataSize;
    std::memcpy(header + 2, &fileSize, 4);
    const int offset = 54;
    std::memcpy(header + 10, &offset, 4);
    const int headerSize = 40;
    std::memcpy(header + 14, &headerSize, 4);
    std::memcpy(header + 18, &w, 4);
    std::memcpy(header + 22, &h, 4);
    const short planes = 1;
    const short bits = 24;
    std::memcpy(header + 26, &planes, 2);
    std::memcpy(header + 28, &bits, 2);
    std::memcpy(header + 34, &dataSize, 4);
    std::fwrite(header, 1, sizeof(header), f);

    const unsigned char pad[3] = {0, 0, 0};
    for (int y = 0; y < h; ++y) {
        const unsigned char* row = &pixels[static_cast<size_t>(y) * w * 3];
        for (int x = 0; x < w; ++x) {
            const unsigned char bgr[3] = {row[x * 3 + 2], row[x * 3 + 1], row[x * 3 + 0]};
            std::fwrite(bgr, 1, 3, f);
        }
        if (rowPad) std::fwrite(pad, 1, rowPad, f);
    }

    std::fclose(f);
    return true;
}

void fail(const char* msg) { MessageBoxA(nullptr, msg, "spacetime4d", MB_ICONERROR | MB_OK); }

}  // namespace

int main(int argc, char** argv) {
    const char* shotPath = nullptr;
    float startTime = 0.0f;
    int startMode = 0;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--shot") == 0 && i + 1 < argc) {
            shotPath = argv[++i];
        } else if (std::strcmp(argv[i], "--time") == 0 && i + 1 < argc) {
            startTime = static_cast<float>(std::atof(argv[++i]));
        } else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            startMode = std::atoi(argv[++i]);
        }
    }

    if (!loadWglExtensions()) {
        fail("Modern OpenGL context creation is not available on this machine.");
        return 1;
    }
    if (!createWindow(g_app, shotPath != nullptr)) {
        fail("Could not create an OpenGL 3.3 window.");
        return 1;
    }

    const char* missing = nullptr;
    if (!loadGlFunctions(&missing)) {
        char msg[256];
        std::snprintf(msg, sizeof(msg), "Missing OpenGL entry point: %s",
                      missing ? missing : "unknown");
        fail(msg);
        return 1;
    }

    char log[2048] = {0};
    if (!g_app.solid.build(kSolidVs, kSolidFs, log, sizeof(log))) {
        fail(log);
        return 1;
    }
    if (!g_app.glow.build(kSolidVs, kGlowFs, log, sizeof(log))) {
        fail(log);
        return 1;
    }
    if (!g_app.text.create(17, log, sizeof(log))) {
        fail("Could not bake the font atlas.");
        return 1;
    }

    g_app.gpuSolid.create();
    g_app.gpuGlow.create();
    g_app.gpuGhost.create();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL);

    if (startMode == 1) g_app.world.mode = Mode::Tesseract;
    if (startMode == 2) g_app.world.mode = Mode::Filmstrip;
    setupView(g_app, g_app.world.mode);
    g_app.world.now = startTime;
    g_app.spin = startTime;

    LARGE_INTEGER freq;
    LARGE_INTEGER prev;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    while (g_app.running) {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = static_cast<float>(now.QuadPart - prev.QuadPart) / freq.QuadPart;
        prev = now;
        if (dt > 0.1f) dt = 0.1f;

        if (shotPath) dt = 0.0f;
        update(g_app, dt);
        render(g_app);

        if (shotPath) {
            glFinish();
            saveBmp(shotPath, g_app.width, g_app.height);
            break;
        }

        SwapBuffers(g_app.dc);
    }

    g_app.text.destroy();
    g_app.gpuSolid.destroy();
    g_app.gpuGlow.destroy();
    g_app.gpuGhost.destroy();
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(g_app.rc);
    ReleaseDC(g_app.hwnd, g_app.dc);
    DestroyWindow(g_app.hwnd);
    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return main(__argc, __argv); }
