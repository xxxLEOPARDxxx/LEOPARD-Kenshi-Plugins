// LayerProbe - диагностика, не для выпуска (21-22.10.2026).
//
// Замер (tools/sampler.py) показал: ~13% главного потока игры уходит на то,
// что MyGUI заново собирает вершины слоёв интерфейса. Первая версия пробы
// (обход виджетов) нашла: каждый кадр меняются только подписи имён над
// персонажами на слое Back, и только пока движется камера. Значит, сборку
// вызывает не изменение виджетов.
//
// Вторая версия смотрит на сам рендер MyGUI:
//  * LayerManager::renderToTarget(target, update) - сколько раз за кадр и
//    с каким флагом update (true = пересобрать всё безусловно);
//  * SharedLayer / OverlappedLayer::renderToTarget - какой слой рисуется;
//  * RenderItem::renderToTarget - сколько пачек вершин пересобрано (по
//    слоям), сколько в них вершин и элементов.
// Третья версия (22.10): LayerNode::outOfDate - кто помечает слой
// изменённым: первый вызывающий вне MyGUIEngine (игра или плагин), и
// сколько пачек реально рисуется (вершин > 0 - это вызов отрисовки).
// Раз в 10 с - сводка в RE_Kenshi_log.txt.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

#include <cstdio>
#include <map>
#include <string>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_ILayer.h>
#include <mygui/MyGUI_ILayerNode.h>
#include <vector>
#include <algorithm>

namespace
{
    struct LayerStat
    {
        unsigned items, rebuilt, forced;    // пачек всего / пересобрано / из-за update
        unsigned drawn;                     // пачек с вершинами (вызовы отрисовки)
        unsigned long long verts, draws;    // вершин и элементов в пересобранных
    };

    std::map<std::string, LayerStat> g_layers;
    const MyGUI::ILayer* g_curLayer = NULL;
    // кто помечает: (слой, последний кадр в MyGUI, первый вне MyGUI) -> раз
    struct DirtyKey
    {
        const MyGUI::ILayer* layer;
        void* entry;
        void* from;
        bool operator<(const DirtyKey& o) const
        {
            if (layer != o.layer) return layer < o.layer;
            if (entry != o.entry) return entry < o.entry;
            return from < o.from;
        }
    };
    typedef std::map<DirtyKey, unsigned> DirtyMap;
    DirtyMap g_dirty;
    char* g_myguiLo = NULL;
    char* g_myguiHi = NULL;

