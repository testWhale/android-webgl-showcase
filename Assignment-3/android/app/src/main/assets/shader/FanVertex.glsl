#version 300 es

// FanVertex.glsl — Quiz 3 Part 2.
// Was: pass-through of a baked per-corner Shade factor. Now: outputs the data
// per-fragment Phong needs — the fragment position and normal in EYE space —
// so the fragment shader can run a real ambient+diffuse+specular light.

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;

// Eye-space position + normal handed to the fragment shader for lighting.
out vec3 EyePosition;
out vec3 EyeNormal;

uniform mat4 MODELVIEWPROJECTIONMATRIX;   // P * V * M — clip position
uniform mat4 MODELVIEWMATRIX;             // V * M     — eye-space position
uniform mat3 NORMALMATRIX;                // inverse-transpose of (V*M) upper 3x3

void main()
{
    // Position for rasterisation.
    gl_Position = MODELVIEWPROJECTIONMATRIX * vec4(VertexPosition, 1.0);

    // Position the light math works in: eye space.
    EyePosition = vec3(MODELVIEWMATRIX * vec4(VertexPosition, 1.0));

    // Normal transformed by the normal matrix, NOT passed through raw —
    // otherwise the normal stays frozen in the mesh's rest pose while the
    // part's model matrix rotates the geometry, and the lighting goes wrong
    // the instant a blade turns. NORMALMATRIX also corrects the non-uniform
    // scale every stretched part of the fan carries.
    EyeNormal = normalize(NORMALMATRIX * VertexNormal);
}
