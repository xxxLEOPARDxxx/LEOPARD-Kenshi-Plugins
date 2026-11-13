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
    C(layout.height == 68);
    C(layout.fields[0].left == 94 && layout.fields[1].left == 226);
    C(layout.fields[2].left == 55 && layout.fields[3].left == 277);
    C(layout.fields[2].top == 36);
    layout = BestMinimapInfo::Flow(widths, heights, 1, 420);
    C(layout.rows == 1 && layout.height == 44 && layout.fields[0].left == 150);
    layout = BestMinimapInfo::Flow(widths, heights, 0, 420);
    C(layout.rows == 0 && layout.height == 0);
    for (int width = 80; width <= 420; width += 10)
    {
        for (int count = 1; count <= 4; ++count)
        {
            layout = BestMinimapInfo::Flow(widths, heights, count, width);
            C(layout.rows >= 1 && layout.rows <= count);
            for (int first = 0; first < count; )
            {
                int end = first;
                while (end + 1 < count && layout.fields[end + 1].top == layout.fields[first].top)
                    ++end;
                const int right = width - layout.fields[end].left - layout.fields[end].width;
                C(layout.fields[first].left >= right && layout.fields[first].left - right <= 1 ||
                  right >= layout.fields[first].left && right - layout.fields[first].left <= 1);
                first = end + 1;
            }
            for (int field = 0; field < count; ++field)
            {
                const BestMinimapInfo::FieldRect& rect = layout.fields[field];
                C(rect.width > 0 && rect.height == 20);
                C(rect.left >= 12 && rect.left + rect.width <= width - 12);
                C(rect.top >= 12 && rect.top + rect.height <= layout.height - 12);
                if (field && layout.fields[field - 1].top == rect.top)
                    C(layout.fields[field - 1].left + layout.fields[field - 1].width + 12 <= rect.left);
            }
        }
    }
    std::printf("%d info flow/overflow checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
