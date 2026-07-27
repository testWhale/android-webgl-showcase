#version 300 es

// HUDVertex.glsl — Quiz 3 Part 4.
// Screen-space widgets. Positions arrive already in PIXELS (top-left origin,
// Y growing down); the orthographic PROJECTION maps them straight to clip
// space with no camera and no depth scaling, so a button drawn at (40, 60)
// stays at (40, 60) every frame regardless of what the 3D camera does.
// Colour is per-vertex so one draw call can paint many differently-coloured
// quads (buttons + the green/grey speed-bar blocks).

layout(location = 0) in vec2 VertexPosition;  // pixels
layout(location = 1) in vec3 VertexColor;

out vec3 FragColor;

uniform mat4 PROJECTION;   // TransformOrtho(0, w, h, 0, -1, 1)

void main()
{
    gl_Position = PROJECTION * vec4(VertexPosition, 0.0, 1.0);
    FragColor   = VertexColor;
}
