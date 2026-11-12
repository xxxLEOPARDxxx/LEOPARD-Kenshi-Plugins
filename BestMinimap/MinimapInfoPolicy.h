#pragma once

#include <cstddef>
#include <cstdio>
#include <cmath>

namespace BestMinimapInfo
{
    enum Fields
    {
        Scale = 1,
        Coordinates = 2,
        Heading = 4,
        Clock = 8
    };

    inline int Height(unsigned fields, int rowHeight)
    {
        int rows = 0;
        if (fields & Scale)
            ++rows;
        if (fields & Coordinates)
            rows += 2;
        if (fields & Heading)
            ++rows;
        if (fields & Clock)
            ++rows;
        return rows ? rows * rowHeight + 8 : 0;
    }

    struct Layout
    {
        int width;
        int height;
        int panelTop;
        int panelHeight;
    };

    inline Layout Plan(unsigned fields, int width, int height, int pad)
    {
        Layout layout = { width + 2 * pad, height + 2 * pad, 0, Height(fields, 16) };
        if (layout.panelHeight)
        {
            // The fixed corners of the vanilla thin panel need 98 pixels.
            if (layout.width < 100)
                layout.width = 100;
            layout.panelTop = layout.height + 3;
            layout.height = layout.panelTop + layout.panelHeight;
        }
        return layout;
    }

    // Stable reference: the plugin's default Range=2000 is always 100%.
    // Display dimensions/aspect do not change the zoom setting.
    inline double ZoomPercent(double range)
    {
        if (!(range > 0 && range < 1e9))
            return 0;
        return 200000.0 / range;
    }

    inline void ZoomText(char* text, size_t capacity, double range)
    {
        const double percent = ZoomPercent(range);
        if (percent <= 0)
            sprintf_s(text, capacity, "--");
        else if (percent < 10)
            sprintf_s(text, capacity, "%.1f%%", percent);
        else
            sprintf_s(text, capacity, "%.0f%%", percent);
    }

    // Kenshi positions use ten native world units per physical metre.
    inline int CoordinateMetres(double native)
    {
        if (!(native > -1e9 && native < 1e9))
            return 0;
        const double metres = native / 10;
        return static_cast<int>(metres + (metres < 0 ? -0.5 : 0.5));
    }

    // Map image direction: zero is up/north, increasing clockwise.
    // Return -1 when the camera has no usable horizontal direction.
    inline int Bearing(double dx, double dy)
    {
        if (!(dx > -1e9 && dx < 1e9 && dy > -1e9 && dy < 1e9) || dx * dx + dy * dy < 1e-18)
            return -1;
        double degrees = std::atan2(dx, -dy) * 57.29577951308232;
        if (degrees < 0)
            degrees += 360;
        return static_cast<int>(degrees + 0.5) % 360;
    }

    inline bool Due(unsigned now, unsigned last, bool changed)
    {
        return changed || now - last >= 250;
    }

    inline bool WithinMap(int x, int y, int width, int height)
    {
        return x >= 0 && y >= 0 && x < width && y < height;
    }
}
