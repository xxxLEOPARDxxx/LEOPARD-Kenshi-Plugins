#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <Debug.h>
#include <kenshi/Building/Building.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/OptionsHolder.h>
#include <kenshi/PlayerInterface.h>
#include "VeinSound.h"

#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_MAX_CHANNELS 2
#include "third_party/stb_vorbis.c"

#pragma comment(lib, "dsound.lib")

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace finite_ore {
namespace {

const unsigned VoiceCount = 4;
IDirectSound* device = NULL;
IDirectSoundBuffer* voices[VoiceCount] = {};
unsigned nextVoice = 0;
bool reportedPlaybackError = false;

bool ReadClip(std::vector<short>& samples, unsigned& rate)
{
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW((HMODULE)&__ImageBase, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
        return false;
    const std::wstring module(path, length);
    const size_t stop = module.find_last_of(L"\\/");
    if (stop == std::wstring::npos)
        return false;
    const std::wstring clip = module.substr(0, stop + 1) + L"audio\\Crash_Debris.ogg";
    std::ifstream file(clip.c_str(), std::ios::binary | std::ios::ate);
    const std::streamoff size = file.tellg();
    if (size <= 0 || size > 1024 * 1024)
        return false;
    std::vector<unsigned char> encoded(static_cast<size_t>(size));
    if (!file.seekg(0) || !file.read(reinterpret_cast<char*>(&encoded[0]), size))
        return false;

    // This is a short fixed Ogg Vorbis effect. Bound decoder storage as well
    // as the output duration before allocating the shared DirectSound buffer.
    std::vector<char> scratch(1024 * 1024);
    const stb_vorbis_alloc allocation = {&scratch[0], static_cast<int>(scratch.size())};
    std::unique_ptr<stb_vorbis, void (*)(stb_vorbis*)> decoder(
        stb_vorbis_open_memory(&encoded[0], static_cast<int>(size), NULL, &allocation),
        &stb_vorbis_close);
    if (!decoder)
        return false;
    const stb_vorbis_info info = stb_vorbis_get_info(decoder.get());
    if (info.channels != 2 || info.sample_rate < 8000 || info.sample_rate > 48000)
        return false;
    const unsigned frames = stb_vorbis_stream_length_in_samples(decoder.get());
    if (!frames || frames > 10 * info.sample_rate)
        return false;
    samples.resize(frames * 2);
    rate = info.sample_rate;
    const int decoded = stb_vorbis_get_samples_short_interleaved(
        decoder.get(), 2, &samples[0], static_cast<int>(samples.size()));
    return decoded == static_cast<int>(frames) && !stb_vorbis_get_error(decoder.get());
}

bool CreateVoices(HWND window)
{
    std::vector<short> samples;
    unsigned rate = 0;
    if (!ReadClip(samples, rate) ||
        FAILED(DirectSoundCreate(NULL, &device, NULL)) ||
        FAILED(device->SetCooperativeLevel(window, DSSCL_NORMAL)))
        return false;

    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2 * sizeof(short);
    format.nAvgBytesPerSec = rate * format.nBlockAlign;

    DSBUFFERDESC descriptor = {};
    descriptor.dwSize = sizeof(descriptor);
    descriptor.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_LOCSOFTWARE;
    descriptor.dwBufferBytes = static_cast<DWORD>(samples.size() * sizeof(short));
    descriptor.lpwfxFormat = &format;
    if (FAILED(device->CreateSoundBuffer(&descriptor, &voices[0], NULL)))
        return false;

    void* first = NULL;
    void* second = NULL;
    DWORD firstSize = 0, secondSize = 0;
    if (FAILED(voices[0]->Lock(0, descriptor.dwBufferBytes, &first, &firstSize, &second, &secondSize, 0)))
        return false;
    const char* const pcm = reinterpret_cast<const char*>(&samples[0]);
    std::memcpy(first, pcm, firstSize);
    if (secondSize)
        std::memcpy(second, pcm + firstSize, secondSize);
    if (FAILED(voices[0]->Unlock(first, firstSize, second, secondSize)))
        return false;

    for (unsigned i = 1; i < VoiceCount; ++i) {
        // DirectSound requires an initial volume change on duplicate buffers.
        if (FAILED(device->DuplicateSoundBuffer(voices[0], &voices[i])) ||
            FAILED(voices[i]->SetVolume(DSBVOLUME_MIN)))
            return false;
    }
    return true;
}

bool PlayClip(HWND window, LONG volume, LONG pan)
{
    if (!device && !CreateVoices(window))
        return false;

    IDirectSoundBuffer* const voice = voices[nextVoice];
    nextVoice = (nextVoice + 1) % VoiceCount;
    return SUCCEEDED(voice->Stop()) &&
           SUCCEEDED(voice->SetCurrentPosition(0)) &&
           SUCCEEDED(voice->SetVolume(volume)) &&
           SUCCEEDED(voice->SetPan(pan)) &&
           SUCCEEDED(voice->Play(0, 0, 0));
}

}

void ResetVeinSound()
{
    for (unsigned i = 0; i < VoiceCount; ++i) {
        if (voices[i]) {
            voices[i]->Stop();
            voices[i]->Release();
            voices[i] = NULL;
        }
    }
    if (device) {
        device->Release();
        device = NULL;
    }
    nextVoice = 0;
}

void PlayVeinBreak(Building& vein)
{
    if (!ou || !ou->player || !ou->player->getCamera() ||
        !options || !(options->sfxVolume > 0))
        return;

    const Ogre::Vector3 camera = ou->getCameraPos();
    const Ogre::Vector3 delta = vein.getPosition() - camera;
    const float range = 440.0f;
    const float distance = std::sqrt(delta.x * delta.x + delta.z * delta.z + delta.y * delta.y * 0.25f);
    if (!(distance < range))
        return;

    const HWND window = GetForegroundWindow();
    DWORD owner = 0;
    GetWindowThreadProcessId(window, &owner);
    if (owner != GetCurrentProcessId())
        return;

    const Ogre::Vector3 forward = ou->getCameraCenter() - camera;
    const float heading = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    const float side = heading > 0.01f && distance > 0.01f ?
        (delta.z * forward.x - delta.x * forward.z) / (heading * distance) : 0;
    const LONG pan = static_cast<LONG>(2000 * std::max(-1.0f, std::min(1.0f, side)));
    const float gain = std::min(options->sfxVolume, 1.0f) * (1 - distance / range);
    if (!(gain > 0))
        return;
    const LONG volume = static_cast<LONG>(std::max(-10000.0f, 2000.0f * std::log10(gain)));
    try {
        if (PlayClip(window, volume, pan))
            return;
    } catch (...) {
        // An audio device or clip failure must never prevent vein removal.
    }

    ResetVeinSound();
    if (!reportedPlaybackError) {
        reportedPlaybackError = true;
        ErrorLog("FiniteOreVeins: failed to play audio/Crash_Debris.ogg");
    }
}

}
