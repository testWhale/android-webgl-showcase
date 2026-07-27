#pragma once

/**
 * Scene3D.h — Quiz 3 Parts 1, 2, 7, 8. NEW file.
 *
 * The 3D world scene. It OWNS the Quiz 2 Fan and the new Ground, and is the one
 * place that knows about the camera and the light:
 *   - builds the shared perspective projection (Resize) and view matrix (from
 *     an adjustable camera distance) every frame, and hands both to the Fan and
 *     Ground so they render in one consistent world;
 *   - drives the Fan's spin speed from the value the HUD reports;
 *   - owns PickAt() (Part 8): turns a screen click into a world-space ray and
 *     finds which fan part is under the cursor.
 *
 * It reads/writes only shared application state (speed target, camera
 * distance); it never talks to the HUD directly.
 *
 * Bonus: camera distance and fan speed are EASED toward their targets over a
 * few frames instead of snapping, so zoom/speed changes glide.
 */

#include "Model.h"
#include "Platform.h"
#include "Transform.h"
#include "Fan.h"
#include "Ground.h"

#include <glm/glm.hpp>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

class Scene3D : public Model {
public:
#ifdef PLATFORM_ANDROID
    explicit Scene3D(AAssetManager* assetMgr);
#else
    Scene3D();
#endif
    ~Scene3D() override;

    void InitModel()          override;
    void Render()             override;
    void Resize(int w, int h) override;

    void SetBladeCount(int count) override;  // forwarded to the Fan (keys 3/4/5)

    // ---- Camera (Part 7) --------------------------------------------------
    void SetCameraDistance(float d);   // absolute, clamped to [kMinDist, kMaxDist]
    void ZoomIn();                     // +ZOOM: move closer
    void ZoomOut();                    // -ZOOM: move farther

    // ---- Speed (Part 6) — target set by the HUD, eased toward each frame ---
    void SetSpeedTarget(float s);

    // ---- Picking (Part 8) -------------------------------------------------
    // screenX/screenY use top-left origin (mouse/touch convention). Returns
    // true if a fan part was hit. Updates the Fan's highlight in every case
    // (new part, same part again = clear, or empty space = clear).
    bool PickAt(float screenX, float screenY);

    // ---- Pick state for the HUD swatch (bonus) ----------------------------
    bool        HasPick() const;      // true when a fan part is highlighted
    glm::vec3   PickedColor() const;  // that part's base colour (0 if none)
    const char* PickedName() const;   // that part's name ("" if none)

private:
    Fan*    fan    = nullptr;
    Ground* ground = nullptr;

    Transform transform;   // used only for TransformUnproject in PickAt

    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projMatrix = glm::mat4(1.0f);
    glm::vec3 lightWorld = glm::vec3(4.0f, 6.0f, 5.0f);

    int   screenW = 1, screenH = 1;

    // Camera distance: current (eased) + target.
    float cameraDistance = 8.0f;
    float targetDistance = 8.0f;

    // Fan speed: current (eased) + target (set by HUD).
    float currentSpeed = 8.0f;
    float targetSpeed  = 8.0f;

    static constexpr float kMinDist  = 3.0f;
    static constexpr float kMaxDist  = 20.0f;
    static constexpr float kZoomStep = 1.0f;
    static constexpr float kEase     = 0.15f;  // per-frame easing factor

    void rebuildView();
};
