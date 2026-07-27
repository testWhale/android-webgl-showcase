#pragma once

/**
 * SceneHUD.h — Quiz 3 Parts 4, 5, 6. NEW file.
 *
 * The 2D screen-space dashboard, drawn last, on top of the 3D scene, with its
 * own orthographic projection (TransformOrtho(0, w, h, 0, -1, 1), top-left
 * origin) that is completely independent of the 3D camera — so the widgets
 * never drift, shrink, or move when the camera zooms.
 *
 * Widgets (all plain flat-coloured quads — no textures, no fonts):
 *   +SPD / -SPD   (top-left)   drive the fan speed value, clamped to [0, 20]
 *   +ZOOM / -ZOOM (top-right)  the Renderer maps these to Scene3D's zoom
 *   speed bar     (bottom)     20 fixed 10x20 blocks: first `speed` green, rest grey
 *
 * It owns NO 3D state. It owns the speed value and reports it (CurrentSpeed);
 * the Renderer forwards that to Scene3D. Hit-testing (TouchDown) compares the
 * raw pixel click against each button rectangle — no unprojection, because
 * screen space already is pixel space.
 */

#include "Model.h"
#include "Platform.h"
#include "Transform.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

class SceneHUD : public Model {
public:
    enum Button { BTN_NONE = -1, BTN_SPD_UP = 0, BTN_SPD_DOWN = 1,
                  BTN_ZOOM_IN = 2, BTN_ZOOM_OUT = 3 };

#ifdef PLATFORM_ANDROID
    explicit SceneHUD(AAssetManager* assetMgr);
#else
    SceneHUD();
#endif
    ~SceneHUD() override;

    void InitModel()          override;
    void Render()             override;
    void Resize(int w, int h) override;

    // Part 5: returns true if (x, y) landed on a button (press consumed).
    bool TouchDown(float x, float y);
    void TouchRelease();

    int   ActiveButton() const { return activeButton; }
    void  StepSpeed(int dir);                 // +/- one block, clamped [0, 20]
    float CurrentSpeed() const { return speed; }

    // Bonus: the Renderer pushes the currently-picked part's colour + name here
    // so the HUD can echo it as a colour-coded strip (no font files/textures —
    // the letters are drawn procedurally from small quads).
    void SetPickSwatch(bool active, const glm::vec3& color, const char* name)
        { swatchActive = active; swatchColor = color; swatchName = name ? name : ""; }

private:
    struct Rect { float x, y, w, h; };
    static bool pointInRect(float px, float py, const Rect& r);
    void addQuad(std::vector<float>& out, const Rect& r, const glm::vec3& c) const;
    // Draw a string from the built-in 5x7 quad font. px = pixel cell size.
    void addText(std::vector<float>& out, float x, float y, float px,
                 const std::string& text, const glm::vec3& c) const;

#ifdef PLATFORM_ANDROID
    AAssetManager* mgr = nullptr;
#endif

    GLuint program = 0;
    GLuint vao     = 0;
    GLuint vbo     = 0;
    GLint  uProj   = -1;

    Transform transform;
    glm::mat4 projMatrix = glm::mat4(1.0f);
    int screenW = 1, screenH = 1;

    Rect buttons[4];      // +SPD, -SPD, +ZOOM, -ZOOM (indices match Button enum)
    Rect barOrigin{};     // first speed-bar block; others offset from it
    Rect swatchRect{};    // picked-part colour swatch (bonus)

    float speed        = 8.0f;   // 0..20 — Part 6
    int   activeButton = BTN_NONE;

    bool        swatchActive = false;
    glm::vec3   swatchColor   = glm::vec3(0.0f);
    std::string swatchName;

    static constexpr int   kBarBlocks = 20;
    static constexpr float kBlockW = 10.0f, kBlockH = 20.0f, kBlockGap = 2.0f;
};
