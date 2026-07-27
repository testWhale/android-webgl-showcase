#define LOG_TAG "Fan3D"
#include "Fan.h"
#include "ShaderHelper.h"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Geometry - one 8-vertex / 36-index cube, shared by every part.
// Quiz 3: the old per-corner Shade attribute is replaced by a NORMAL. The 8
// corners are shared across faces, so each corner's normal is just its
// normalized position (a rounded, slightly-soft look) - which the brief
// explicitly allows for a shared-corner cube. This is what lets the fragment
// shader run real Phong instead of the old baked gradient.
// ---------------------------------------------------------------------------
static const GLfloat kPositions[8][3] = {
    {-1.0f, -1.0f,  1.0f},  // V0 front bottom-left
    {-1.0f,  1.0f,  1.0f},  // V1 front top-left
    { 1.0f,  1.0f,  1.0f},  // V2 front top-right
    { 1.0f, -1.0f,  1.0f},  // V3 front bottom-right
    {-1.0f, -1.0f, -1.0f},  // V4 back bottom-left
    {-1.0f,  1.0f, -1.0f},  // V5 back top-left
    { 1.0f,  1.0f, -1.0f},  // V6 back top-right
    { 1.0f, -1.0f, -1.0f},  // V7 back bottom-right
};

// Per-corner normal = normalized position (1/sqrt(3) each component).
static const GLfloat kNormals[8][3] = {
    {-0.5773f, -0.5773f,  0.5773f},
    {-0.5773f,  0.5773f,  0.5773f},
    { 0.5773f,  0.5773f,  0.5773f},
    { 0.5773f, -0.5773f,  0.5773f},
    {-0.5773f, -0.5773f, -0.5773f},
    {-0.5773f,  0.5773f, -0.5773f},
    { 0.5773f,  0.5773f, -0.5773f},
    { 0.5773f, -0.5773f, -0.5773f},
};

static const GLushort kIndices[36] = {
    0,3,1, 3,2,1,
    7,4,6, 4,5,6,
    4,0,5, 0,1,5,
    3,7,2, 7,6,2,
    1,2,5, 2,6,5,
    3,0,7, 0,4,7
};

static constexpr GLuint ATTRIB_POSITION = 0;
static constexpr GLuint ATTRIB_NORMAL   = 1;

static constexpr size_t kPosSize    = sizeof(kPositions);
static constexpr size_t kNormalSize = sizeof(kNormals);

// ---- Per-part colours (carried over from Quiz 2) --------------------------
static const glm::vec3 kColorBase  (0.45f, 0.28f, 0.12f);  // brown
static const glm::vec3 kColorPole  (0.55f, 0.55f, 0.58f);  // light gray
static const glm::vec3 kColorHub   (0.20f, 0.20f, 0.22f);  // dark gray
static const glm::vec3 kColorBlade[5] = {
    glm::vec3(0.85f, 0.12f, 0.12f),  // blade 0: red
    glm::vec3(0.15f, 0.35f, 0.85f),  // blade 1: blue
    glm::vec3(0.90f, 0.55f, 0.08f),  // blade 2: orange
    glm::vec3(0.18f, 0.70f, 0.22f),  // blade 3: green
    glm::vec3(0.55f, 0.20f, 0.75f),  // blade 4: purple
};

// Amber highlight applied to whichever part the user picks (Part 8).
static const glm::vec3 kColorHighlight(1.0f, 0.75f, 0.10f);

// Part id helpers (see Fan::PickPart::id).
static constexpr int PART_BASE   = 0;
static constexpr int PART_POLE   = 1;
static constexpr int PART_HUB    = 2;
static constexpr int PART_BLADE0 = 3;

