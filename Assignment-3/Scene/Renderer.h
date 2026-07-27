#pragma once

/**
 * Renderer.h — Quiz 3 Part 1.
 *
 * Singleton renderer. In Quiz 2 it owned one Fan directly; in Quiz 3 it owns
 * two scenes — a Scene3D (fan + ground, perspective, depth-tested) and a
 * SceneHUD (buttons + speed bar, orthographic, depth-test off) — and drives
 * them in a fixed per-frame order:
 *
 *   clear -> Scene3D (depth ON) -> disable depth -> SceneHUD -> enable depth -> swap
 *
 * It also routes input: a click goes to the HUD first; only if the HUD misses
 * does it fall through to Scene3D's 3D picking. This routing is what lets
 * main.cpp and the JNI bridge stay untouched — they still just call
 * Renderer::TouchEventDown.
 */

#include "Platform.h"

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

class Scene3D;
class SceneHUD;

class Renderer {
public:
    static Renderer& Instance() {
        static Renderer instance;
        return instance;
    }

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

#ifdef PLATFORM_ANDROID
    void setAssetManager(AAssetManager* mgr);
#endif

    bool initializeRenderer();
    void resize(int w, int h);
    void render();

    void TouchEventDown(float x, float y);
    void TouchEventMove(float x, float y);
    void TouchEventRelease(float x, float y);

    // Stretch goal: forwarded to the fan via Scene3D (keys 3/4/5).
    void SetBladeCount(int count);

private:
    Renderer()  = default;
    ~Renderer();

    void createModels();
    void initializeModels();

    // Apply one button "step" (speed +/-1 or one zoom step). Shared by the
    // initial press and the throttled press-and-hold repeat.
    void applyButtonStep(int button);

#ifdef PLATFORM_ANDROID
    AAssetManager* assetMgr = nullptr;
#endif

    Scene3D*  scene3D = nullptr;
    SceneHUD* hud     = nullptr;

    int screenWidth  = 0;
    int screenHeight = 0;
    int holdFrames   = 0;   // for press-and-hold repeat throttling
};
