#version 330 core

// -----------------------------------------------------------------------------
// Sprite fragment shader (SpriteBatch / ECS RenderSystem).
// Samples the bound texture and modulates it with the per-vertex tint.
// -----------------------------------------------------------------------------

in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
    vec4 texel = texture(uTexture, vTexCoord);

    FragColor = texel * vColor;

    if (FragColor.a <= 0.0)
        discard;
}