// Light in eye space for the current frame (set at the top of Render()).
static glm::vec3 s_lightEye(0.0f);

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
#ifdef PLATFORM_ANDROID
Fan::Fan(AAssetManager* assetMgr) : mgr(assetMgr) {
#else
Fan::Fan() {
#endif
    modelType = FanType;
}

Fan::~Fan()
{
    if (vao)     { glDeleteVertexArrays(1, &vao); vao     = 0; }
    if (vbo)     { glDeleteBuffers(1, &vbo);       vbo     = 0; }
    if (ibo)     { glDeleteBuffers(1, &ibo);       ibo     = 0; }
    if (program) { glDeleteProgram(program);       program = 0; }
}

// ---------------------------------------------------------------------------
// InitModel - compile shaders, upload the shared cube (positions + normals).
// ---------------------------------------------------------------------------
void Fan::InitModel()
{
    LOGI("Fan::InitModel");

#ifdef PLATFORM_ANDROID
    program = ShaderHelper::buildProgramFromAssets(
        mgr, "shader/FanVertex.glsl", "shader/FanFragment.glsl");
#else
    program = ShaderHelper::buildProgramFromFile(
        "FanVertex.glsl", "FanFragment.glsl");
#endif

    if (!program) { LOGE("Fan: failed to build shader program"); return; }

    uMVP       = glGetUniformLocation(program, "MODELVIEWPROJECTIONMATRIX");
    uMV        = glGetUniformLocation(program, "MODELVIEWMATRIX");
    uNormal    = glGetUniformLocation(program, "NORMALMATRIX");
    uPartColor = glGetUniformLocation(program, "PARTCOLOR");
    uLightPos  = glGetUniformLocation(program, "LIGHTPOSITION");

    // VBO: positions sub-region then normals sub-region.
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, kPosSize + kNormalSize, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,        kPosSize,    kPositions);
    glBufferSubData(GL_ARRAY_BUFFER, kPosSize, kNormalSize, kNormals);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glVertexAttribPointer(ATTRIB_NORMAL,   3, GL_FLOAT, GL_FALSE, 0, (void*)kPosSize);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,         0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    transform.TransformInit();

    // Bonus - oscillation: t=0 for the sway starts now.
    startTime = std::chrono::steady_clock::now();

    LOGI("Fan::InitModel done (VAO=%u, VBO=%u, IBO=%u)", vao, vbo, ibo);
}

// ---------------------------------------------------------------------------
// Resize - Scene3D owns the projection now and pushes it in via SetCamera(),
// so nothing projection-related is needed here.
// ---------------------------------------------------------------------------
void Fan::Resize(int /*w*/, int /*h*/) {}

// ---------------------------------------------------------------------------
// SetCamera - Scene3D hands us the shared view + projection every frame.
// ---------------------------------------------------------------------------
void Fan::SetCamera(const glm::mat4& view, const glm::mat4& projection)
{
    viewMatrix = view;
    projMatrix = projection;
}

// ---------------------------------------------------------------------------
// drawPart - upload this part's matrices + colour, draw the shared cube, and
// cache its world-space pick geometry for Part 8.
// ---------------------------------------------------------------------------
void Fan::drawPart(int partId, const glm::vec3& color,
                   const glm::vec3& localA, const glm::vec3& localB, float radius)
{
    glm::mat4* mvp   = transform.TransformGetModelViewProjectionMatrix();
    glm::mat4* mv    = transform.TransformGetModelViewMatrix();
    glm::mat4* model = transform.TransformGetModelMatrix();
    glm::mat3  nrm;
    transform.TransformGetNormalMatrix(&nrm);

    if (uMVP      >= 0) glUniformMatrix4fv(uMVP,    1, GL_FALSE, glm::value_ptr(*mvp));
    if (uMV       >= 0) glUniformMatrix4fv(uMV,     1, GL_FALSE, glm::value_ptr(*mv));
    if (uNormal   >= 0) glUniformMatrix3fv(uNormal, 1, GL_FALSE, glm::value_ptr(nrm));
    if (uLightPos >= 0) glUniform3fv(uLightPos, 1, glm::value_ptr(s_lightEye));

    const glm::vec3& c = (partId == highlightPart) ? kColorHighlight : color;
    if (uPartColor >= 0) glUniform3fv(uPartColor, 1, glm::value_ptr(c));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
    glBindVertexArray(0);

    // Cache the part's world-space endpoints for picking (model matrix only,
    // no view, so these are world positions).
    PickPart p;
    p.id     = partId;
    p.a      = glm::vec3(*model * glm::vec4(localA, 1.0f));
    p.b      = glm::vec3(*model * glm::vec4(localB, 1.0f));
    p.radius = radius;
    pickParts.push_back(p);
}

