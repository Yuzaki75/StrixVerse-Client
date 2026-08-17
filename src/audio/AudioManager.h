#pragma once

#include <string>
#include <vector>

struct SDL_AudioStream;

// -----------------------------------------------------------------------------
// AudioManager
//
// Looping background music, one track at a time.
//
// Ogg Vorbis is decoded up front with stb_vorbis - the same stb family already
// used for images, so no new dependency - and fed to an SDL3 audio stream in
// short chunks. Feeding in chunks rather than queueing the whole track keeps
// SDL's buffer small and makes looping a matter of wrapping the read cursor.
//
// Every path is non-fatal: a missing file, an undecodable file or a machine
// with no audio device leaves the game silent and running.
// -----------------------------------------------------------------------------
class AudioManager
{
public:
    AudioManager() = default;
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Opening the device is deferred until the first track, so a machine with
    // no audio hardware costs nothing.
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

    // Tops the stream up and wraps at the end of the track. Call once a frame.
    void Update();

private:
    bool OpenStream(int channels, int sampleRate);
    void CloseStream();

    // Queues up to `bytes` from the cursor, wrapping when the track ends.
    void FeedChunk();

    SDL_AudioStream* m_Stream = nullptr;

    std::vector<short> m_Samples;      // Interleaved 16-bit PCM.
    std::string        m_CurrentPath;

    int    m_Channels   = 0;
    int    m_SampleRate = 0;
    size_t m_Cursor     = 0;           // Index into m_Samples.

    float m_Volume = 0.7f;
};
