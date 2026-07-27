#version 300 es

// GroundVertex.glsl — Quiz 3 Part 3.
// The ground is one flat quad in the XZ plane. It passes the fragment's
// WORLD position through so the fragment shader can compute a checkerboard
// from world coordinates (so the pattern tiles across the whole plane,
// independent of the camera), plus the eye-space position/normal that the
// same Phong light from Part 2 needs.

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;

out vec3 WorldPosition;
out vec3 EyePosition;
out vec3 EyeNormal;

uniform mat4 MODELMATRIX;                  // M     — world position (for the checker)
uniform mat4 MODELVIEWMATRIX;             // V * M — eye position (for lighting)
uniform mat4 MODELVIEWPROJECTIONMATRIX;   // P*V*M — clip position
uniform mat3 NORMALMATRIX;

void main()
{
    gl_Position   = MODELVIEWPROJECTIONMATRIX * vec4(VertexPosition, 1.0);
    WorldPosition = vec3(MODELMATRIX     * vec4(VertexPosition, 1.0));
    EyePosition   = vec3(MODELVIEWMATRIX * vec4(VertexPosition, 1.0));
    EyeNormal     = normalize(NORMALMATRIX * VertexNormal);
}
