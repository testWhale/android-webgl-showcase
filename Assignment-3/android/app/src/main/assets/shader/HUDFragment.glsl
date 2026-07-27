#version 300 es

precision mediump float;

// HUDFragment.glsl — Quiz 3 Part 4.
// Flat-coloured screen-space quads/triangles. No texture, no font — the colour
// is interpolated from the per-vertex colour supplied by the HUD geometry.

in vec3 FragColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(FragColor, 1.0);
}
