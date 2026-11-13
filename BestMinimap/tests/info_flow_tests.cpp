#include <cstdio>
#include "../MinimapInfoPolicy.h"

int main()
{
    int checks = 0;
    int failures = 0;
#define C(condition) do { ++checks; if (!(condition)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)
    const int widths[4] = { 120, 100, 210, 88 };
    const int heights[4] = { 20, 20, 20, 20 };
    BestMinimapInfo::FlowLayout layout = BestMinimapInfo::Flow(widths, heights, 4, 420);
    C(layout.rows == 2);
    C(layout.height == 53);
    C(layout.fields[0].left == 8 && layout.fields[1].left == 140);
    C(layout.fields[2].left == 8 && layout.fields[3].left == 230);
    C(layout.fields[2].top == 28);
    layout = BestMinimapInfo::Flow(widths, heights, 1, 420);
    C(layout.rows == 1 && layout.height == 30);
    layout = BestMinimapInfo::Flow(widths, heights, 0, 420);
    C(layout.rows == 0 && layout.height == 0);
    for (int width = 80; width <= 420; width += 10)
    {
        for (int count = 1; count <= 4; ++count)
        {
            layout = BestMinimapInfo::Flow(widths, heights, count, width);
            C(layout.rows >= 1 && layout.rows <= count);
            for (int field = 0; field < count; ++field)
            {
                const BestMinimapInfo::FieldRect& rect = layout.fields[field];
                C(rect.width > 0 && rect.height == 20);
                C(rect.left >= 8 && rect.left + rect.width <= width - 8);
                C(rect.top >= 5 && rect.top + rect.height <= layout.height - 5);
                if (field && layout.fields[field - 1].top == rect.top)
                    C(layout.fields[field - 1].left + layout.fields[field - 1].width + 12 <= rect.left);
            }
        }
    }
    std::printf("%d info flow/overflow checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
