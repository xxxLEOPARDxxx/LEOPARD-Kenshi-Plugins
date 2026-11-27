#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <Debug.h>
#include <kenshi/Building/Building.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/OptionsHolder.h>
#include <kenshi/PlayerInterface.h>
#include "VeinSound.h"

#pragma comment(lib, "dsound.lib")

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace finite_ore {
namespace {

const unsigned VoiceCount = 4;
IDirectSound* device = NULL;
IDirectSoundBuffer* voices[VoiceCount] = {};
unsigned nextVoice = 0;
bool reportedPlaybackError = false;

struct WaveHeader {
    DWORD riff, riffSize, wave, format, formatSize;
    WORD codec, channels;
    DWORD sampleRate, bytesPerSecond;
    WORD blockAlign, bitsPerSample;
    DWORD data, dataSize;
};
static_assert(sizeof(WaveHeader) == 44, "Expected the PCM WAV header layout");

bool ReadClip(WaveHeader& header, std::vector<char>& samples)
{
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW((HMODULE)&__ImageBase, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
        return false;
    const std::wstring module(path, length);
    const size_t stop = module.find_last_of(L"\\/");
    if (stop == std::wstring::npos)
        return false;
    const std::wstring clip = module.substr(0, stop + 1) + L"audio\\Crash_Debris.wav";

    std::ifstream file(clip.c_str(), std::ios::binary);
    // The packaged clip uses a canonical stereo PCM header. Bound reads in
    // case the asset is replaced with an unsupported or damaged file.
    if (!file.read(reinterpret_cast<char*>(&header), sizeof(header)) ||
        header.riff != mmioFOURCC('R','I','F','F') ||
        header.wave != mmioFOURCC('W','A','V','E') ||
        header.format != mmioFOURCC('f','m','t',' ') || header.formatSize != 16 ||
        header.data != mmioFOURCC('d','a','t','a') || header.codec != WAVE_FORMAT_PCM ||
        header.channels != 2 || header.bitsPerSample != 16 || header.blockAlign != 4 ||
        header.sampleRate != 44100 || header.bytesPerSecond != 44100 * 4 ||
        !header.dataSize || header.dataSize > 4 * 1024 * 1024 ||
        header.dataSize % header.blockAlign || header.riffSize != header.dataSize + 36)
        return false;
    samples.resize(header.dataSize);
    return file.read(&samples[0], header.dataSize).good();
}

bool CreateVoices(HWND window)
{
    WaveHeader header;
    std::vector<char> samples;
    if (!ReadClip(header, samples) ||
        FAILED(DirectSoundCreate(NULL, &device, NULL)) ||
        FAILED(device->SetCooperativeLevel(window, DSSCL_NORMAL)))
        return false;

    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = header.channels;
    format.nSamplesPerSec = header.sampleRate;
    format.nAvgBytesPerSec = header.bytesPerSecond;
    format.nBlockAlign = header.blockAlign;
    format.wBitsPerSample = header.bitsPerSample;

    DSBUFFERDESC descriptor = {};
    descriptor.dwSize = sizeof(descriptor);
    descriptor.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_LOCSOFTWARE;
    descriptor.dwBufferBytes = header.dataSize;
    descriptor.lpwfxFormat = &format;
    if (FAILED(device->CreateSoundBuffer(&descriptor, &voices[0], NULL)))
        return false;

    void* first = NULL;
    void* second = NULL;
    DWORD firstSize = 0, secondSize = 0;
    if (FAILED(voices[0]->Lock(0, header.dataSize, &first, &firstSize, &second, &secondSize, 0)))
        return false;
    std::memcpy(first, &samples[0], firstSize);
    if (secondSize)
        std::memcpy(second, &samples[firstSize], secondSize);
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
        ErrorLog("FiniteOreVeins: failed to play audio/Crash_Debris.wav");
    }
}

}
