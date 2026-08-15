#version 330 core

// -----------------------------------------------------------------------------
// Sprite vertex shader (SpriteBatch / ECS RenderSystem).
//
// Matches SpriteBatch's interleaved vertex layout exactly:
//   position (vec2) | texCoord (vec2) | colour (vec4)
// -----------------------------------------------------------------------------

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 uProjection;

out vec2 vTexCoord;
out vec4 vColor;

void main()
{
    vTexCoord = aTexCoord;
    vColor    = aColor;

    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