    std::string Where(void* a)
    {
        HMODULE m = NULL;
        char name[MAX_PATH] = "?";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               static_cast<LPCSTR>(a), &m) && m != NULL)
        {
            GetModuleFileNameA(m, name, sizeof(name));
            const char* slash = strrchr(name, 0x5C);
            char buf[MAX_PATH + 32];
            sprintf_s(buf, "%s+0x%llX", slash != NULL ? slash + 1 : name,
                      static_cast<unsigned long long>(static_cast<char*>(a) - reinterpret_cast<char*>(m)));
            return buf;
        }
        return "?";
    }
    unsigned g_calls = 0, g_updates = 0;
    DWORD g_reportAt = 0;

    void Report()
    {
        char line[512];
        sprintf_s(line, "LayerProbe: %u renders in 10 s, update=true in %u", g_calls, g_updates);
        DebugLog(line);
        const unsigned n = g_calls != 0 ? g_calls : 1;
        for (std::map<std::string, LayerStat>::const_iterator it = g_layers.begin(); it != g_layers.end(); ++it)
        {
            const LayerStat& s = it->second;
            if (s.items == 0)
                continue;
            sprintf_s(line, "LayerProbe:   %-12s per render: items %.1f drawn %.1f rebuilt %.1f (forced %.1f) verts %.0f draws %.1f",
                      it->first.c_str(), double(s.items) / n, double(s.drawn) / n, double(s.rebuilt) / n,
                      double(s.forced) / n, double(s.verts) / n, double(s.draws) / n);
            DebugLog(line);
        }
        // кто помечает слои - 25 самых частых
        std::vector<std::pair<unsigned, DirtyKey > > top;
        for (DirtyMap::const_iterator it = g_dirty.begin();
             it != g_dirty.end(); ++it)
            top.push_back(std::make_pair(it->second, it->first));
        std::sort(top.rbegin(), top.rend());
        for (size_t i = 0; i < top.size() && i < 25; ++i)
        {
            const MyGUI::ILayer* const l = top[i].second.layer;
            _snprintf_s(line, _TRUNCATE, "LayerProbe:   dirty %-12s %7.1f per render  via %s  from %s",
                      l != NULL ? l->getName().c_str() : "?", double(top[i].first) / n,
                      Where(top[i].second.entry).c_str(), Where(top[i].second.from).c_str());
            DebugLog(line);
        }
        g_dirty.clear();
        g_layers.clear();
        g_calls = g_updates = 0;
    }

    // ---- LayerManager::renderToTarget
    void (*g_origLM)(void*, void*, bool) = NULL;
    void LM_hook(void* self, void* target, bool update)
    {
        ++g_calls;
        if (update)
            ++g_updates;
        g_origLM(self, target, update);
        g_curLayer = NULL;
        const DWORD now = GetTickCount();
        if (g_reportAt == 0)
            g_reportAt = now;
        if (now - g_reportAt >= 10000)
        {
            g_reportAt = now;
            Report();
        }
    }

    // ---- слои
    void (*g_origShared)(void*, void*, bool) = NULL;
    void Shared_hook(void* self, void* target, bool update)
    {
        g_curLayer = static_cast<const MyGUI::ILayer*>(self);
        g_origShared(self, target, update);
    }
    void (*g_origOverlapped)(void*, void*, bool) = NULL;
    void Overlapped_hook(void* self, void* target, bool update)
    {
        g_curLayer = static_cast<const MyGUI::ILayer*>(self);
        g_origOverlapped(self, target, update);
    }

    // ---- RenderItem::renderToTarget: +8 текстура, +0x18 mOutOfDate,
    // +0x20/+0x28 элементы (по 16 байт), +0x40 mCountVertex
    void (*g_origRI)(void*, void*, bool) = NULL;
    void RI_hook(void* self, void* target, bool update)
    {
        const char* const p = static_cast<const char*>(self);
        const bool tex = *reinterpret_cast<void* const*>(p + 8) != NULL;
        const bool outOfDate = *reinterpret_cast<const bool*>(p + 0x18);
        g_origRI(self, target, update);
        LayerStat& s = g_layers[g_curLayer != NULL ? g_curLayer->getName() : std::string("?")];
        ++s.items;
        if (*reinterpret_cast<const size_t*>(p + 0x40) != 0)
            ++s.drawn;
        if (tex && (outOfDate || update))
        {
            ++s.rebuilt;
            if (!outOfDate)
                ++s.forced;
            s.verts += *reinterpret_cast<const size_t*>(p + 0x40);
            const char* const b = *reinterpret_cast<const char* const*>(p + 0x20);
            const char* const e = *reinterpret_cast<const char* const*>(p + 0x28);
            s.draws += (e - b) / 16;
        }
    }

    // ---- LayerNode::outOfDate(RenderItem*) - сюда приходят все пометки
    const MyGUI::ILayer* LayerOf(void* node)
    {
        __try
        {
            return static_cast<MyGUI::ILayerNode*>(node)->getLayer();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return NULL;
        }
    }

    void (*g_origOOD)(void*, void*) = NULL;
    void OOD_hook(void* self, void* item)
    {
        void* frames[24];
        const USHORT got = CaptureStackBackTrace(1, 24, frames, NULL);
        void* from = NULL;
        void* entry = NULL;
        for (USHORT i = 0; i < got; ++i)
        {
            char* const a = static_cast<char*>(frames[i]);
            if (a < g_myguiLo || a >= g_myguiHi)
            {
                from = a;
                break;
            }
            entry = a;
        }
        const MyGUI::ILayer* const layer = LayerOf(self);
        const DirtyKey key = { layer, entry, from };
        ++g_dirty[key];
        g_origOOD(self, item);
    }

    // экспорт MyGUIEngine - это jmp-переходник (инкрементальная сборка); ставим
    // хук на саму функцию
    void* Export(HMODULE m, const char* name)
    {
        unsigned char* p = reinterpret_cast<unsigned char*>(GetProcAddress(m, name));
        if (p != NULL && p[0] == 0xE9)
            p = p + 5 + *reinterpret_cast<int*>(p + 1);
        return p;
    }

    bool Hook(HMODULE m, const char* name, void* hook, void* orig)
    {
        void* const at = Export(m, name);
        if (at == NULL || KenshiLib::SUCCESS != KenshiLib::AddHook(at, hook, static_cast<void**>(orig)))
        {
            ErrorLog(std::string("LayerProbe: could not hook ") + name);
            return false;
        }
        return true;
    }
}

__declspec(dllexport) void startPlugin()
{
    HMODULE const m = GetModuleHandleA("MyGUIEngine_x64.dll");
    if (m == NULL)
    {
        ErrorLog("LayerProbe: MyGUIEngine_x64.dll not loaded");
        return;
    }
    Hook(m, "?renderToTarget@LayerManager@MyGUI@@QEAAXPEAVIRenderTarget@2@_N@Z", LM_hook, &g_origLM);
    Hook(m, "?renderToTarget@SharedLayer@MyGUI@@UEAAXPEAVIRenderTarget@2@_N@Z", Shared_hook, &g_origShared);
    Hook(m, "?renderToTarget@OverlappedLayer@MyGUI@@UEAAXPEAVIRenderTarget@2@_N@Z", Overlapped_hook, &g_origOverlapped);
    Hook(m, "?renderToTarget@RenderItem@MyGUI@@QEAAXPEAVIRenderTarget@2@_N@Z", RI_hook, &g_origRI);
    MODULEINFO mi;
    if (GetModuleInformation(GetCurrentProcess(), m, &mi, sizeof(mi)))
    {
        g_myguiLo = static_cast<char*>(mi.lpBaseOfDll);
        g_myguiHi = g_myguiLo + mi.SizeOfImage;
        Hook(m, "?outOfDate@LayerNode@MyGUI@@UEAAXPEAVRenderItem@2@@Z", OOD_hook, &g_origOOD);
    }
    DebugLog("LayerProbe v4: installed - MyGUI render stats and who marks layers dirty, every 10 s");
}
