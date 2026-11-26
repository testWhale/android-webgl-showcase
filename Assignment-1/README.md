# Assignment 1 — Android Programming Quiz

An introductory OpenGL ES 3.0 scene: a colour-interpolated triangle and a solid square, each drawn with its own vertex/fragment shader. The same C++ scene code builds for Android, Windows desktop and WebGL.

**Live demo:** https://testwhale.github.io/android-webgl-showcase/Assignment-1/

## Source layout

| File | Role |
|---|---|
| `Scene/Triangle.cpp/.h` | Triangle geometry with per-vertex colours |
| `Scene/Square.cpp/.h` | Square geometry |
| `Scene/Renderer.cpp/.h` | Sets up GL state and draws both models |
| `android/app/src/main/assets/shader/BlueTriangle*.glsl` | Shaders, shared by every platform |

## Build

- **Web:** `./build_web.sh` (Emscripten activated), then `cd web && python3 -m http.server 8080`
- **Desktop (Windows):** `script_build_and_run.bat`
- **Android:** open `android/` in Android Studio and run `app`
