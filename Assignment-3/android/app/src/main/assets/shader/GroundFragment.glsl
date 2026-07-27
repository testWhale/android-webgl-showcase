#version 300 es

precision mediump float;

// GroundFragment.glsl — Quiz 3 Part 3.
// Procedural checkerboard (no texture) computed from world XZ cell parity,
// then fed THROUGH the same Phong light as the fan — so the floor is lit, not
// a flat two-colour mix. The parity picks the material colour; ambient +
// diffuse + specular then shade it exactly like every fan part.

in vec3 WorldPosition;
in vec3 EyePosition;
in vec3 EyeNormal;

uniform vec3 LIGHTPOSITION;   // light position in EYE space

layout(location = 0) out vec4 outColor;

const float TILE = 1.0;                      // world units per checker cell
const vec3  COLOR_A = vec3(0.85, 0.85, 0.88); // light square
const vec3  COLOR_B = vec3(0.12, 0.12, 0.14); // dark square

// Cell-parity checkerboard: 0.0 or 1.0 depending on which square we're in.
float checker(vec2 worldXZ, float tileSize)
{
    vec2  cell   = floor(worldXZ / tileSize);
    return mod(cell.x + cell.y, 2.0);
}

void main()
{
    float parity   = checker(WorldPosition.xz, TILE);
    vec3  material = mix(COLOR_A, COLOR_B, parity);

    vec3 N = normalize(EyeNormal);
    vec3 V = normalize(-EyePosition);
    vec3 L = normalize(LIGHTPOSITION - EyePosition);
    vec3 R = reflect(-L, N);

    vec3  ambient  = 0.25 * material;
    float diff     = max(dot(N, L), 0.0);
    vec3  diffuse  = diff * material;
    float spec     = pow(max(dot(R, V), 0.0), 16.0);
    vec3  specular = (diff > 0.0 ? spec : 0.0) * vec3(0.4);

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
