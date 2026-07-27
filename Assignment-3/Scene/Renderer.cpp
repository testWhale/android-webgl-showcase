#define LOG_TAG "Renderer"
#include "Renderer.h"
#include "Scene3D.h"
#include "SceneHUD.h"

Renderer::~Renderer()
{
    delete scene3D; scene3D = nullptr;
    delete hud;     hud     = nullptr;
}

#ifdef PLATFORM_ANDROID
void Renderer::setAssetManager(AAssetManager* mgr) { assetMgr = mgr; }
#endif

bool Renderer::initializeRenderer()
{
    LOGI("Renderer::initializeRenderer");
    createModels();
    initializeModels();
    return true;
}

void Renderer::createModels()
{
    delete scene3D;
    delete hud;
#ifdef PLATFORM_ANDROID
    scene3D = new Scene3D(assetMgr);
    hud     = new SceneHUD(assetMgr);
#else
    scene3D = new Scene3D();
    hud     = new SceneHUD();
#endif
    LOGI("Renderer: Scene3D + SceneHUD created");
}

void Renderer::initializeModels()
{
    scene3D->InitModel();
    hud->InitModel();
}

void Renderer::resize(int w, int h)
{
    screenWidth  = w;
    screenHeight = h;
    glViewport(0, 0, w, h);
    scene3D->Resize(w, h);
    hud->Resize(w, h);
    LOGI("Renderer::resize %d x %d", w, h);
}

// ---------------------------------------------------------------------------
// The fixed mixed 3D + HUD frame order (Part 1).
// ---------------------------------------------------------------------------
void Renderer::render()
{
    // Bonus - press-and-hold: while a button is held, repeat its action on a
    // throttle so a single click = one step and a hold = a steady repeat.
    if (hud->ActiveButton() >= 0) {
        if (++holdFrames % 8 == 0) applyButtonStep(hud->ActiveButton());
    } else {
        holdFrames = 0;
    }

    // The HUD owns the speed value; feed it to the 3D scene each frame.
    scene3D->SetSpeedTarget(hud->CurrentSpeed());

    // Echo the currently-picked part's colour + name into the HUD strip (bonus).
    hud->SetPickSwatch(scene3D->HasPick(), scene3D->PickedColor(), scene3D->PickedName());

    // clear ->
    glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Scene3D with depth test ON (the fan must occlude itself correctly) ->
    glEnable(GL_DEPTH_TEST);
    scene3D->Render();

    // disable depth (a HUD button must never hide behind "closer" 3D geometry) ->
    glDisable(GL_DEPTH_TEST);

    // SceneHUD on top ->
    hud->Render();

    // restore depth for next frame's Scene3D pass. (Buffer swap is done by the
    // platform layer: main.cpp on desktop/web, the JNI GLSurfaceView on Android.)
    glEnable(GL_DEPTH_TEST);
}

// ---------------------------------------------------------------------------
// Input routing: HUD first, then 3D picking (Parts 5 + 8).
// ---------------------------------------------------------------------------
void Renderer::TouchEventDown(float x, float y)
{
    if (hud->TouchDown(x, y)) {
        // A button was hit — consume the press, apply one step immediately, and
        // do NOT fall through to 3D picking.
        applyButtonStep(hud->ActiveButton());
        holdFrames = 0;
    } else {
        // Missed the HUD — treat it as a click on the 3D scene.
        scene3D->PickAt(x, y);
    }
}

void Renderer::TouchEventMove(float /*x*/, float /*y*/)
{
    // Quiz 2's drag-to-boost is retired; nothing to do on move.
}

void Renderer::TouchEventRelease(float /*x*/, float /*y*/)
{
    hud->TouchRelease();
}

void Renderer::applyButtonStep(int button)
{
    switch (button) {
        case SceneHUD::BTN_SPD_UP:   hud->StepSpeed(+1);  break;
        case SceneHUD::BTN_SPD_DOWN: hud->StepSpeed(-1);  break;
        case SceneHUD::BTN_ZOOM_IN:  scene3D->ZoomIn();   break;
        case SceneHUD::BTN_ZOOM_OUT: scene3D->ZoomOut();  break;
        default: break;
    }
}

void Renderer::SetBladeCount(int count)
{
    scene3D->SetBladeCount(count);
}
