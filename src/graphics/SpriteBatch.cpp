#include "SpriteBatch.h"

#include "Texture.h"

bool SpriteBatch::Initialize()
{
    return true;
}

void SpriteBatch::Shutdown()
{
}

void SpriteBatch::Begin()
{
}

void SpriteBatch::Draw(
    Texture& texture,
    float x,
    float y,
    float width,
    float height)
{
    // TODO:
    // Store sprite in batch
}

void SpriteBatch::End()
{
    Flush();
}

void SpriteBatch::Flush()
{
    // TODO:
    // Upload vertices
    // Draw batched sprites
}