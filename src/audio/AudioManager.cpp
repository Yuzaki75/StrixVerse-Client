#include "AudioManager.h"

#include "../core/Logger.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <random>

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

    // Where effect WAVs live, relative to the working directory, matching how
    // music paths are handed to PlayMusic.
    constexpr const char* kSfxDir       = "Assets/audio/sfx";
    constexpr std::size_t kMaxSfxVoices = 16;

    // Case-insensitive ASCII compare, enough for file extensions.
    bool EqualsInsensitive(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
            return false;

        for (std::size_t i = 0; i < a.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i])))
                return false;
        }

        return true;
    }

    // True when `fileName` is `<name>.wav` or `<name>_NN.wav` (digits only),
    // which keeps unrelated files such as pickupCoin.wav out of the "coin"
    // variant list while accepting punch_01.wav .. punch_12.wav.
    bool MatchesSfxName(const std::string& fileName, const std::string& name)
    {
        const std::filesystem::path path(fileName);
        const std::string extension = path.extension().string();
        const std::string stem      = path.stem().string();

        if (!EqualsInsensitive(extension, ".wav"))
            return false;

        if (stem == name)
            return true;

        if (stem.size() <= name.size() + 1 ||
            stem.compare(0, name.size(), name) != 0 ||
            stem[name.size()] != '_')
            return false;

        for (std::size_t i = name.size() + 1; i < stem.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(stem[i])))
                return false;
        }

        return true;
    }

    // Random index into `count` entries, used to pick an effect variation.
    std::size_t RandomIndex(std::size_t count)
    {
        static std::mt19937 rng{ std::random_device{}() };

        std::uniform_int_distribution<std::size_t> dist(0, count - 1);
        return dist(rng);
    }
}

AudioManager::~AudioManager()
{
    Shutdown();
}

void AudioManager::Shutdown()
{
    CloseStream();

    for (SDL_AudioStream* stream : m_SfxVoices)
        FreeSfxStream(stream);
    m_SfxVoices.clear();

    m_Samples.clear();
    m_Samples.shrink_to_fit();
    m_CurrentPath.clear();

    m_SfxCache.clear();
    m_SfxVariants.clear();
    m_MissingSfx.clear();

    if (m_DeviceId)
    {
        SDL_CloseAudioDevice(m_DeviceId);
        m_DeviceId = 0;
    }
}

void AudioManager::CloseStream()
{
    if (m_Stream)
    {
        SDL_DestroyAudioStream(m_Stream);
        m_Stream = nullptr;
    }
}

bool AudioManager::EnsureDevice()
{
    if (m_DeviceId)
        return true;

    // A null spec lets SDL pick the device's native format. Each bound stream
    // converts into whatever that ends up being, so nothing else has to care.
    m_DeviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

    if (!m_DeviceId)
    {
        LOG_WARN(std::string("AudioManager: no audio device available - ") + SDL_GetError());
        return false;
    }

    return true;
}

