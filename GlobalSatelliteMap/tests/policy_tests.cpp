#include "../SatellitePolicy.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
int checks = 0;
void check(bool ok, const char *name)
{
    ++checks;
    if (!ok)
    {
        std::cerr << "FAIL " << name << "\n";
        std::exit(1);
    }
}
std::string valid()
{
    std::ostringstream s;
    s << "GSM1\nworld " << std::string(64, 'a') << "\nsave " << std::string(64, 'b')
      << "\nbounds -100 -200 300 200\ngrid 4 4 8192\nlighting static\n";
    s << "source data/mods.cfg " << std::string(64, 'c') << "\nsource data/gamedata.base "
      << std::string(64, 'd') << "\nsource data/Dialogue.mod " << std::string(64, 'e') << "\n";
    for (int z = 0; z < 4; ++z)
        for (int x = 0; x < 4; ++x)
            for (int l = 0; l < 6; ++l)
                s << "asset " << x << " " << z << " " << l << " " << (8192 >> l) << " t" << x << z
                  << l << ".dds " << std::string(64, 'f') << "\n";
    return s.str();
}
int main()
{
    using namespace GSM;
    Manifest m;
    std::string err;
    check(Parse(valid(), m, err), "complete manifest");
    std::string crlf = valid();
    for (size_t at = 0; (at = crlf.find('\n', at)) != crlf.npos; at += 2)
        crlf.insert(at, "\r");
    check(Parse(crlf, m, err), "Windows CRLF manifest");
    unsigned header[32] = {0};
    header[0] = 0x20534444;
    header[1] = 124;
    header[2] = 0x1007;
    header[3] = header[4] = 256;
    header[19] = 32;
    header[20] = 4;
    header[21] = 0x31545844;
    header[27] = 0x1000;
    check(DDSHeader(header, 128 + Bytes(256), 256), "valid bounded BC1 header");
    header[2] = 0;
    check(!DDSHeader(header, 128 + Bytes(256), 256), "missing required DDS flags rejected");
    header[2] = 0x1007;
    check(!DDSHeader(header, 127 + Bytes(256), 256), "truncated BC1 rejected");
    header[28] = 512;
    check(!DDSHeader(header, 128 + Bytes(256), 256), "cubemap rejected");
    header[28] = 0;
    header[7] = 2;
    check(!DDSHeader(header, 128 + Bytes(256), 256), "unexpected mip chain rejected");
    header[7] = 0;
    header[4] = 8192;
    check(!DDSHeader(header, 128 + Bytes(256), 256), "wrong header dimensions rejected");
    check(m.assets.size() == 96, "exact asset count");
    check(Tile(-100, -100, 300) == 0, "lower edge");
    check(Tile(0, -100, 300) == 1, "internal edge belongs right");
    check(Tile(300, -100, 300) == -1, "upper edge excluded");
    check(Tile(-101, -100, 300) == -1, "outside excluded");
    check(Tile(0, 0, 0) == -1, "degenerate bounds");
    check(ChooseLevel(16, 8192, 96u * 1024u * 1024u) >= 2, "aggregate visible cap");
    check(ChooseLevel(1, 8192, 96u * 1024u * 1024u) == 0, "one 8K permitted");
    check(ChooseLevel(16, 1, 96u * 1024u * 1024u) == 5, "minimum level");
    check(!SafeName("../bad.dds"), "traversal");
    check(!SafeName("C:bad.dds"), "drive");
    check(!SafeName("tile/evil.dds"), "nested asset");
    check(!SafeRelative("../a"), "source traversal");
    check(!SafeRelative("a/../b"), "source embedded traversal");
    check(!SafeRelative("/a"), "absolute source");
    check(SafeRelative("data/gamedata.base"), "relative source");
    check(!Parse(valid() + "unknown yes\n", m, err), "unknown fields");
    check(!Parse(valid() + "asset 0 0 0 8192 repeat.dds " + std::string(64, 'f') + "\n", m, err),
          "duplicate assets");
    std::string a = valid();
    a.replace(a.find("grid 4 4"), 8, "grid 8 8");
    check(!Parse(a, m, err), "wrong grid");
    a = valid();
    a.replace(a.find("lighting static"), 15, "lighting live  ");
    check(!Parse(a, m, err), "unsupported native lighting");
    a = valid();
    a.erase(a.rfind("asset "));
    check(!Parse(a, m, err), "missing mip");
    a = valid();
    a.replace(a.find("-100 -200 300 200"), 17, "nan  -200 300 200");
    check(!Parse(a, m, err), "nonfinite bounds");
    check(!Parse(std::string(65537, 'x'), m, err), "manifest byte cap");
    check(Parse(valid(), m, err), "parse reset after failure");
    for (int z = 0; z < 4; ++z)
        for (int x = 0; x < 4; ++x)
        {
            Rect r = TileRect(x, z, 400, 400);
            check(r.x == x * 100 && r.y == z * 100 && r.w == 100 && r.h == 100,
                  "exact grid placement");
        }
    check(!Intersects(Rect(0, 0, 100, 100), Rect(100, 0, 100, 100)), "touching edge not visible");
    check(Intersects(Rect(0, 0, 100, 100), Rect(99, 99, 100, 100)), "one pixel overlap visible");
    Session state;
    check(state.Change("a", 1), "initial identity");
    unsigned epoch = state.epoch;
    check(!state.Change("a", 1) && state.epoch == epoch, "same identity stable");
    check(state.Change("b", 1), "save switch");
    epoch = state.epoch;
    state.Reset();
    check(state.epoch == epoch + 1 && state.key.empty(), "load invalidation");
    check(state.Change("b", 2), "same save new world");
    std::cout << checks << " production-policy checks passed\n";
}
