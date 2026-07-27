#pragma once

/**
 * Ground.h — Quiz 3 Part 3. NEW file.
 *
 * A single large quad (two triangles) lying in the XZ plane under the fan.
 * It owns no 3D state of its own: Scene3D hands it the shared view/projection
 * and light every frame (SetCamera/SetLightWorld), exactly like the Fan.
 *
 * Its fragment shader (GroundFragment.glsl) computes a black/white checkerboard
 * procedurally from the fragment's world XZ position — no texture, no image
 * file — and then lights the result with the same Phong ADS model as the fan,
 * so the floor shades under the same light instead of looking flat.
 */

#include "Model.h"
#include "Platform.h"
#include "Transform.h"

#include <glm/glm.hpp>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

class Ground : public Model {
public:
#ifdef PLATFORM_ANDROID
    explicit Ground(AAssetManager* assetMgr);
#else
    Ground();
#endif
    ~Ground() override;

    void InitModel()          override;
    void Render()             override;
    void Resize(int w, int h) override;

    void SetCamera(const glm::mat4& view, const glm::mat4& projection);
    void SetLightWorld(const glm::vec3& lightWorld) { lightPosWorld = lightWorld; }

private:
#ifdef PLATFORM_ANDROID
    AAssetManager* mgr = nullptr;
#endif

    GLuint program = 0;
    GLuint vao     = 0;
    GLuint vbo     = 0;
    GLuint ibo     = 0;

    GLint uModel    = -1;
    GLint uMV       = -1;
    GLint uMVP      = -1;
    GLint uNormal   = -1;
    GLint uLightPos = -1;

    Transform transform;
    glm::mat4 viewMatrix    = glm::mat4(1.0f);
    glm::mat4 projMatrix    = glm::mat4(1.0f);
    glm::vec3 lightPosWorld = glm::vec3(4.0f, 6.0f, 5.0f);
};
