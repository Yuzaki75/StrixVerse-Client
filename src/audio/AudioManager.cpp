#include "AudioManager.h"

#include "../core/Logger.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <format>

// stb_vorbis is a C file with its implementation inline. It is compiled here,
// in the one translation unit that needs it, with its warnings quietened - it
// is third-party code and the project builds at /W4.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244 4245 4267 4456 4457 4701 4703)
#endif

#define STB_VORBIS_NO_PUSHDATA_API
#include <stb_vorbis.c>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace
{
    // How much audio to keep queued. Small enough that a track change is
    // responsive, large enough to survive a frame spike without a gap.
    constexpr float kBufferedSeconds = 0.5f;
    constexpr float kChunkSeconds    = 0.25f;
}

AudioManager::~AudioManager()
{
    Shutdown();
}

void AudioManager::Shutdown()
{
    CloseStream();

    m_Samples.clear();
    m_Samples.shrink_to_fit();
    m_CurrentPath.clear();
}

void AudioManager::CloseStream()
{
    if (m_Stream)
    {
        SDL_DestroyAudioStream(m_Stream);
        m_Stream = nullptr;
    }
}

bool AudioManager::OpenStream(int channels, int sampleRate)
{
    CloseStream();

    SDL_AudioSpec spec{};
    spec.format   = SDL_AUDIO_S16;
    spec.channels = channels;
    spec.freq     = sampleRate;

    // A null callback means we push data ourselves from Update().
    m_Stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);

    if (!m_Stream)
    {
        LOG_WARN(std::string("AudioManager: no audio device available - ") + SDL_GetError());
        return false;
    }

    SDL_SetAudioStreamGain(m_Stream, m_Volume);
    SDL_ResumeAudioStreamDevice(m_Stream);

    return true;
}

bool AudioManager::PlayMusic(const std::string& path)
{
    // Already playing this track: leave it alone rather than restarting it.
    if (m_CurrentPath == path && m_Stream)
        return true;

    short* decoded  = nullptr;
    int    channels = 0;
    int    rate     = 0;

    const int frames = stb_vorbis_decode_filename(path.c_str(), &channels, &rate, &decoded);

    if (frames <= 0 || !decoded || channels <= 0 || rate <= 0)
    {
        // Missing or unreadable music is not worth failing a screen over.
        LOG_WARN(std::format("AudioManager: could not decode '{}'; continuing without music", path));

        if (decoded)
            free(decoded);

        return false;
    }

    m_Samples.assign(decoded, decoded + static_cast<size_t>(frames) * channels);
    free(decoded);

    m_Channels    = channels;
    m_SampleRate  = rate;
    m_Cursor      = 0;
    m_CurrentPath = path;

    if (!OpenStream(channels, rate))
    {
        m_Samples.clear();
        m_CurrentPath.clear();
        return false;
    }

    LOG_INFO(std::format("AudioManager: playing '{}' ({} Hz, {} ch, {:.1f}s)",
                         path, rate, channels,
                         static_cast<float>(frames) / static_cast<float>(rate)));

    // Prime the buffer so playback starts immediately.
    Update();

    return true;
}

void AudioManager::StopMusic()
{
    CloseStream();

    m_Samples.clear();
    m_CurrentPath.clear();
    m_Cursor = 0;
}

void AudioManager::SetMusicVolume(float volume)
{
    m_Volume = std::clamp(volume, 0.0f, 1.0f);

    if (m_Stream)
        SDL_SetAudioStreamGain(m_Stream, m_Volume);
}

void AudioManager::FeedChunk()
{
    const size_t chunkSamples =
        static_cast<size_t>(static_cast<float>(m_SampleRate) * kChunkSeconds) *
        static_cast<size_t>(m_Channels);

    if (chunkSamples == 0 || m_Samples.empty())
        return;

    // Wrap at the end of the track, which is what makes it loop.
    if (m_Cursor >= m_Samples.size())
        m_Cursor = 0;

    const size_t available = m_Samples.size() - m_Cursor;
    const size_t count     = std::min(chunkSamples, available);

    SDL_PutAudioStreamData(m_Stream,
                           m_Samples.data() + m_Cursor,
                           static_cast<int>(count * sizeof(short)));

    m_Cursor += count;
}

void AudioManager::Update()
{
    if (!m_Stream || m_Samples.empty() || m_SampleRate <= 0 || m_Channels <= 0)
        return;

    const int bytesPerSecond = m_SampleRate * m_Channels * static_cast<int>(sizeof(short));
    const int target         = static_cast<int>(static_cast<float>(bytesPerSecond) * kBufferedSeconds);

    // Bounded so a stall cannot spin here queueing the whole track.
    for (int guard = 0; guard < 8; ++guard)
    {
        if (SDL_GetAudioStreamAvailable(m_Stream) >= target)
            break;

        FeedChunk();
    }
}
