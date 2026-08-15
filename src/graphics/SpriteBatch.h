#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "Texture.h"

class Shader;

// -----------------------------------------------------------------------------
// SpriteBatch
//
// Batches textured quads for the world/ECS render path.
//
// Draw order is insertion order: callers (RenderSystem sorts by layer, then by
// texture) decide the ordering, and the batch only breaks when the bound
// texture changes. End() submits the frame; there is no separate flush step to
// forget.
// -----------------------------------------------------------------------------
class SpriteBatch
{
public:
    explicit SpriteBatch(int maxSprites = 4096);
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    // The projection used for subsequent batches. Defaults to identity, so it
    // must be set once the window size (or camera) is known.
    void SetProjection(const glm::mat4& projection) { m_Projection = projection; }
    const glm::mat4& GetProjection() const { return m_Projection; }

    void Begin();

    void Draw(const Texture& texture, float x, float y, float width, float height,
              float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    // Submits everything queued since Begin().
    void End();

    // Submits without ending the batch. End() calls this.
    void Flush();

    size_t GetDrawCallCount() const { return m_DrawCallCount; }

private:
    struct Vertex
    {
        float x, y;
        float u, v;
        float r, g, b, a;
    };

    struct Batch
    {
        unsigned int textureID   = 0;
        size_t       firstVertex = 0;
        size_t       vertexCount = 0;
    };

    void InitRenderData();
    bool EnsureShader();

    std::vector<Vertex> m_Vertices;
    std::vector<Batch>  m_Batches;

    std::shared_ptr<Shader> m_Shader;
    bool                    m_ShaderLoadFailed = false;

    glm::mat4 m_Projection{1.0f};

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    size_t       m_VBOCapacityBytes = 0;

    int    m_MaxSprites   = 0;
    size_t m_DrawCallCount = 0;
    bool   m_CapacityWarned = false;
};
