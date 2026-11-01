#pragma once
#include <cmath>
#include <cstddef>
namespace ProbePolicy
{
inline bool ValidShaderSource(const char *bytes, std::size_t size)
{
    if (!bytes || !size || size > 32768) return false;
    for (std::size_t i=0;i<size;++i)
        if (!bytes[i] || static_cast<unsigned char>(bytes[i]) > 127) return false;
    return true;
}
inline bool SafeAuxiliaryCleanup(bool hasWorkspace, const void *owner, const void *current)
{
    return !hasWorkspace || (owner && owner == current);
}
template <class T> inline bool Contains(const T *const *items, std::size_t count, const T *value)
{
    for (std::size_t i = 0; i < count; ++i)
        if (items[i] == value)
            return true;
    return false;
}
inline int Cell(double coordinate)
{
    return int(std::floor(coordinate / 1024.0));
}
inline double Centre(int cell)
{
    return (cell + 0.5) * 1024.0;
}
inline bool SafeCell(int cell)
{
    return cell >= -1000 && cell <= 1000;
}
inline bool ValidMode(int mode)
{
    return mode >= 0 && mode <= 4;
}
// Model of the installed perspective-only deferred reconstruction.
inline double NativeReconstructedX(double x, double height, double farClip)
{
    return x * std::sqrt(x * x + height * height) / std::sqrt(x * x + farClip * farClip);
}
} // namespace ProbePolicy