bool AudioManager::OpenStream(int channels, int sampleRate)
{
    CloseStream();

    if (!EnsureDevice())
        return false;

    SDL_AudioSpec src{};
    src.format   = SDL_AUDIO_S16;
    src.channels = channels;
    src.freq     = sampleRate;

    m_Stream = SDL_CreateAudioStream(&src, nullptr);

    if (!m_Stream)
    {
        LOG_WARN(std::string("AudioManager: could not create audio stream - ") + SDL_GetError());
        return false;
    }

    // Binding sets the device end of the stream and starts it flowing. Devices
    // open unpaused in SDL3, so no resume call is needed.
    if (!SDL_BindAudioStream(m_DeviceId, m_Stream))
    {
        LOG_WARN(std::string("AudioManager: could not bind audio stream - ") + SDL_GetError());
        SDL_DestroyAudioStream(m_Stream);
        m_Stream = nullptr;
        return false;
    }

    SDL_SetAudioStreamGain(m_Stream, m_Volume);

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
    if (m_Stream && !m_Samples.empty() && m_SampleRate > 0 && m_Channels > 0)
    {
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

    UpdateSfx();
}

// -----------------------------------------------------------------------------
// Sound effects.
// -----------------------------------------------------------------------------

const std::vector<std::string>& AudioManager::FindSfxVariants(const std::string& name)
{
    const auto cached = m_SfxVariants.find(name);
    if (cached != m_SfxVariants.end())
        return cached->second;

    static const std::vector<std::string> kNone;

    // One directory scan per base name, on first use only.
    std::vector<std::string> files;

    std::error_code ec;
    std::filesystem::directory_iterator dir(kSfxDir,
                                            std::filesystem::directory_options::skip_permission_denied,
                                            ec);

    if (!ec)
    {
        for (const std::filesystem::directory_entry& entry : dir)
        {
            std::error_code fileEc;
            if (!entry.is_regular_file(fileEc) || fileEc)
                continue;

            const std::string fileName = entry.path().filename().string();

            if (MatchesSfxName(fileName, name))
                files.push_back(fileName);
        }
    }

    std::sort(files.begin(), files.end());

    // Nothing on disk: warn once per base name so spammy callers stay quiet.
    if (files.empty())
    {
        if (std::find(m_MissingSfx.begin(), m_MissingSfx.end(), name) == m_MissingSfx.end())
        {
            LOG_WARN(std::format("AudioManager: no sfx found for '{}'", name));
            m_MissingSfx.push_back(name);
        }

        return kNone;
    }

    return m_SfxVariants.emplace(name, std::move(files)).first->second;
}

const AudioManager::SfxSample* AudioManager::LoadSfxSample(const std::string& fileName)
{
    const auto cached = m_SfxCache.find(fileName);
    if (cached != m_SfxCache.end())
        return &cached->second;

    SDL_AudioSpec spec{};
    Uint8* buffer     = nullptr;
    Uint32 length     = 0;

    const std::string path = std::string(kSfxDir) + "/" + fileName;

    if (!SDL_LoadWAV(path.c_str(), &spec, &buffer, &length) || !buffer || length == 0)
    {
        LOG_WARN(std::format("AudioManager: could not load '{}'; continuing without it", path));

        if (buffer)
            SDL_free(buffer);

        return nullptr;
    }

    SfxSample sample;
    sample.data.assign(buffer, buffer + length);
    sample.format   = static_cast<int>(spec.format);
    sample.channels = spec.channels;
    sample.freq     = spec.freq;
    SDL_free(buffer);

    return &m_SfxCache.emplace(fileName, std::move(sample)).first->second;
}

void AudioManager::FreeSfxStream(SDL_AudioStream*& stream)
{
    if (!stream)
        return;

    // Unbinding first stops the device thread reading while we destroy; both
    // calls tolerate a stream that was never bound.
    SDL_UnbindAudioStream(stream);
    SDL_DestroyAudioStream(stream);
    stream = nullptr;
}

bool AudioManager::PlaySfx(const std::string& name)
{
    if (name.empty())
        return false;

    const std::vector<std::string>& variants = FindSfxVariants(name);
    if (variants.empty())
        return false;

    // Random take when several files exist, plain otherwise.
    const std::string& fileName =
        variants.size() == 1 ? variants.front() : variants[RandomIndex(variants.size())];

    const SfxSample* sample = LoadSfxSample(fileName);
    if (!sample)
        return false;

    if (!EnsureDevice())
        return false;

    // Cap concurrent one-shots by dropping the oldest, so rapid retriggering
    // stays responsive instead of stacking up latency.
    if (m_SfxVoices.size() >= kMaxSfxVoices)
    {
        FreeSfxStream(m_SfxVoices.front());
        m_SfxVoices.erase(m_SfxVoices.begin());
    }

    SDL_AudioSpec src{};
    src.format   = static_cast<SDL_AudioFormat>(sample->format);
    src.channels = sample->channels;
    src.freq     = sample->freq;

    SDL_AudioStream* stream = SDL_CreateAudioStream(&src, nullptr);

    if (!stream)
    {
        LOG_WARN(std::format("AudioManager: could not create sfx stream for '{}' - {}",
                             fileName, SDL_GetError()));
        return false;
    }

    // Binding sets the device end of the stream, which is where any needed
    // format/rate/channel conversion happens; every bound stream is mixed
    // together by SDL on the one shared device.
    if (!SDL_BindAudioStream(m_DeviceId, stream))
    {
        LOG_WARN(std::format("AudioManager: could not bind sfx stream for '{}' - {}",
                             fileName, SDL_GetError()));
        SDL_DestroyAudioStream(stream);
        return false;
    }

    SDL_SetAudioStreamGain(stream, m_SfxVolume);

    // The whole clip goes in once; the voice dies when the queue drains.
    if (!SDL_PutAudioStreamData(stream, sample->data.data(),
                                static_cast<int>(sample->data.size())) ||
        !SDL_FlushAudioStream(stream))
    {
        LOG_WARN(std::format("AudioManager: could not queue sfx '{}' - {}",
                             fileName, SDL_GetError()));
        FreeSfxStream(stream);
        return false;
    }

    m_SfxVoices.push_back(stream);

    return true;
}

void AudioManager::SetSfxVolume(float volume)
{
    m_SfxVolume = std::clamp(volume, 0.0f, 1.0f);

    for (SDL_AudioStream* stream : m_SfxVoices)
        SDL_SetAudioStreamGain(stream, m_SfxVolume);
}

void AudioManager::UpdateSfx()
{
    for (auto it = m_SfxVoices.begin(); it != m_SfxVoices.end();)
    {
        // Fully drained once nothing is queued: unbind and free it.
        if (SDL_GetAudioStreamQueued(*it) <= 0)
        {
            FreeSfxStream(*it);
            it = m_SfxVoices.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
