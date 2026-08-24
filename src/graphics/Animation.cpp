#include "Animation.h"

#include "Texture.h"

#include <algorithm>

namespace StrixVerse
{
    namespace Graphics
    {
        void Animator::AddClip(const std::string& name, const AnimationClip& clip)
        {
            m_Clips[name] = clip;
        }

        void Animator::Play(const std::string& name)
        {
            if (name == m_CurrentName)
            {
                return;
            }
            if (m_Clips.find(name) == m_Clips.end())
            {
                return;
            }

            m_CurrentName = name;
            Reset();
        }

        void Animator::Update(float dt)
        {
            const AnimationClip* clip = CurrentClip();
            if (!clip || clip->frameCount <= 0 || clip->fps <= 0.0f)
            {
                return;
            }

            // A sheetless clip still counts time, so a placeholder state
            // machine behaves identically to a real one; only the draw is
            // missing.
            m_Time += dt;

            const float frameFloat = m_Time * clip->fps;
            if (clip->loop)
            {
                m_Frame = static_cast<int>(frameFloat) % clip->frameCount;
            }
            else
            {
                m_Frame = std::min(static_cast<int>(frameFloat),
                                   clip->frameCount - 1);
            }
        }

        void Animator::Reset()
        {
            m_Time  = 0.0f;
            m_Frame = 0;
        }

        bool Animator::Finished() const
        {
            const AnimationClip* clip = CurrentClip();
            if (!clip || clip->loop || clip->frameCount <= 0)
            {
                return false;
            }

            return static_cast<int>(m_Time * clip->fps) >= clip->frameCount;
        }

        bool Animator::GetFrameUV(int frameIndex,
                                  float& u0, float& v0, float& u1, float& v1) const
        {
            const AnimationClip* clip = CurrentClip();
            if (!clip || !clip->sheet || clip->frameCount <= 0 ||
                clip->frameWidth <= 0 || clip->frameHeight <= 0)
            {
                return false;
            }

            // Clamped within the clip, then shifted to where the clip
            // actually sits in the sheet. frameIndex is the caller's frame
            // number inside this clip, never a sheet position.
            const int index =
                clip->firstFrame + std::clamp(frameIndex, 0, clip->frameCount - 1);

            // Column count follows from the sheet itself, so a strip that wraps
            // onto a second row needs no extra metadata.
            const int columns = std::max(1,
                static_cast<int>(clip->sheet->GetWidth()) / clip->frameWidth);
            const int rows = std::max(1,
                static_cast<int>(clip->sheet->GetHeight()) / clip->frameHeight);

            const int col = index % columns;
            const int row = index / std::max(1, columns);
            const int clampedRow = std::min(row, rows - 1);

            const float sheetW = static_cast<float>(clip->sheet->GetWidth());
            const float sheetH = static_cast<float>(clip->sheet->GetHeight());

            u0 = static_cast<float>(col) * clip->frameWidth / sheetW;
            u1 = static_cast<float>(col + 1) * clip->frameWidth / sheetW;
            v0 = static_cast<float>(clampedRow) * clip->frameHeight / sheetH;
            v1 = static_cast<float>(clampedRow + 1) * clip->frameHeight / sheetH;

            return true;
        }

        const AnimationClip* Animator::CurrentClip() const
        {
            if (m_CurrentName.empty())
            {
                return nullptr;
            }

            const auto it = m_Clips.find(m_CurrentName);
            return it != m_Clips.end() ? &it->second : nullptr;
        }
    }
}
