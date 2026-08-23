#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct SDL_AudioStream;

// -----------------------------------------------------------------------------
// AudioManager
//
// Looping background music, one track at a time, plus short one-shot sound
// effects layered on top.
//
// Ogg Vorbis is decoded up front with stb_vorbis - the same stb family already
// used for images, so no new dependency - and fed to an SDL3 audio stream in
// short chunks. Feeding in chunks rather than queueing the whole track keeps
// SDL's buffer small and makes looping a matter of wrapping the read cursor.
//
// Sound effects are WAVs loaded lazily with SDL_LoadWAV and cached decoded.
// Each playing effect gets its own stream, all bound to the one shared audio
// device so SDL mixes them together; a stream is freed once it drains.
//
// Every path is non-fatal: a missing file, an undecodable file or a machine
// with no audio device leaves the game silent and running.
//
// Threading: the manager is not internally synchronised and expects all calls
// (music and effects alike) from the main/game thread, matching how screens
// and Engine use it. SDL's own device thread only touches streams through
// SDL's internals, which is safe.
// -----------------------------------------------------------------------------
class AudioManager
{
public:
    AudioManager() = default;
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Opening the device is deferred until the first track or effect, so a
    // machine with no audio hardware costs nothing.
    void Shutdown();

    // Starts a track looping. Asking for the track already playing is a no-op,
    // so a screen can call this every time it is entered without restarting
    // the music mid-phrase.
    bool PlayMusic(const std::string& path);

    void StopMusic();

    bool IsPlaying() const { return m_Stream != nullptr && !m_Samples.empty(); }
    const std::string& GetCurrentTrack() const { return m_CurrentPath; }

    // 0 silences, 1 is the decoded level. Applied live.
    void SetMusicVolume(float volume);
    float GetMusicVolume() const { return m_Volume; }

    // Plays a one-shot sound effect by base name, e.g. "punch", "ui_click".
    // Looks for Assets/audio/sfx/<name>.wav first and otherwise picks randomly
    // among Assets/audio/sfx/<name>_01.wav .. _NN.wav, giving free variation
    // when several takes exist. Returns false (silently logged) if there is
    // nothing to play.
    bool PlaySfx(const std::string& name);

    // 0 silences, 1 is the decoded level. Applied live, including to effects
    // that are already playing.
    void SetSfxVolume(float volume);
    float GetSfxVolume() const { return m_SfxVolume; }

    // Tops the music up, wraps at the end of the track and reaps finished
    // effects. Call once a frame.
    void Update();

private:
    bool OpenStream(int channels, int sampleRate);
    void CloseStream();

    // Opens the one playback device everything shares. No-op once open.
    bool EnsureDevice();

    // Queues up to `bytes` from the cursor, wrapping when the track ends.
    void FeedChunk();

    // One decoded WAV file. Format is kept raw; each voice's stream converts
    // to the device format, so mixed rates/channel counts all work.
    struct SfxSample
    {
        std::vector<unsigned char> data; // As returned by SDL_LoadWAV.
        int format   = 0;                // SDL_AudioFormat value.
        int channels = 0;
        int freq     = 0;
    };

    // SFX helpers.
    const std::vector<std::string>& FindSfxVariants(const std::string& name);
    const SfxSample* LoadSfxSample(const std::string& fileName);
    void FreeSfxStream(SDL_AudioStream*& stream);
    void UpdateSfx();

    SDL_AudioStream* m_Stream = nullptr;

    std::vector<short> m_Samples;      // Interleaved 16-bit PCM.
    std::string        m_CurrentPath;

    int    m_Channels   = 0;
    int    m_SampleRate = 0;
    size_t m_Cursor     = 0;           // Index into m_Samples.

    float m_Volume = 0.7f;

    // Shared playback device (SDL_AudioDeviceID). 0 until first opened.
    unsigned int m_DeviceId = 0;

    // Decoded samples, keyed by file name inside Assets/audio/sfx/.
    std::unordered_map<std::string, SfxSample> m_SfxCache;

    // File names per requested base name, scanned once at first use.
    std::unordered_map<std::string, std::vector<std::string>> m_SfxVariants;

    // Live one-shot streams, oldest first. Bounded so a burst of sounds
    // cannot pile up unbounded.
    std::vector<SDL_AudioStream*> m_SfxVoices;

    // Base names we have already logged as missing, so the log is written
    // once instead of every attempt.
    std::vector<std::string> m_MissingSfx;

    float m_SfxVolume = 1.0f;
};
