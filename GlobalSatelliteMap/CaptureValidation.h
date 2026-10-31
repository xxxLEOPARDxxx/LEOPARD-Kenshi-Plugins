#pragma once
#include "SatellitePolicy.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
namespace GSM
{
struct ValidationJob
{
    std::string folder, game, savePath, engine, error;
    Manifest manifest;
    LONG generation;
    HANDLE thread;
    bool ok;
    ValidationJob() : generation(0), thread(NULL), ok(false) {}
};
// Worker performs only bounded file reads and crypto. No game/Ogre/MyGUI calls.
extern volatile LONG generation;
DWORD WINAPI ValidateCapture(LPVOID input);
bool ReadSmall(const std::string &path, std::string &text, unsigned cap);
struct FileSnapshot
{
    std::string hash;
    unsigned long long stamp, size, readBytes;
    unsigned header[32];
    FileSnapshot() : stamp(0), size(0), readBytes(0)
    {
        for (int i = 0; i < 32; ++i)
            header[i] = 0;
    }
};
// Internal production file primitives, also linked by offline native regressions.
// Optional observers default to NULL; deterministic tests inject real file races/cancellation.
bool HashFileSnapshot(const std::string &path, FileSnapshot &out, unsigned long long &remaining,
                      const ValidationJob &job, void (*afterSnapshot)(void *) = NULL,
                      void *context = NULL);
bool EnumerateNativeFiles(const std::string &root, const std::string &relative,
                          std::vector<std::string> &files, int depth, unsigned &entriesRemaining,
                          const ValidationJob &job, void (*afterEntry)(void *) = NULL,
                          void *context = NULL);
} // namespace GSM