// ---------------------------------------------------------------------------
// Render - build the fan hierarchy under the camera Scene3D supplied.
// ---------------------------------------------------------------------------
void Fan::Render()
{
    if (!program || !vao) return;

    // Spin advances at the HUD-driven speed (0 = stopped).
    spinAngle += speed;
    if (spinAngle >= 360.0f) spinAngle -= 360.0f;

    // Light position in eye space (constant for every part this frame).
    s_lightEye = glm::vec3(viewMatrix * glm::vec4(lightPosWorld, 1.0f));

    glUseProgram(program);

    // Load the camera Scene3D gave us into the projection + view stacks; the
    // model stack then holds only the fan's own part hierarchy.
    transform.TransformSetMatrixMode(PROJECTION_MATRIX);
    transform.TransformLoadMatrix(&projMatrix);
    transform.TransformSetMatrixMode(VIEW_MATRIX);
    transform.TransformLoadMatrix(&viewMatrix);
    transform.TransformSetMatrixMode(MODEL_MATRIX);
    transform.TransformLoadIdentity();

    // Rebuild the pick cache from scratch this frame.
    pickParts.clear();

    // ---- base ----
    transform.TransformPushMatrix();
        transform.TransformTranslate(0.0f, -2.6f, 0.0f);
        transform.TransformScale(1.6f, 0.25f, 0.8f);
        drawPart(PART_BASE, kColorBase,
                 glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0.5f);
    transform.TransformPopMatrix();

    // ---- pole ----
    transform.TransformPushMatrix();
        transform.TransformTranslate(0.0f, -0.7f, 0.0f);
        transform.TransformScale(0.15f, 1.7f, 0.15f);
        drawPart(PART_POLE, kColorPole,
                 glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.35f);
    transform.TransformPopMatrix();

    // ---- oscillation (bonus): sway the whole head around a vertical axis
    // through the hub; base and pole stay outside this push/pop.
    float elapsedSec = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - startTime).count();
    float oscAngle = kOscAmplitudeDeg * std::sin(elapsedSec * kOscSpeed);

    transform.TransformPushMatrix();
        transform.TransformTranslate(0.0f, 1.25f, 0.0f);
        transform.TransformRotate(glm::radians(oscAngle), 0.0f, 1.0f, 0.0f);
        transform.TransformTranslate(0.0f, -1.25f, 0.0f);

        // ---- hub ----
        transform.TransformPushMatrix();
            transform.TransformTranslate(0.0f, 1.25f, 0.0f);
            transform.TransformScale(0.3f, 0.3f, 0.3f);
            drawPart(PART_HUB, kColorHub,
                     glm::vec3(0.0f), glm::vec3(0.0f), 0.5f);
        transform.TransformPopMatrix();

        // ---- blades, evenly spaced around Z ----
        const float bladeSpacing = 360.0f / (float)bladeCount;
        for (int i = 0; i < bladeCount; ++i) {
            transform.TransformPushMatrix();
                transform.TransformTranslate(0.0f, 1.25f, 0.4f);
                transform.TransformRotate(glm::radians(spinAngle + i * bladeSpacing), 0.0f, 0.0f, 1.0f);
                transform.TransformTranslate(0.0f, 1.1f, 0.0f);
                transform.TransformScale(0.22f, 0.8f, 0.05f);
                // Segment endpoints (0,-1,0)-(0,1,0) span the blade's long axis,
                // so thin blades are ray-tested as segments, not one loose sphere.
                drawPart(PART_BLADE0 + i, kColorBlade[i],
                         glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.32f);
            transform.TransformPopMatrix();
        }
    transform.TransformPopMatrix();
}

// ---------------------------------------------------------------------------
// Base colour of a part (used by the HUD swatch to echo the picked part).
// ---------------------------------------------------------------------------
glm::vec3 Fan::PartColor(int partId) const
{
    switch (partId) {
        case PART_BASE: return kColorBase;
        case PART_POLE: return kColorPole;
        case PART_HUB:  return kColorHub;
        default: {
            int b = partId - PART_BLADE0;
            if (b >= 0 && b < kMaxBlades) return kColorBlade[b];
            return glm::vec3(0.0f);
        }
    }
}

// ---------------------------------------------------------------------------
// Display name of a part (used by the HUD colour strip). Blades are named
// "BLADE 1".."BLADE 5" (1-based for the reader).
// ---------------------------------------------------------------------------
const char* Fan::PartName(int partId) const
{
    static const char* kBladeNames[5] =
        { "BLADE 1", "BLADE 2", "BLADE 3", "BLADE 4", "BLADE 5" };
    switch (partId) {
        case PART_BASE: return "BASE";
        case PART_POLE: return "POLE";
        case PART_HUB:  return "HUB";
        default: {
            int b = partId - PART_BLADE0;
            if (b >= 0 && b < kMaxBlades) return kBladeNames[b];
            return "";
        }
    }
}

// ---------------------------------------------------------------------------
// Bonus - runtime blade count (wired to keys 3/4/5 via the Renderer).
// ---------------------------------------------------------------------------
void Fan::SetBladeCount(int count)
{
    int clamped = std::max(kMinBlades, std::min(count, kMaxBlades));
    if (clamped == bladeCount) return;
    bladeCount = clamped;
    // A picked blade index may no longer exist after shrinking the count.
    if (highlightPart >= PART_BLADE0 + bladeCount) highlightPart = -1;
    LOGI("Fan: bladeCount = %d", bladeCount);
}
