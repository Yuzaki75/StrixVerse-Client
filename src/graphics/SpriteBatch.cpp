#include "SpriteBatch.h"

#include "Shader.h"
#include "../core/AssetManager.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"

#include <glad/glad.h>

#include <algorithm>

namespace
{
    constexpr size_t kVerticesPerSprite = 6;
}

SpriteBatch::SpriteBatch(int maxSprites)
    : m_MaxSprites(std::max(1, maxSprites))
{
    InitRenderData();
}

SpriteBatch::~SpriteBatch()
{
    if (m_VBO != 0)
        glDeleteBuffers(1, &m_VBO);

    if (m_VAO != 0)
        glDeleteVertexArrays(1, &m_VAO);
}

void SpriteBatch::InitRenderData()
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    m_VBOCapacityBytes = static_cast<size_t>(m_MaxSprites) * kVerticesPerSprite * sizeof(Vertex);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_VBOCapacityBytes), nullptr, GL_STREAM_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, x)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, u)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, r)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_Vertices.reserve(static_cast<size_t>(m_MaxSprites) * kVerticesPerSprite);
}

bool SpriteBatch::EnsureShader()
{
    if (m_Shader)
        return true;

    if (m_ShaderLoadFailed)
        return false;

    auto assets = ServiceLocator::Get<AssetManager>();
    if (!assets)
        return false;

    m_Shader = assets->LoadShader("shaders/default.vert", "shaders/default.frag");
    if (!m_Shader)
    {
        // Only complain once; the frame loop would otherwise spam the log.
        m_ShaderLoadFailed = true;
        Logger::Error("SpriteBatch: failed to load shaders/default.vert + shaders/default.frag.");
        return false;
    }

    return true;
}

void SpriteBatch::Begin()
{
    m_Vertices.clear();
    m_Batches.clear();
    m_DrawCallCount = 0;
}

void SpriteBatch::Draw(const Texture& texture, float x, float y, float width, float height,
                       float r, float g, float b, float a)
{
    if (width <= 0.0f || height <= 0.0f || a <= 0.0f)
        return;

    const size_t spriteCount = m_Vertices.size() / kVerticesPerSprite;
    if (spriteCount >= static_cast<size_t>(m_MaxSprites))
    {
        if (!m_CapacityWarned)
        {
            m_CapacityWarned = true;
            Logger::Warning("SpriteBatch: sprite capacity reached; further sprites this frame are dropped.");
        }
        return;
    }

    const unsigned int textureID = texture.GetRendererID();

    // Start a new run whenever the texture changes; insertion order is kept so
    // the caller's layer sorting survives.
    if (m_Batches.empty() || m_Batches.back().textureID != textureID)
    {
        Batch batch;
        batch.textureID   = textureID;
        batch.firstVertex = m_Vertices.size();
        batch.vertexCount = 0;
        m_Batches.push_back(batch);
    }

    const float x1 = x + width;
    const float y1 = y + height;

    m_Vertices.push_back({x,  y,  0.0f, 0.0f, r, g, b, a});
    m_Vertices.push_back({x1, y,  1.0f, 0.0f, r, g, b, a});
    m_Vertices.push_back({x,  y1, 0.0f, 1.0f, r, g, b, a});

    m_Vertices.push_back({x1, y,  1.0f, 0.0f, r, g, b, a});
    m_Vertices.push_back({x1, y1, 1.0f, 1.0f, r, g, b, a});
    m_Vertices.push_back({x,  y1, 0.0f, 1.0f, r, g, b, a});

    m_Batches.back().vertexCount += kVerticesPerSprite;
}

void SpriteBatch::End()
{
    Flush();
}

void SpriteBatch::Flush()
{
    if (m_Vertices.empty())
        return;

    if (!EnsureShader())
    {
        m_Vertices.clear();
        m_Batches.clear();
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_Shader->Bind();
    m_Shader->SetMat4("uProjection", m_Projection);
    m_Shader->SetInt("uTexture", 0);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    const size_t requiredBytes = m_Vertices.size() * sizeof(Vertex);
    if (requiredBytes > m_VBOCapacityBytes)
        m_VBOCapacityBytes = requiredBytes * 2;

    // Orphan then refill: avoids stalling on the previous frame's draw.
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_VBOCapacityBytes), nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(requiredBytes), m_Vertices.data());

    glActiveTexture(GL_TEXTURE0);

    for (const Batch& batch : m_Batches)
    {
        if (batch.vertexCount == 0)
            continue;

        glBindTexture(GL_TEXTURE_2D, batch.textureID);

        glDrawArrays(GL_TRIANGLES,
                     static_cast<GLint>(batch.firstVertex),
                     static_cast<GLsizei>(batch.vertexCount));

        ++m_DrawCallCount;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    m_Shader->Unbind();

    m_Vertices.clear();
    m_Batches.clear();
}
