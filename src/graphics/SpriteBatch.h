#pragma once

class Texture;

class SpriteBatch
{
public:
    bool Initialize();

    void Shutdown();

    void Begin();

    void Draw(
        Texture& texture,
        float x,
        float y,
        float width,
        float height);

    void End();

    void Flush();
};