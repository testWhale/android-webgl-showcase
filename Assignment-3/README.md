# Android Programming Quiz 3 — Fan Dashboard HUD

> Extended in place from Android Programming Quiz 2. Rename this folder to
> **`Android Programming Quiz 3`** when you commit it; it lives alongside the
> untouched Quiz 1 and Quiz 2 folders in the same repository.

A 3D table fan (base, pole, hub, spinning blades) over a procedural
checkerboard ground, driven by an on-screen dashboard HUD instead of touch
gestures. One C++ codebase renders on Android, Windows desktop, and WebGL.

The renderer is split into two scenes drawn in a fixed order every frame:

```
Renderer
├── Scene3D  — Fan (from Quiz 2) + Ground, perspective, depth-tested, Phong-lit
└── SceneHUD — 4 buttons + 20-block speed bar, orthographic, depth-test OFF, drawn last
```

Frame order: **clear → Scene3D (depth ON) → disable depth → SceneHUD → enable depth → swap.**

## What changed from Quiz 2

- **Two scenes, not one.** `Renderer` owns a `Scene3D` and a `SceneHUD` instead
  of a `Fan` directly. Neither scene knows the other exists; the HUD reports
  input, the scene owns behaviour.
- **Real Phong lighting.** The fan's baked `PARTCOLOR * Shade` gradient is gone.
  The cube now carries per-corner normals and the fragment shader runs
  ambient + diffuse + specular, with a normal matrix so lighting stays correct
  as blades spin.
- **Procedural checkerboard ground.** A large quad whose black/white pattern is
  computed in-shader from world XZ cell parity (no texture) and then lit by the
  same Phong light as the fan.
- **Gestures retired.** Quiz 2's tap-to-toggle and drag-to-boost code is
  removed. Speed and camera zoom are now HUD-driven.
- **3D picking.** Clicking directly on the fan unprojects the click into a
  world-space ray and highlights the exact part (base, pole, hub, or a specific
  blade) under the cursor.

## Controls

| Input | Effect |
|---|---|
| **+SPD / −SPD** (top-left) | Change fan speed, clamped to 0–20; the speed bar fills/empties |
| **+ZOOM / −ZOOM** (top-right) | Move the camera closer / farther, clamped to [3, 20] |
| Click/tap a fan part | Pick it — the part turns amber; clicking it again, or clicking empty space, clears the highlight |
| Hold a button | Press-and-hold repeats the action (bonus) |
| Keys `3` / `4` / `5` (Desktop, Web) | Change blade count at runtime |

HUD clicks never trigger a pick, and fan clicks never hit a HUD button — the
press goes to the HUD first and only falls through to 3D picking on a miss.

## Bonus features (for the A+ tier)

All documented here per the rubric:

1. **Smooth easing** — camera distance and fan speed glide toward their target
   values over a few frames instead of snapping (`Scene3D`, `kEase`).
2. **Press-and-hold repeat** — holding any HUD button repeats its action on a
   throttle, so you don't have to click once per step (`Renderer`, `holdFrames`).
3. **Oscillating head** — the hub + blades sway ±25° around a vertical axis
   like a real oscillating fan, independent of speed (carried from Quiz 2).
4. **Runtime blade count** — keys 3/4/5 re-space the blades at 360°/N.

## Files

| File | Role |
|---|---|
| `Scene/Fan.cpp/.h` | Quiz 2 geometry/hierarchy; tap/drag removed; normals added; per-part world-position caching + highlight for picking |
| `Scene/Ground.cpp/.h` | New — checkerboard + Phong quad |
| `Scene/Scene3D.cpp/.h` | New — owns Fan + Ground, camera/view/projection, light, `PickAt()` |
| `Scene/SceneHUD.cpp/.h` | New — buttons + speed bar, ortho, hit-testing |
| `Scene/Renderer.cpp/.h` | Owns both scenes; pipeline order; input routing |
| `android/.../assets/shader/Fan*.glsl` | Upgraded to Phong (+ normal attribute) |
| `android/.../assets/shader/Ground*.glsl` | New — procedural checkerboard + Phong |
| `android/.../assets/shader/HUD*.glsl` | New — flat-coloured screen-space quads |

`Transform.h/.cpp`, `main.cpp`, and the JNI bridge (`NativeTemplate.cpp`) are
unmodified — all input routing lives in `Renderer`.

## Prerequisites (all platforms)

- CMake 3.15+
- Git (all three build paths fetch GLM automatically)

---

## Desktop (Windows)

**Prerequisites:** Visual Studio 2022 (Desktop C++ workload), CMake 3.15+,
internet access on first configure (downloads GLFW, GLEW, GLM).

```
script_build_and_run.bat
```

**Output:** `build_desktop\Debug\Fan3D.exe`

**Manual equivalent:**

```
cmake -S . -B build_desktop -G "Visual Studio 17 2022" -A x64
cmake --build build_desktop --config Debug
build_desktop\Debug\Fan3D.exe
```

Click the HUD buttons to drive speed/zoom, click the fan to pick a part, ESC to quit.

---

## Android

**Prerequisites:** Android Studio (current stable), Android SDK Platform 36,
NDK + CMake components. Min SDK 34.

1. Open the `android/` folder in Android Studio.
2. Let Gradle sync (this first sync generates the `gradlew` wrapper scripts).
3. Run the `app` configuration on a device/emulator with OpenGL ES 3.0.

**Command line equivalent** (after the wrapper exists):

```
cd android
gradlew.bat assembleDebug
```

**Output:** `android\app\build\outputs\apk\debug\app-debug.apk`

Check logs with:

```
adb logcat -s Fan3D
```

---

## Web (WebGL / Emscripten)

**Prerequisites:** [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html),
activated in the current shell.

```
build_web.bat        (Windows)   /   ./build_web.sh   (Unix)
```

First run clones GLM into `external/glm` (git-ignored). Output is `web/index.html`.

**Serve it** (local WebGL won't load over `file://`):

```
cd web
python -m http.server 8080
```

Open `http://localhost:8080/`. Check the browser console (F12) for the HUD
`+SPD pressed` / pick `part N selected` log lines.

---

## CI/CD

`.github/workflows/build-and-deploy.yml` should build Desktop (windows-latest),
Android (ubuntu-latest), and Web (ubuntu-latest) for **all three** assignments
in this one repository, per the quiz's CI requirement.

## What's not in this repo

`build_desktop/`, `web/`, `external/`, and Android/Gradle build/IDE-cache
directories are git-ignored — they're generated, not part of the submission.
