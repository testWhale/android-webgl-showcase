#define LOG_TAG "Fan3D"
#include "Scene3D.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Closest squared distance between a ray (O + t*D, t >= 0) and a segment
// [A, B], plus the ray parameter t at that closest point. Ericson,
// "Real-Time Collision Detection". Used to ray-test the fan's thin blades as
// segments (a single sphere at a blade's centre would be too coarse).
// ---------------------------------------------------------------------------
static float closestRaySegment(const glm::vec3& O, const glm::vec3& D,
                               const glm::vec3& A, const glm::vec3& B,
                               float& rayT)
{
    glm::vec3 d1 = D;          // ray direction (not necessarily unit)
    glm::vec3 d2 = B - A;      // segment direction
    glm::vec3 r  = O - A;
    float a = glm::dot(d1, d1);
    float e = glm::dot(d2, d2);
    float f = glm::dot(d2, r);

    float s, t;                // s along ray, t along segment (both 0..1-ish)
    const float EPS = 1e-8f;

    if (a <= EPS && e <= EPS) { rayT = 0.0f; return glm::dot(r, r); }
    if (a <= EPS) {
        s = 0.0f;
        t = std::min(std::max(f / e, 0.0f), 1.0f);
    } else {
        float c = glm::dot(d1, r);
        if (e <= EPS) {
            t = 0.0f;
            s = std::max(-c / a, 0.0f);            // ray: s >= 0
        } else {
            float b   = glm::dot(d1, d2);
            float den = a * e - b * b;
            s = (den > EPS) ? std::max((b * f - c * e) / den, 0.0f) : 0.0f;
            t = (b * s + f) / e;
            if (t < 0.0f)      { t = 0.0f; s = std::max(-c / a, 0.0f); }
            else if (t > 1.0f) { t = 1.0f; s = std::max((b - c) / a, 0.0f); }
        }
    }

    glm::vec3 c1 = O + d1 * s;
    glm::vec3 c2 = A + d2 * t;
    rayT = s;
    return glm::dot(c1 - c2, c1 - c2);
}

// ---------------------------------------------------------------------------
#ifdef PLATFORM_ANDROID
Scene3D::Scene3D(AAssetManager* assetMgr) {
    fan    = new Fan(assetMgr);
    ground = new Ground(assetMgr);
}
#else
Scene3D::Scene3D() {
    fan    = new Fan();
    ground = new Ground();
}
#endif

Scene3D::~Scene3D()
{
    delete fan;
    delete ground;
}

void Scene3D::InitModel()
{
    ground->InitModel();
    fan->InitModel();
    fan->SetLightWorld(lightWorld);
    ground->SetLightWorld(lightWorld);
}

void Scene3D::Resize(int w, int h)
{
    screenW = (w > 0) ? w : 1;
    screenH = (h > 0) ? h : 1;
    float aspect = (float)screenW / (float)screenH;
    projMatrix = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 1000.0f);
    fan->Resize(w, h);
    ground->Resize(w, h);
}

// View = pull back by the (eased) camera distance, then tilt 20 deg on Y —
// the same transform Quiz 2 baked into Fan::Render(), now owned here so +ZOOM /
// -ZOOM can vary the distance.
void Scene3D::rebuildView()
{
    viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.8f, -cameraDistance))
               * glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Scene3D::Render()
{
    // Bonus easing: glide current values toward their targets.
    cameraDistance += (targetDistance - cameraDistance) * kEase;
    currentSpeed   += (targetSpeed   - currentSpeed)   * kEase;

    rebuildView();

    fan->SetCamera(viewMatrix, projMatrix);
    ground->SetCamera(viewMatrix, projMatrix);
    fan->SetSpeed(currentSpeed);

    // Ground first, then fan (both depth-tested by the Renderer's Scene3D pass).
    ground->Render();
    fan->Render();
}

void Scene3D::SetBladeCount(int count) { if (fan) fan->SetBladeCount(count); }

bool Scene3D::HasPick() const { return fan && fan->Highlight() >= 0; }

glm::vec3 Scene3D::PickedColor() const
{
    if (!fan || fan->Highlight() < 0) return glm::vec3(0.0f);
    return fan->PartColor(fan->Highlight());
}

const char* Scene3D::PickedName() const
{
    if (!fan || fan->Highlight() < 0) return "";
    return fan->PartName(fan->Highlight());
}

// ---------------------------------------------------------------------------
// Camera controls (Part 7)
// ---------------------------------------------------------------------------
void Scene3D::SetCameraDistance(float d)
{
    targetDistance = std::min(std::max(d, kMinDist), kMaxDist);
}

void Scene3D::ZoomIn()  { SetCameraDistance(targetDistance - kZoomStep); }
void Scene3D::ZoomOut() { SetCameraDistance(targetDistance + kZoomStep); }

// ---------------------------------------------------------------------------
// Speed (Part 6) — target set by the HUD.
// ---------------------------------------------------------------------------
void Scene3D::SetSpeedTarget(float s)
{
    targetSpeed = std::min(std::max(s, 0.0f), 20.0f);
}

// ---------------------------------------------------------------------------
// PickAt (Part 8) — unproject click to a world ray, test each fan part.
// ---------------------------------------------------------------------------
bool Scene3D::PickAt(float screenX, float screenY)
{
    if (!fan) return false;

    int viewport[4] = { 0, 0, screenW, screenH };

    // Screen origin is top-left; the GL viewport origin is bottom-left, so
    // flip Y before unprojecting.
    float winX = screenX;
    float winY = (float)screenH - screenY;

    glm::mat4 view = viewMatrix;   // model = identity, so unproject gives world

    float nx, ny, nz, fx, fy, fz;
    int okNear = transform.TransformUnproject(winX, winY, 0.0f, &view, &projMatrix, viewport, &nx, &ny, &nz);
    int okFar  = transform.TransformUnproject(winX, winY, 1.0f, &view, &projMatrix, viewport, &fx, &fy, &fz);
    if (!okNear || !okFar) return false;

    glm::vec3 rayO(nx, ny, nz);
    glm::vec3 rayDir = glm::vec3(fx, fy, fz) - rayO;   // near -> far (not unit)

    // Test the ray against every cached fan part; keep the nearest hit.
    int   bestId = -1;
    float bestT  = 1e30f;
    for (const Fan::PickPart& p : fan->PickParts()) {
        float t;
        float d2 = closestRaySegment(rayO, rayDir, p.a, p.b, t);
        if (d2 <= p.radius * p.radius && t < bestT) {
            bestT  = t;
            bestId = p.id;
        }
    }

    if (bestId < 0) {
        fan->SetHighlight(-1);          // empty space clears the highlight
        LOGI("Pick: empty space (highlight cleared)");
        return false;
    }

    // Clicking the already-highlighted part clears it; otherwise select it.
    if (bestId == fan->Highlight()) {
        fan->SetHighlight(-1);
        LOGI("Pick: part %d deselected", bestId);
    } else {
        fan->SetHighlight(bestId);
        LOGI("Pick: part %d selected", bestId);
    }
    return true;
}
