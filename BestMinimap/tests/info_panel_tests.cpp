#include <cstdio>
#include <cstring>
#include <limits>
#include "../MinimapInfoPolicy.h"

int main()
{
    int failures = 0;
    int checks = 0;
#define C(condition) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        std::printf("FAIL line %d: %s\n", __LINE__, #condition); \
    } \
} while (0)
    using namespace BestMinimapInfo;

    C(Height(0, 16) == 0);
    C(Height(Scale, 16) == 24);
    C(Height(Coordinates, 16) == 40);
    C(Height(Scale | Coordinates | Heading | Clock, 16) == 88);

    Layout layout = Plan(0, 80, 80, 0);
    C(layout.width == 80 && layout.height == 80 && layout.panelHeight == 0);
    layout = Plan(Scale, 80, 80, 0);
    C(layout.width == 100 && layout.height == 107 && layout.panelTop == 83 && layout.panelHeight == 24);
    layout = Plan(Scale | Coordinates | Heading | Clock, 320, 200, 8);
    C(layout.width == 336 && layout.height == 307 && layout.panelTop == 219 && layout.panelHeight == 88);

    C(ZoomPercent(2000) == 100);
    C(ZoomPercent(1000) == 200);
    C(ZoomPercent(4000) == 50);
    C(ZoomPercent(30) > 6666.66 && ZoomPercent(30) < 6666.67);
    C(ZoomPercent(150000) > 1.333 && ZoomPercent(150000) < 1.334);
    C(ZoomPercent(0) == 0);
    C(ZoomPercent(-1) == 0);
    C(ZoomPercent(std::numeric_limits<double>::infinity()) == 0);

    char text[64];
    ZoomText(text, sizeof(text), 2000);
    C(std::strcmp(text, "100%") == 0);
    ZoomText(text, sizeof(text), 30);
    C(std::strcmp(text, "6667%") == 0);
    ZoomText(text, sizeof(text), 150000);
    C(std::strcmp(text, "1.3%") == 0);
    ZoomText(text, sizeof(text), 0);
    C(std::strcmp(text, "--") == 0);

    C(CoordinateMetres(-15) == -2);
    C(CoordinateMetres(15) == 2);
    C(CoordinateMetres(-4) == 0);
    C(Bearing(0, -1) == 0);
    C(Bearing(1, 0) == 90);
    C(Bearing(0, 1) == 180);
    C(Bearing(-1, 0) == 270);
    C(Bearing(-0.00001, -1) == 0);
    C(Bearing(0, 0) == -1);
    C(Bearing(std::numeric_limits<double>::infinity(), 0) == -1);

    C(!Due(1000, 900, false));
    C(Due(1000, 750, false));
    C(Due(1000, 999, true));
    C(Due(5, 0xffffff00u, false));
    C(WithinMap(0, 0, 80, 80));
    C(WithinMap(79, 79, 80, 80));
    C(!WithinMap(80, 40, 80, 80));
    C(!WithinMap(40, 80, 80, 80));
    C(!WithinMap(-1, 40, 80, 80));
    C(!WithinMap(40, 83, 80, 80));
    C(!WithinMap(40, -1, 80, 80));
    C(!WithinMap(0, 0, 0, 0));

    std::printf("%d info panel production checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
