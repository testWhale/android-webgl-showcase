#version 300 es

precision mediump float;

// FanFragment.glsl — Quiz 3 Part 2.
// Real per-fragment Phong (ambient + diffuse + specular). PARTCOLOR — the same
// per-part colour Quiz 2 uploaded — is now fed in as the material colour that
// the ambient and diffuse terms tint, instead of being multiplied by a baked
// grayscale gradient.

in vec3 EyePosition;
in vec3 EyeNormal;

uniform vec3 PARTCOLOR;        // material colour for this part
uniform vec3 LIGHTPOSITION;    // light position in EYE space (V * worldLight)

layout(location = 0) out vec4 outColor;

void main()
{
    // Re-normalise: interpolation across the triangle shortens the normal.
    vec3 N = normalize(EyeNormal);

    // In eye space the camera is at the origin, so the view direction is
    // simply the direction from the fragment back to the origin.
    vec3 V = normalize(-EyePosition);
    vec3 L = normalize(LIGHTPOSITION - EyePosition);
    vec3 R = reflect(-L, N);

    // ---- Ambient: a constant floor so the dark side is never pure black.
    vec3 ambient = 0.20 * PARTCOLOR;

    // ---- Diffuse: Lambert, tinted by the part colour.
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * PARTCOLOR;

    // ---- Specular: white highlight, only where the surface faces the light.
    float spec = pow(max(dot(R, V), 0.0), 32.0);
    vec3 specular = (diff > 0.0 ? spec : 0.0) * vec3(1.0);

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
