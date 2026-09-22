#include "text.h"

#include <cstring>

namespace {

const int kCols = 16;
const int kRows = 6;
const int kFirst = 32;

}  // namespace

bool TextRenderer::create(int pixelHeight, char* log, int logSize) {
    if (!shader_.build(kTextVs, kTextFs, log, logSize)) return false;

    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) return false;

    HFONT font = CreateFontA(-pixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    if (!font) {
        font = CreateFontA(-pixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                           FIXED_PITCH | FF_MODERN, "Courier New");
    }
    HGDIOBJ oldFont = SelectObject(dc, font);

    TEXTMETRICA tm;
    GetTextMetricsA(dc, &tm);
    const int cw = tm.tmAveCharWidth + 1;
    const int ch = tm.tmHeight;

    atlasW_ = cw * kCols;
    atlasH_ = ch * kRows;
    cellW_ = static_cast<float>(cw);
    cellH_ = static_cast<float>(ch);

    BITMAPINFO bi;
    std::memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = atlasW_;
    bi.bmiHeader.biHeight = -atlasH_;  // top down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bmp) {
        SelectObject(dc, oldFont);
        DeleteObject(font);
        DeleteDC(dc);
        return false;
    }
    HGDIOBJ oldBmp = SelectObject(dc, bmp);

    RECT all{0, 0, atlasW_, atlasH_};
    FillRect(dc, &all, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));

    for (int i = 0; i < kCols * kRows; ++i) {
        const char c = static_cast<char>(kFirst + i);
        TextOutA(dc, (i % kCols) * cw, (i / kCols) * ch, &c, 1);
    }
    GdiFlush();

    std::vector<unsigned char> alpha(static_cast<size_t>(atlasW_) * atlasH_);
    const unsigned char* src = static_cast<const unsigned char*>(bits);
    for (size_t i = 0; i < alpha.size(); ++i) alpha[i] = src[i * 4];

    SelectObject(dc, oldBmp);
    SelectObject(dc, oldFont);
    DeleteObject(bmp);
    DeleteObject(font);
    DeleteDC(dc);

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW_, atlasH_, 0, GL_RED, GL_UNSIGNED_BYTE,
                 alpha.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(8));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(16));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    return true;
}

void TextRenderer::destroy() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (tex_) glDeleteTextures(1, &tex_);
    vbo_ = vao_ = tex_ = 0;
}

void TextRenderer::begin(int screenW, int screenH) {
    proj_ = ortho2d(static_cast<float>(screenW), static_cast<float>(screenH));
    batch_.clear();
}

void TextRenderer::pushQuad(float x, float y, float w, float h, float u0, float v0, float u1,
                            float v1, Color col) {
    const float quad[6][8] = {
        {x, y, u0, v0, col.r, col.g, col.b, col.a},
        {x, y + h, u0, v1, col.r, col.g, col.b, col.a},
        {x + w, y, u1, v0, col.r, col.g, col.b, col.a},
        {x + w, y, u1, v0, col.r, col.g, col.b, col.a},
        {x, y + h, u0, v1, col.r, col.g, col.b, col.a},
        {x + w, y + h, u1, v1, col.r, col.g, col.b, col.a},
    };
    batch_.insert(batch_.end(), &quad[0][0], &quad[0][0] + 48);
}

void TextRenderer::draw(float x, float y, const char* text, Color col, float scale) {
    const float w = cellW_ * scale;
    const float h = cellH_ * scale;
    const float du = cellW_ / atlasW_;
    const float dv = cellH_ / atlasH_;

    // a dark pass first so text stays readable over bright geometry
    for (int pass = 0; pass < 2; ++pass) {
        const Color c = pass == 0 ? Color{0.0f, 0.0f, 0.0f, col.a * 0.75f} : col;
        const float ox = pass == 0 ? 1.0f : 0.0f;
        const float oy = pass == 0 ? 1.0f : 0.0f;

        float cursor = x;
        for (const char* p = text; *p; ++p) {
            const unsigned char ch = static_cast<unsigned char>(*p);
            if (ch >= kFirst && ch < kFirst + kCols * kRows) {
                const int index = ch - kFirst;
                const float u0 = (index % kCols) * du;
                const float v0 = (index / kCols) * dv;
                pushQuad(cursor + ox, y + oy, w, h, u0, v0, u0 + du, v0 + dv, c);
            }
            cursor += w;
        }
    }
}

void TextRenderer::flush() {
    if (batch_.empty()) return;

    shader_.use();
    glUniformMatrix4fv(shader_.loc("uProj"), 1, GL_FALSE, proj_.m);
    glUniform1i(shader_.loc("uAtlas"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const size_t bytes = batch_.size() * sizeof(float);
    if (bytes > cap_) {
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bytes), batch_.data(),
                     GL_DYNAMIC_DRAW);
        cap_ = bytes;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes), batch_.data());
    }

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch_.size() / 8));
    glBindVertexArray(0);
    batch_.clear();
}
