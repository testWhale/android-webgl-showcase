#define LOG_TAG "Fan3D"
#include "Ground.h"
#include "ShaderHelper.h"

#include <glm/gtc/type_ptr.hpp>

// ---------------------------------------------------------------------------
// One big quad in the XZ plane, sitting at the fan base's bottom (y = -2.85).
// Made large (100 x 100 world units) so, combined with the 1-unit checker
// cells, it reads as an infinite tiled floor rather than a single panel.
// Each vertex carries an up-facing normal (0,1,0) for the Phong light.
// ---------------------------------------------------------------------------
static const GLfloat kGroundY = -2.85f;
static const GLfloat kHalf    = 50.0f;

static const GLfloat kVertices[4][6] = {
    // x         y         z        nx    ny    nz
    { -kHalf, kGroundY, -kHalf,   0.0f, 1.0f, 0.0f },
    { -kHalf, kGroundY,  kHalf,   0.0f, 1.0f, 0.0f },
    {  kHalf, kGroundY,  kHalf,   0.0f, 1.0f, 0.0f },
    {  kHalf, kGroundY, -kHalf,   0.0f, 1.0f, 0.0f },
};

static const GLushort kIndices[6] = { 0, 1, 2, 0, 2, 3 };

static constexpr GLuint ATTRIB_POSITION = 0;
static constexpr GLuint ATTRIB_NORMAL   = 1;

// ---------------------------------------------------------------------------
#ifdef PLATFORM_ANDROID
Ground::Ground(AAssetManager* assetMgr) : mgr(assetMgr) {
#else
Ground::Ground() {
#endif
    modelType = TriangleType;
}

Ground::~Ground()
{
    if (vao)     { glDeleteVertexArrays(1, &vao); vao     = 0; }
    if (vbo)     { glDeleteBuffers(1, &vbo);       vbo     = 0; }
    if (ibo)     { glDeleteBuffers(1, &ibo);       ibo     = 0; }
    if (program) { glDeleteProgram(program);       program = 0; }
}

void Ground::InitModel()
{
    LOGI("Ground::InitModel");

#ifdef PLATFORM_ANDROID
    program = ShaderHelper::buildProgramFromAssets(
        mgr, "shader/GroundVertex.glsl", "shader/GroundFragment.glsl");
#else
    program = ShaderHelper::buildProgramFromFile(
        "GroundVertex.glsl", "GroundFragment.glsl");
#endif

    if (!program) { LOGE("Ground: failed to build shader program"); return; }

    uModel    = glGetUniformLocation(program, "MODELMATRIX");
    uMV       = glGetUniformLocation(program, "MODELVIEWMATRIX");
    uMVP      = glGetUniformLocation(program, "MODELVIEWPROJECTIONMATRIX");
    uNormal   = glGetUniformLocation(program, "NORMALMATRIX");
    uLightPos = glGetUniformLocation(program, "LIGHTPOSITION");

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(ATTRIB_NORMAL,   3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,         0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    transform.TransformInit();
    LOGI("Ground::InitModel done (VAO=%u)", vao);
}

// Projection is owned by Scene3D and pushed in via SetCamera().
void Ground::Resize(int /*w*/, int /*h*/) {}

void Ground::SetCamera(const glm::mat4& view, const glm::mat4& projection)
{
    viewMatrix = view;
    projMatrix = projection;
}

void Ground::Render()
{
    if (!program || !vao) return;

    glUseProgram(program);

    // Model stays identity (the quad's world position is baked into its
    // vertices), so MODELMATRIX = identity and world XZ == vertex XZ, which is
    // exactly what the checkerboard shader tiles on.
    transform.TransformSetMatrixMode(PROJECTION_MATRIX);
    transform.TransformLoadMatrix(&projMatrix);
    transform.TransformSetMatrixMode(VIEW_MATRIX);
    transform.TransformLoadMatrix(&viewMatrix);
    transform.TransformSetMatrixMode(MODEL_MATRIX);
    transform.TransformLoadIdentity();

    glm::mat4* model = transform.TransformGetModelMatrix();
    glm::mat4* mv    = transform.TransformGetModelViewMatrix();
    glm::mat4* mvp   = transform.TransformGetModelViewProjectionMatrix();
    glm::mat3  nrm;
    transform.TransformGetNormalMatrix(&nrm);

    glm::vec3 lightEye = glm::vec3(viewMatrix * glm::vec4(lightPosWorld, 1.0f));

    if (uModel    >= 0) glUniformMatrix4fv(uModel,  1, GL_FALSE, glm::value_ptr(*model));
    if (uMV       >= 0) glUniformMatrix4fv(uMV,     1, GL_FALSE, glm::value_ptr(*mv));
    if (uMVP      >= 0) glUniformMatrix4fv(uMVP,    1, GL_FALSE, glm::value_ptr(*mvp));
    if (uNormal   >= 0) glUniformMatrix3fv(uNormal, 1, GL_FALSE, glm::value_ptr(nrm));
    if (uLightPos >= 0) glUniform3fv(uLightPos, 1, glm::value_ptr(lightEye));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
    glBindVertexArray(0);
}
