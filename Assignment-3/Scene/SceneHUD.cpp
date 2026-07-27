#define LOG_TAG "Fan3D"
#include "SceneHUD.h"
#include "ShaderHelper.h"

#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

// ---- Widget colours --------------------------------------------------------
static const glm::vec3 kBtnSpdUp  (0.22f, 0.58f, 0.30f);  // green-ish
static const glm::vec3 kBtnSpdDn  (0.58f, 0.30f, 0.24f);  // red-ish
static const glm::vec3 kBtnZoomIn (0.24f, 0.42f, 0.66f);  // blue-ish
static const glm::vec3 kBtnZoomOut(0.40f, 0.30f, 0.60f);  // purple-ish
static const glm::vec3 kBtnActive (0.95f, 0.85f, 0.30f);  // held highlight
static const glm::vec3 kBarOn     (0.20f, 0.85f, 0.32f);  // lit block (green)
static const glm::vec3 kBarOff    (0.28f, 0.28f, 0.32f);  // unlit block (grey)

static const char* kNames[4] = { "+SPD", "-SPD", "+ZOOM", "-ZOOM" };

// ---------------------------------------------------------------------------
#ifdef PLATFORM_ANDROID
SceneHUD::SceneHUD(AAssetManager* assetMgr) : mgr(assetMgr) {
#else
SceneHUD::SceneHUD() {
#endif
    modelType = TriangleType;
}

SceneHUD::~SceneHUD()
{
    if (vao)     { glDeleteVertexArrays(1, &vao); vao     = 0; }
    if (vbo)     { glDeleteBuffers(1, &vbo);       vbo     = 0; }
    if (program) { glDeleteProgram(program);       program = 0; }
}

void SceneHUD::InitModel()
{
    LOGI("SceneHUD::InitModel");

#ifdef PLATFORM_ANDROID
    program = ShaderHelper::buildProgramFromAssets(
        mgr, "shader/HUDVertex.glsl", "shader/HUDFragment.glsl");
#else
    program = ShaderHelper::buildProgramFromFile(
        "HUDVertex.glsl", "HUDFragment.glsl");
#endif

    if (!program) { LOGE("SceneHUD: failed to build shader program"); return; }
    uProj = glGetUniformLocation(program, "PROJECTION");

    // One dynamic interleaved VBO: [x, y, r, g, b] per vertex, rebuilt each
    // frame (buttons + the live speed bar). VertexPosition = loc 0 (vec2),
    // VertexColor = loc 1 (vec3).
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---------------------------------------------------------------------------
// Resize - orthographic projection + re-derive every widget's pixel rect, the
// same way Fan::Resize used to re-derive its perspective on resize/rotate.
// ---------------------------------------------------------------------------
void SceneHUD::Resize(int w, int h)
{
    screenW = (w > 0) ? w : 1;
    screenH = (h > 0) ? h : 1;

    transform.TransformSetMatrixMode(PROJECTION_MATRIX);
    transform.TransformLoadIdentity();
    transform.TransformOrtho(0.0f, (float)screenW, (float)screenH, 0.0f, -1.0f, 1.0f);
    projMatrix = *transform.TransformGetProjectionMatrix();

    const float bw = 110.0f, bh = 44.0f, margin = 30.0f, gap = 16.0f, top = 40.0f;
    buttons[BTN_SPD_UP]   = { margin,                top,           bw, bh };
    buttons[BTN_SPD_DOWN] = { margin,                top + bh + gap, bw, bh };
    buttons[BTN_ZOOM_IN]  = { screenW - margin - bw, top,           bw, bh };
    buttons[BTN_ZOOM_OUT] = { screenW - margin - bw, top + bh + gap, bw, bh };

    const float pitch  = kBlockW + kBlockGap;
    const float barW   = kBarBlocks * pitch - kBlockGap;
    barOrigin = { (screenW - barW) * 0.5f, screenH - 50.0f, kBlockW, kBlockH };

    // Picked-part colour strip: a bigger swatch bottom-left, with room to the
    // right of it for the part's name.
    swatchRect = { 30.0f, (float)screenH - 96.0f, 40.0f, 40.0f };
}

// ---------------------------------------------------------------------------
bool SceneHUD::pointInRect(float px, float py, const Rect& r)
{
    return px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h;
}

void SceneHUD::addQuad(std::vector<float>& out, const Rect& r, const glm::vec3& c) const
{
    const float x0 = r.x, y0 = r.y, x1 = r.x + r.w, y1 = r.y + r.h;
    const float q[6][2] = {
        {x0, y0}, {x0, y1}, {x1, y1},
        {x0, y0}, {x1, y1}, {x1, y0}
    };
    for (int i = 0; i < 6; ++i) {
        out.push_back(q[i][0]); out.push_back(q[i][1]);
        out.push_back(c.r);     out.push_back(c.g); out.push_back(c.b);
    }
}

// ---------------------------------------------------------------------------
// Built-in 5x7 quad font. Each glyph is 7 rows; the low 5 bits of each row are
// the pixels, MSB = leftmost. Only the characters the part names need are
// defined (A B D E H L N O P S U, digits 1-5, space). "No fonts" in the brief
// means no font files / texture atlases — these letters are just small quads.
// ---------------------------------------------------------------------------
static const unsigned char* glyph(char ch)
{
    static const unsigned char A[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const unsigned char B[7] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    static const unsigned char D[7] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
    static const unsigned char E[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static const unsigned char H[7] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const unsigned char L[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    static const unsigned char N[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    static const unsigned char O[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const unsigned char P[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const unsigned char S[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
    static const unsigned char U[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const unsigned char D1[7]= {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E};
    static const unsigned char D2[7]= {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F};
    static const unsigned char D3[7]= {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E};
    static const unsigned char D4[7]= {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02};
    static const unsigned char D5[7]= {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E};
    static const unsigned char SP[7]= {0,0,0,0,0,0,0};
    switch (ch) {
        case 'A': return A; case 'B': return B; case 'D': return D;
        case 'E': return E; case 'H': return H; case 'L': return L;
        case 'N': return N; case 'O': return O; case 'P': return P;
        case 'S': return S; case 'U': return U;
        case '1': return D1; case '2': return D2; case '3': return D3;
        case '4': return D4; case '5': return D5;
        default:  return SP;
    }
}

void SceneHUD::addText(std::vector<float>& out, float x, float y, float px,
                       const std::string& text, const glm::vec3& c) const
{
    float cursor = x;
    for (char ch : text) {
        const unsigned char* g = glyph(ch);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (g[row] & (1 << (4 - col))) {
                    Rect pixel = { cursor + col * px, y + row * px, px, px };
                    addQuad(out, pixel, c);
                }
            }
        }
        cursor += 6.0f * px;   // 5 px glyph + 1 px spacing
    }
}

// ---------------------------------------------------------------------------
// Render - rebuild button + bar geometry every frame (the bar is dynamic:
// its green/grey split follows `speed`) and draw it all in one ortho pass.
// ---------------------------------------------------------------------------
void SceneHUD::Render()
{
    if (!program || !vao) return;

    std::vector<float> verts;
    verts.reserve((4 + kBarBlocks + 2 + 160) * 6 * 5);  // buttons + bar + swatch + name text

    // Buttons (held button flashes the active colour).
    for (int b = 0; b < 4; ++b) {
        glm::vec3 col;
        switch (b) {
            case BTN_SPD_UP:   col = kBtnSpdUp;   break;
            case BTN_SPD_DOWN: col = kBtnSpdDn;   break;
            case BTN_ZOOM_IN:  col = kBtnZoomIn;  break;
            default:           col = kBtnZoomOut; break;
        }
        if (b == activeButton) col = kBtnActive;
        addQuad(verts, buttons[b], col);
    }

    // Speed bar: first round(speed) blocks green, the rest grey.
    const int lit = std::min(std::max((int)std::lround(speed), 0), kBarBlocks);
    const float pitch = kBlockW + kBlockGap;
    for (int i = 0; i < kBarBlocks; ++i) {
        Rect block = { barOrigin.x + i * pitch, barOrigin.y, kBlockW, kBlockH };
        addQuad(verts, block, (i < lit) ? kBarOn : kBarOff);
    }

    // Picked-part colour strip (bonus): a colour swatch plus the part's name
    // drawn in the built-in quad font, confirming the pick on-screen without the
    // console. Shows "NONE" in grey when nothing is picked.
    addQuad(verts, swatchRect, glm::vec3(0.55f, 0.55f, 0.58f));   // frame
    Rect inner = { swatchRect.x + 3.0f, swatchRect.y + 3.0f,
                   swatchRect.w - 6.0f, swatchRect.h - 6.0f };
    addQuad(verts, inner, swatchActive ? swatchColor : glm::vec3(0.10f, 0.10f, 0.12f));

    const float tpx = 4.0f;   // font pixel size (glyph = 20x28 px)
    const std::string label = swatchActive ? swatchName : std::string("NONE");
    const glm::vec3   tcol  = swatchActive ? swatchColor : glm::vec3(0.55f, 0.55f, 0.60f);
    addText(verts, swatchRect.x + swatchRect.w + 14.0f,
            swatchRect.y + (swatchRect.h - 7.0f * tpx) * 0.5f, tpx, label, tcol);

    glUseProgram(program);
    if (uProj >= 0) glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(projMatrix));

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 5));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---------------------------------------------------------------------------
// Part 5 - hit-test the raw pixel click against each button rectangle.
// ---------------------------------------------------------------------------
bool SceneHUD::TouchDown(float x, float y)
{
    for (int b = 0; b < 4; ++b) {
        if (pointInRect(x, y, buttons[b])) {
            activeButton = b;
            LOGI("%s pressed", kNames[b]);
            return true;
        }
    }
    activeButton = BTN_NONE;
    return false;
}

void SceneHUD::TouchRelease() { activeButton = BTN_NONE; }

void SceneHUD::StepSpeed(int dir)
{
    speed = std::min(std::max(speed + (float)dir, 0.0f), 20.0f);
}
