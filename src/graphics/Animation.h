#pragma once

#include <string>
#include <unordered_map>

// Texture lives at global scope, like SpriteBatch and Camera2D -- declaring it
// inside StrixVerse would silently create a second, unrelated type.
class Texture;

namespace StrixVerse
{
    namespace Graphics
    {
        // -------------------------------------------------------------------
        // AnimationClip
        // -------------------------------------------------------------------
        // One animation: a strip of equal-sized frames cut from one texture.
        //
        // Frames are laid out left-to-right, top-to-bottom, in a grid whose
        // column count follows from the sheet width and the frame width -- so
        // both a single row and a wrapped grid work without extra metadata.
        //
        // The sheet pointer is borrowed, not owned: clips point at textures
        // the AssetManager already keeps alive.
        // -------------------------------------------------------------------
        struct AnimationClip
        {
            const Texture* sheet = nullptr;
            int   frameWidth  = 0;
            int   frameHeight = 0;

            // Index of this clip's first frame within the sheet, counted in
            // the same left-to-right, top-to-bottom order as everything else.
            //
            // Without it every clip has to start at frame zero, which means a
            // separate sheet per clip - six body zones times three states is
            // eighteen files to keep in step instead of six.
            int   firstFrame  = 0;

            int   frameCount  = 1;
            float fps         = 8.0f;
            bool  loop        = true;
        };

        // -------------------------------------------------------------------
        // Animator
        // -------------------------------------------------------------------
        // Plays one of a named set of clips and tracks the current frame.
        //
        // Deliberately dumb: it owns playback state and UV maths, nothing
        // else. It does not draw -- a caller asks for the current frame's UVs
        // and hands them to the SpriteBatch -- and it does not decide *when*
        // a state changes; that lives with whoever knows about velocity and
        // grounding.
        //
        // Play() is a no-op when the named clip is already playing, so callers
        // can re-assert a state every frame without restarting it. A name that
        // was never added is ignored rather than asserted: a missing animation
        // should cost a still pose, not a crash.
        // -------------------------------------------------------------------
        class Animator
        {
        public:
            void AddClip(const std::string& name, const AnimationClip& clip);

            // Switches to the named clip, restarting it. No-op if the clip is
            // already the current one, or was never added.
            void Play(const std::string& name);

            // Advances playback by dt seconds.
            void Update(float dt);

            // Back to frame zero of the current clip (if any).
            void Reset();

            const std::string& CurrentClipName() const { return m_CurrentName; }
            int  CurrentFrame() const { return m_Frame; }

            // True once a non-looping clip has run past its last frame.
            bool Finished() const;

            // UV rectangle of one frame of the current clip, computed on
            // demand from the sheet's dimensions. Returns false when there is
            // no playable current clip (nothing added yet, or a sheetless
            // placeholder clip).
            bool GetFrameUV(int frameIndex,
                            float& u0, float& v0, float& u1, float& v1) const;

        private:
            const AnimationClip* CurrentClip() const;

            std::unordered_map<std::string, AnimationClip> m_Clips;
            std::string m_CurrentName;
            float m_Time  = 0.0f;   // seconds elapsed in the current clip
            int   m_Frame = 0;
        };
    }
}
