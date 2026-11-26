# Android / OpenGL ES 3.0 → WebGL Showcase

Three C++ OpenGL ES 3.0 applications, first written for Android, that build from one codebase for Android, Windows desktop and the browser (Emscripten → WebGL). Every push to `main` builds them in CI and deploys them to GitHub Pages.

**▶ Live demos: https://testwhale.github.io/android-webgl-showcase/** (no sign-in, runs in any WebGL 2 browser)

| | |
|---|---|
| **What it is** | Three interactive OpenGL ES 3.0 programs in C++: a 2D primitives scene, a 3D fan, and a 3D fan with an on-screen dashboard HUD |
| **Why it exists** | DigiPen Android/graphics coursework, extended so the same C++ code also runs on desktop and in the browser without an Android device |
| **Stack** | C++17, OpenGL ES 3.0 / WebGL 2, GLSL, GLM, GLFW, Emscripten 3.1.64, CMake, Gradle + Android NDK, GitHub Actions, GitHub Pages |
| **My role** | I wrote the scene code for each assignment and set up the cross-platform build and the CI/CD pipeline |
| **Try it** | Open the live link above. To build locally, see [Build](#build) |

---

## Demos

| Demo | What you see | Controls | Live |
|---|---|---|---|
| **Assignment 1 (Android Programming Quiz)** | A colour-interpolated triangle and a solid square drawn with OpenGL ES 3.0 shaders | none | [Launch](https://testwhale.github.io/android-webgl-showcase/Assignment-1/) |
| **Assignment 2 (3D Fan)** | A 3D table fan (base, pole, hub, four blades) built from one shared cube using a push/pop matrix stack; the head oscillates | Click: fan on/off · Drag: spin faster (decays on release) · Keys `3`/`4`/`5`: blade count | [Launch](https://testwhale.github.io/android-webgl-showcase/Assignment-2-3DFan/) |
| **Assignment 3 (Fan Dashboard HUD)** | The fan over a procedural checkerboard ground, with Phong lighting, a 2D HUD (speed/zoom buttons, 20-block speed bar) and 3D picking of fan parts | **±SPD**, **±ZOOM** buttons (hold to repeat) · Click a fan part to highlight it · Keys `3`/`4`/`5` | [Launch](https://testwhale.github.io/android-webgl-showcase/Assignment-3/) |

Per-assignment details: [Assignment-2-3DFan/README.md](Assignment-2-3DFan/README.md), [Assignment-3/README.md](Assignment-3/README.md).

## Verified results (checked 26 Nov 2026)

- All three demos and the landing page load from `testwhale.github.io/android-webgl-showcase/` and render in a WebGL 2.0 (OpenGL ES 3.0) context, with no console errors.
- The latest commit (`69c20b8`) passed all three workflows: **GitHub Pages**, **Desktop** (Ubuntu + Windows) and **Android**.
- 8 workflow runs failed on 3 July 2026 while the pipeline was being set up (commits `bf386c9`, `806e430`, `60a081d`). Every run after that passed until 26 Nov 2026. On that date the Android workflow failed on a documentation-only commit (`a44be1c`) inside the `android-actions/setup-android` step, while it was downloading the SDK command-line tools. The cause is a CI environment issue, not a code change, and it is still open; the Desktop and GitHub Pages workflows passed on that commit.

## Technologies

- **C++17** scene code shared across all platforms (`Scene/`)
- **OpenGL ES 3.0 / WebGL 2** with GLSL shaders loaded from `android/app/src/main/assets/shader/`
- **GLM** for maths (fetched automatically; never committed)
- **GLFW** windowing on desktop and web; a **JNI** bridge (`NativeTemplate.cpp`) on Android
- **Emscripten 3.1.64**: `emcc` with `-s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3`, shaders packed with `--preload-file`
- **CMake** for desktop, **Gradle + Android NDK 27** for Android
- **GitHub Actions** and **GitHub Pages** for CI/CD

## Build

Each assignment folder builds independently.

**Web (Emscripten)**

```bash
source /path/to/emsdk/emsdk_env.sh   # Emscripten SDK activated
cd Assignment-3                      # or Assignment-1 / Assignment-2-3DFan
./build_web.sh                       # writes web/index.html, .js, .wasm, .data
cd web && python3 -m http.server 8080
```

Then open http://localhost:8080. On Windows, use `build_web.bat`.

**Desktop (Windows)**: run `script_build_and_run.bat`. It configures CMake, builds, and launches the program. Dependencies are downloaded on the first configure.

**Android**: open `<assignment>/android/` in Android Studio and run the `app` configuration on a device or emulator with OpenGL ES 3.0 (min SDK 34).

## Deployment

```text
push to main
   ├─ Android  workflow → Gradle + NDK build (Assignment-1, Assignment-2-3DFan)
   ├─ Desktop  workflow → CMake build on ubuntu-latest + windows-latest
   └─ GitHub Pages workflow
        ├─ set up Emscripten 3.1.64
        ├─ build_web.sh for Assignment-1, Assignment-2-3DFan, Assignment-3
        ├─ copy each web/ output into site/<assignment>/
        ├─ copy .github/site/index.html as the landing page
        └─ upload-pages-artifact → deploy-pages
```

Workflow files: [`.github/workflows/pages.yml`](.github/workflows/pages.yml), [`desktop.yml`](.github/workflows/desktop.yml), [`android.yml`](.github/workflows/android.yml).

## Technical challenges

These are taken from the workflow files and the commit history.

- **Getting a browser build of Android code.** The Android entry point is a JNI bridge, so the same scene code also needed a GLFW entry point. It is compiled with `-s FULL_ES3=1` so the OpenGL ES 3.0 calls map onto WebGL 2, and the shaders are preloaded into Emscripten's virtual filesystem.
- **GitHub Pages pipeline set-up.** 8 workflow runs failed on 3 July 2026 while I was iterating on `pages.yml` and the build scripts. From commit `264c6ec` (12 July), the pipeline passed consistently.
- **The Android CI picked the wrong Gradle version.** `gradlew` is not committed, so `setup-gradle` silently fell back to the runner's Gradle 9.x, which is incompatible with AGP 8.x. The workflow now reads the pinned version from `gradle-wrapper.properties` and passes it explicitly.
- **Windows CI couldn't find Visual Studio.** The Visual Studio 17 2022 CMake generator intermittently failed on `windows-latest` runners with "could not find any instance of Visual Studio". I switched the build to Ninja with `msvc-dev-cmd`, which avoids that lookup.
- **The live link broke when the repository was renamed.** GitHub Pages does not redirect after a rename, so the old URL `testwhale.github.io/zachary-android-assignments/` stopped resolving. The landing page uses relative links and was unaffected; this README now uses the current URL.

## My contributions

- The scene implementations for each assignment. For Assignment 3 this includes `Scene3D`, `SceneHUD`, `Ground`, Phong shaders, 3D picking, eased camera and speed, and press-and-hold repeat.
- The cross-platform build: CMake for desktop, `build_web.sh`/`build_web.bat` for Emscripten, and Gradle for Android.
- The three GitHub Actions workflows and the GitHub Pages landing page.

`Transform.h/.cpp` and parts of the framework (`main.cpp`, the JNI bridge) were provided by the course and are noted as unmodified in the per-assignment READMEs.

## Known limitations

- Each web build step in `pages.yml` uses `continue-on-error: true`. If one demo fails to build, the site still deploys without that demo.
- The Android and Desktop workflows build only Assignment-1 and Assignment-2-3DFan. Assignment-3 is built only in the Pages (web) workflow.
- `WK9/` and `Chapter2/` contain earlier course exercises that are not deployed.
