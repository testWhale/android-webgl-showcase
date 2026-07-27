#pragma once

/**
 * Fan.h — Quiz 3 (extended from Quiz 2).
 *
 * The fan geometry/hierarchy is unchanged from Quiz 2: base, pole, hub and
 * N blades, all drawn from one shared cube VAO/VBO/IBO, each part getting its
 * own model matrix off the Transform push/pop stack and its own colour through
 * the PARTCOLOR uniform.
 *
 * What changed for Quiz 3:
 *   - Lighting: the cube now carries a per-corner NORMAL (not the old baked
 *     Shade factor). Render uploads MODELVIEW, MODELVIEWPROJECTION and the
 *     NORMAL matrix so FanFragment.glsl can run real per-fragment Phong.
 *   - Camera: the Quiz 2 baked Translate(0,0.8,-8)/Rotate(20) is GONE. Scene3D
 *     now owns the camera and hands Fan a view + projection matrix every frame
 *     via SetCamera(); the fan only builds its own part hierarchy.
 *   - Speed: the retired tap/drag fields (dragBoost, kBaseSpeed, kTapThreshold,
 *     velocity bookkeeping) are removed. Spin speed is now set externally by the
 *     HUD through SetSpeed() (0..20 deg/frame).
 *   - Picking (Part 8): every frame Render caches each part's world-space
 *     position (a point for base/pole/hub, a segment for each thin blade) so
 *     Scene3D::PickAt can ray-test them. SetHighlight() tints the picked part.
 *
 * Kept from Quiz 2 as bonus behaviour: runtime blade count (3/4/5) and the
 * oscillating head sway.
 */

#include "Model.h"
#include "Platform.h"
#include "Transform.h"

#include <chrono>
#include <vector>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

class Fan : public Model {
public:
    // A pickable part expressed in world space. base/pole/hub are points
    // (a == b); each thin blade is a segment (a = inner end, b = tip) so the
    // ray test can be a segment-distance test, not a coarse single sphere.
    struct PickPart {
        int       id;       // 0 base, 1 pole, 2 hub, 3+i blade i
        glm::vec3 a;        // segment start (world)
        glm::vec3 b;        // segment end   (world)
        float     radius;   // hit tolerance (world units)
    };

#ifdef PLATFORM_ANDROID
    explicit Fan(AAssetManager* assetMgr);
#else
    Fan();
#endif
    ~Fan() override;

    void InitModel()          override;
    void Render()             override;
    void Resize(int w, int h) override;

    // Stretch/bonus: runtime-configurable blade count, clamped to [3, 5].
    void SetBladeCount(int count) override;

    // ---- Quiz 3 wiring, called by Scene3D each frame ----------------------
    void SetCamera(const glm::mat4& view, const glm::mat4& projection);
    void SetLightWorld(const glm::vec3& lightWorld) { lightPosWorld = lightWorld; }
    void SetSpeed(float s)      { speed = s; }        // 0..20, from HUD (Part 6)

    // ---- Picking support (Part 8) -----------------------------------------
    const std::vector<PickPart>& PickParts() const { return pickParts; }
    void SetHighlight(int partId) { highlightPart = partId; }
    int  Highlight() const        { return highlightPart; }

    // Base colour + display name of a part id, for the HUD colour strip.
    glm::vec3   PartColor(int partId) const;
    const char* PartName(int partId) const;

private:
    // Upload this part's matrices + colour, draw the cube, and cache its
    // world-space pick geometry (localA/localB are the part's local endpoints).
    void drawPart(int partId, const glm::vec3& color,
                  const glm::vec3& localA, const glm::vec3& localB, float radius);

#ifdef PLATFORM_ANDROID
    AAssetManager* mgr = nullptr;
#endif

    // ---- GL objects (one shared cube) -------------------------------------
    GLuint program = 0;
    GLuint vao     = 0;
    GLuint vbo     = 0;
    GLuint ibo     = 0;

    GLint uMVP        = -1;
    GLint uMV         = -1;
    GLint uNormal     = -1;
    GLint uPartColor  = -1;
    GLint uLightPos   = -1;

    // ---- Matrix stack + camera fed in by Scene3D --------------------------
    Transform transform;
    glm::mat4 viewMatrix   = glm::mat4(1.0f);
    glm::mat4 projMatrix   = glm::mat4(1.0f);
    glm::vec3 lightPosWorld = glm::vec3(4.0f, 6.0f, 5.0f);

    // ---- Animation state ---------------------------------------------------
    float spinAngle = 0.0f;   // current blade angle (degrees)
    float speed     = 8.0f;   // deg/frame, driven by the HUD (0..20)

    // ---- Picking -----------------------------------------------------------
    std::vector<PickPart> pickParts;   // rebuilt every Render()
    int  highlightPart = -1;           // currently picked part, -1 = none

    // ---- Bonus: runtime blade count ---------------------------------------
    static constexpr int kMinBlades = 3;
    static constexpr int kMaxBlades = 5;
    int bladeCount = 4;

    // ---- Bonus: oscillation ------------------------------------------------
    std::chrono::steady_clock::time_point startTime;
    static constexpr float kOscAmplitudeDeg = 25.0f;
    static constexpr float kOscSpeed        = 0.7f;
};
