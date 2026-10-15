// RecruitMarkers - значок над головой у тех, кого можно нанять.
//
// Аналог LtEast's Recruitable NPC Highlighter, настройки - в MCM.
//
// КОГО СЧИТАЕМ НАНИМАЕМЫМ. Смотрим живой диалог персонажа (Character::
// dialogue), а не названия пакетов: разговоры на событие «игрок заговорил
// со мной» (EV_PLAYER_TALK_TO_ME), вглубь по веткам. Нанимаемый - если в
// дереве есть реплика с действием «вступить в отряд» (DA_JOIN_SQUAD_WITH_EDIT
// или DA_JOIN_SQUAD_FAST), и путь до неё открыт:
//   * реплика не заперта (Dialogue::isLocked) и не одноразовая уже сказанная;
//   * условия реплик на пути выполняются (DialogLineData::checkConditions,
//     собеседник - ближайший выделенный персонаж игрока). Так пленник,
//     раб, «за деньги» и прочие видны, когда игра и правда даст их нанять.
//     Условия можно выключить в MCM - тогда значок у всех, у кого найм
//     вообще есть в диалоге;
//   * разговор не для другого конкретного персонажа (isCharacter).
//
// НАГРУЗКА. Кандидаты (живые, не наши, в пределах MaxDistance от центра
// камеры) - раз в 0,75 с из списка обновляемых персонажей игры. Обход
// диалога - не больше двух персонажей за кадр, результат хранится 10 с.
// Значки - пул ImageBox на слое Back (как имена над головами у игры),
// ссылки - WidgetRef, без поиска по имени. Каждый кадр - только позиция
// головы и проекция на экран.

#define KLOC_DOMAIN "recruit_markers"
#include <Localization.h>
#include <ModConfigMenu.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <IniDefaults.h>
#include <HoldKey.h>
#include <WidgetRef.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_RenderManager.h>
#include <OgreResourceGroupManager.h>

// conversationsMain у Dialogue закрытый
#define private public
#define protected public
#include <kenshi/Dialogue.h>
#undef private
#undef protected

#include <kenshi/Kenshi.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/GameData.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/RootObjectBase.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>
#include <kenshi/util/UtilityT.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    HoldKey::Key g_holdKey;            // по умолчанию ALT; NONE - всегда
    bool g_showUnique = true;
    bool g_showGeneric = true;
    bool g_showAnimals = true;
    bool g_checkConditions = true;
    MyGUI::Colour g_uniqueColour(1.0f, 0.957f, 0.6f);
    MyGUI::Colour g_genericColour(1.0f, 0.745f, 0.4f);
    int g_iconSize = 40;
    float g_alpha = 0.8f;
    bool g_pulseUnique = true;
    float g_headGap = 0.4f;             // метры над костью головы по вертикали мира (VerticalOffset у LtEast)
    float g_maxDistance = 150.0f;       // метры от центра камеры
    int g_maxMarkers = 64;
    bool g_debug = false;

    const int kPoolMax = 128;
    const DWORD kCandidateMs = 750;
    const DWORD kCacheMs = 10000;
    const DWORD kForgetMs = 60000;
    const int kScansPerFrame = 2;
    const int kNodeLimit = 4000;        // реплик за один обход диалога
    const int kDepthLimit = 40;

    std::string ModulePath(const char* ext)
    {
        char path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return std::string();
        std::string file(path, length);
        const std::string::size_type dot = file.rfind('.');
        return dot == std::string::npos ? std::string() : file.substr(0, dot) + ext;
    }

    // Путь к ini игрока - см. shared/IniDefaults.h.
    std::string IniPath() { return IniDefaults::Prepare(ModulePath(".ini")); }

    const char* const kSec = "RecruitMarkers";

    int ReadInt(const std::string& ini, const char* key, int def, int lo, int hi)
    {
        int v = static_cast<int>(GetPrivateProfileIntA(kSec, key, def, ini.c_str()));
        return v < lo ? lo : (v > hi ? hi : v);
    }

    bool ReadBool(const std::string& ini, const char* key, bool def)
    {
        return GetPrivateProfileIntA(kSec, key, def ? 1 : 0, ini.c_str()) != 0;
    }

    MyGUI::Colour ReadColour(const std::string& ini, const char* key, const MyGUI::Colour& def)
    {
        char text[32] = {};
        GetPrivateProfileStringA(kSec, key, "", text, sizeof(text), ini.c_str());
        const char* p = text;
        while (*p == ' ' || *p == '#')
            ++p;
        unsigned int rgb = 0;
        if (strlen(p) < 6 || sscanf_s(p, "%6x", &rgb) != 1)
            return def;
        return MyGUI::Colour(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f,
                             (rgb & 0xFF) / 255.0f);
    }

    void HideAll();
    void ForgetCache();

    // Зовётся при запуске и из MCM после изменения настройки.
    void __cdecl LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        g_enabled = ReadBool(ini, "Enabled", true);
        char key[64] = {};
        GetPrivateProfileStringA(kSec, "HoldKey", "ALT", key, sizeof(key), ini.c_str());
        g_holdKey = HoldKey::Parse(key);
        g_showUnique = ReadBool(ini, "ShowUnique", true);
        g_showGeneric = ReadBool(ini, "ShowGeneric", true);
        g_showAnimals = ReadBool(ini, "ShowAnimals", true);
        const bool conditions = ReadBool(ini, "CheckConditions", true);
        if (conditions != g_checkConditions)
            ForgetCache();              // другое правило - пересчитать всех
        g_checkConditions = conditions;
        g_uniqueColour = ReadColour(ini, "UniqueColour", MyGUI::Colour(1.0f, 0.957f, 0.6f));
        g_genericColour = ReadColour(ini, "GenericColour", MyGUI::Colour(1.0f, 0.745f, 0.4f));
        g_iconSize = ReadInt(ini, "IconSize", 40, 12, 96);
        g_alpha = ReadInt(ini, "Opacity", 8, 1, 10) / 10.0f;
        g_pulseUnique = ReadBool(ini, "PulseUnique", true);
        g_headGap = ReadInt(ini, "HeightAboveHead", 40, 0, 300) / 100.0f;
        g_maxDistance = static_cast<float>(ReadInt(ini, "MaxDistance", 150, 20, 5000));
        g_maxMarkers = ReadInt(ini, "MaxMarkers", 64, 8, kPoolMax);
        g_debug = ReadBool(ini, "Debug", false);
        HideAll();                      // размер и цвет - при следующем кадре
    }

    // ---------------------------------------------------------------
    // Проекция на экран. Объект UtilityT игры не экспортирован - адрес
    // по версии игры, таблица из Vital-Sense (Emkej).
    // ---------------------------------------------------------------

    UtilityT* g_projection = NULL;
    bool g_projectionTried = false;

    UtilityT* Projection()
    {
        if (g_projectionTried)
            return g_projection;
        g_projectionTried = true;
        KenshiLib::BinaryVersion versionInfo = KenshiLib::GetKenshiVersion();
        const unsigned int platform = versionInfo.GetPlatform();
        const std::string version = versionInfo.GetVersion();
        uintptr_t utilityOffset = 0;
        if (platform == KenshiLib::BinaryVersion::STEAM)
        {
            if (version == "1.0.65")
                utilityOffset = 0x02134b10;
            else if (version == "1.0.68")
                utilityOffset = 0x02135b70;
        }
        else if (platform == KenshiLib::BinaryVersion::GOG)
        {
            if (version == "1.0.65")
                utilityOffset = 0x02132a80;
            else if (version == "1.0.68")
                utilityOffset = 0x02134aa0;
        }
        if (utilityOffset == 0)
        {
            ErrorLog("RecruitMarkers: unsupported game version " + versionInfo.ToString() + ", no markers");
            return NULL;
        }
        g_projection = reinterpret_cast<UtilityT*>(
            reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + utilityOffset);
        return g_projection;
    }

    bool SafeProject(UtilityT* utility, const Ogre::Vector3& pos, float& x, float& y)
    {
        __try
        {
            return utility->worldToScreenPX(pos, x, y);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // ---------------------------------------------------------------
    // Нанимаемость по диалогу
    // ---------------------------------------------------------------

    struct ScanState
    {
        Dialogue* dialogue;
        Character* target;              // собеседник - персонаж игрока
        GameData* me;                   // данные самого персонажа
        int nodes;
    };

    bool IsJoinLine(DialogLineData* line)
    {
        const lektor<DialogLineData::DialogAction*>& actions = line->actions;
        for (uint32_t i = 0; i < actions.size(); ++i)
        {
            const DialogLineData::DialogAction* a = actions[i];
            if (a != NULL && (a->key == DA_JOIN_SQUAD_WITH_EDIT || a->key == DA_JOIN_SQUAD_FAST))
                return true;
        }
        return false;
    }

    bool LineOpen(ScanState& s, DialogLineData* line, bool root)
    {
        if (s.dialogue->isLocked(line))
            return false;
        if (line->onceOnly && line->saidItBefore())
            return false;
        // Разговор для другого конкретного персонажа из общего пакета.
        if (root && s.me != NULL && line->hasSpecificCharacterRequirement() &&
            !line->isForSpecificCharacter(s.me))
            return false;
        if (g_checkConditions && s.target != NULL &&
            !line->checkConditions(s.dialogue, s.target, false))
            return false;
        return true;
    }

    bool FindJoin(ScanState& s, DialogChoiceList* list, int depth, bool root)
    {
        if (list == NULL || depth > kDepthLimit)
            return false;
        const lektor<DialogLineData*>& choices = list->conversationChoices;
        for (uint32_t i = 0; i < choices.size(); ++i)
        {
            DialogLineData* const line = choices[i];
            if (line == NULL || ++s.nodes > kNodeLimit)
                return false;
            if (!LineOpen(s, line, root))
                continue;
            if (IsJoinLine(line))
                return true;
            if (FindJoin(s, line->children, depth + 1, false))
                return true;
        }
        return false;
    }

    // 1 - нанимаемый, 0 - нет, -1 - диалог не прочитался (сбой)
    int ScanDialogueRaw(Character* c, Character* target, int& nodes)
    {
        Dialogue* const dialogue = c->dialogue;
        if (dialogue == NULL)
            return 0;
        ScanState s;
        s.dialogue = dialogue;
        s.target = target;
        s.me = c->data;
        s.nodes = 0;
        int found = 0;
        typedef std::map<EventTriggerEnum, DialogChoiceList*, std::less<EventTriggerEnum>,
                         Ogre::STLAllocator<std::pair<EventTriggerEnum const, DialogChoiceList*>,
                                            Ogre::GeneralAllocPolicy> > Lists;
        const Lists& lists = dialogue->conversationsMain;
        Lists::const_iterator it = lists.find(EV_PLAYER_TALK_TO_ME);
        if (it != lists.end() && FindJoin(s, it->second, 0, true))
            found = 1;
        nodes = s.nodes;
        return found;
    }

    int ScanDialogue(Character* c, Character* target, int& nodes)
    {
        __try
        {
            return ScanDialogueRaw(c, target, nodes);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return -1;
        }
    }

    // ---------------------------------------------------------------
    // Кандидаты и кэш
    // ---------------------------------------------------------------

    unsigned long long Key(const hand& h)
    {
        return (static_cast<unsigned long long>(h.index) << 32) | h.serial;
    }

    // Кость головы: -2 - ещё не искали, -1 - ни одной (берём позицию +
    // 2,2), 0..2 - номер в g_headBones. Ищется один раз на персонажа: на
    // ненайденной кости Ogre, судя по всему, бросает исключение - каждый
    // кадр для каждого животного это дорого.
    struct CacheEntry
    {
        bool recruitable;
        DWORD stamp;
        DWORD seen;
        int bone;
    };
    std::map<unsigned long long, CacheEntry> g_cache;

    struct Marker
    {
        hand who;
        bool unique;
        float distance;
        int bone;
    };
    std::vector<Marker> g_markers;      // кого показываем (по последнему обходу)
    std::vector<hand> g_queue;          // кого проверить
    DWORD g_lastCandidates = 0;
    unsigned g_scanned = 0, g_scanFailures = 0;

    void ForgetCache()
    {
        g_cache.clear();
        g_queue.clear();
        g_lastCandidates = 0;
    }

    // Уникальный - по флагу «unique» шаблона персонажа (как в редакторе
    // игры). Character::isUnique() для этого не годится: у двух наёмников
    // из одного шаблона он отвечал по-разному (15.10.2026).
    const std::string g_uniqueKey("unique");

    bool TemplateUnique(Character* c)
    {
        GameData* const d = c->data;
        if (d == NULL)
            return false;
        boost::unordered::unordered_map<std::string, bool, boost::hash<std::string>, std::equal_to<std::string>,
                                         Ogre::STLAllocator<std::pair<std::string const, bool>,
                                                            Ogre::GeneralAllocPolicy> >::const_iterator it =
            d->bdata.find(g_uniqueKey);
        return it != d->bdata.end() && it->second;
    }

    bool SafeFilter(Character* c, const Ogre::Vector3& camera, bool& unique, float& distance)
    {
        __try
        {
            if (c->isDead() || c->isPlayerCharacter())
                return false;
            if (c->isAnimal() != NULL && !g_showAnimals)
                return false;
            unique = TemplateUnique(c);
            if (unique ? !g_showUnique : !g_showGeneric)
                return false;
            const Ogre::Vector3 p = c->getPosition();
            distance = p.distance(camera);
            return distance <= g_maxDistance;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool ByDistance(const Marker& a, const Marker& b) { return a.distance < b.distance; }

    void RefreshCandidates(DWORD now)
    {
        g_markers.clear();
        g_queue.clear();
        const Ogre::Vector3 camera = ou->getCameraCenter();
        const ogre_unordered_set<Character*>::type& all = ou->getCharacterUpdateList();
        for (ogre_unordered_set<Character*>::type::const_iterator it = all.begin(); it != all.end(); ++it)
        {
            Character* const c = *it;
            bool unique = false;
            float distance = 0.0f;
            if (c == NULL || !SafeFilter(c, camera, unique, distance))
                continue;
            const hand& h = c->handle;
            std::map<unsigned long long, CacheEntry>::iterator e = g_cache.find(Key(h));
            if (e == g_cache.end() || now - e->second.stamp >= kCacheMs)
                g_queue.push_back(h);
            if (e != g_cache.end())
            {
                e->second.seen = now;
                if (e->second.recruitable)
                {
                    Marker m;
                    m.who = h;
                    m.unique = unique;
                    m.distance = distance;
                    m.bone = e->second.bone;
                    g_markers.push_back(m);
                }
            }
        }
        std::sort(g_markers.begin(), g_markers.end(), ByDistance);
        if (static_cast<int>(g_markers.size()) > g_maxMarkers)
            g_markers.resize(g_maxMarkers);
        // кого давно нет рядом - забываем
        for (std::map<unsigned long long, CacheEntry>::iterator e = g_cache.begin(); e != g_cache.end();)
        {
            if (now - e->second.seen >= kForgetMs)
                g_cache.erase(e++);
            else
                ++e;
        }
    }

    Character* Talker(Character* npc)
    {
        if (ou == NULL || ou->player == NULL)
            return NULL;
        const Ogre::Vector3 pos = npc->getPosition();
        Character* c = ou->player->getNearestSelectedCharacterTo(pos);
        if (c == NULL)
            c = ou->player->getNearestCharacterTo(pos);
        return c;
    }

    Character* SafeTalker(Character* npc)
    {
        __try
        {
            return Talker(npc);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return NULL;
        }
    }

    // Не больше двух персонажей и ~2 мс за кадр (первый - всегда).
    void ScanSome(DWORD now)
    {
        LARGE_INTEGER freq, start, cur;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        for (int n = 0; n < kScansPerFrame && !g_queue.empty(); ++n)
        {
            if (n > 0)
            {
                QueryPerformanceCounter(&cur);
                if ((cur.QuadPart - start.QuadPart) * 1000 > freq.QuadPart * 2)
                    break;
            }
            const hand h = g_queue.back();
            g_queue.pop_back();
            Character* const c = h.getCharacter();
            if (c == NULL)
                continue;
            Character* const target = g_checkConditions ? SafeTalker(c) : NULL;
            int nodes = 0;
            const int result = ScanDialogue(c, target, nodes);
            ++g_scanned;
            std::map<unsigned long long, CacheEntry>::iterator found = g_cache.find(Key(h));
            if (found == g_cache.end())
            {
                CacheEntry fresh;
                fresh.bone = -2;
                found = g_cache.insert(std::make_pair(Key(h), fresh)).first;
            }
            CacheEntry& e = found->second;
            e.recruitable = result == 1;
            e.stamp = now;
            e.seen = now;
            if (result < 0)
            {
                ++g_scanFailures;
                if (g_scanFailures <= 5)
                    ErrorLog("RecruitMarkers: a dialogue could not be read safely: " + c->getName());
            }
            else if (g_debug && result == 1)
                DebugLog(std::string("RecruitMarkers: recruitable ") + (TemplateUnique(c) ? "unique " : "generic ") +
                         c->getName());
        }
    }

    // ---------------------------------------------------------------
    // Значки
    // ---------------------------------------------------------------

    WidgetRef g_icons[kPoolMax];
    int g_shown = 0;                    // сколько первых значков видимы

    const std::string& IconTexture()
    {
        static std::string texture;
        static bool tried = false;
        if (tried)
            return texture;
        tried = true;
        const char* const names[] = { "gui/gfx/recruitmarkers_icon.png", "gui\\gfx\\recruitmarkers_icon.png",
                                      "recruitmarkers_icon.png" };
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]) && texture.empty(); ++i)
        {
            try
            {
                if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(names[i]))
                    texture = names[i];
            }
            catch (...)
            {
            }
        }
        if (texture.empty())
            ErrorLog("RecruitMarkers: gui/gfx/recruitmarkers_icon.png not found");
        return texture;
    }

    MyGUI::ImageBox* Icon(int i)
    {
        if (MyGUI::Widget* const w = g_icons[i].get())
            return static_cast<MyGUI::ImageBox*>(w);
        MyGUI::Gui* const gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
            return NULL;
        char name[48];
        sprintf_s(name, "RecruitMarkers_Icon_%d", i);
        MyGUI::ImageBox* image = NULL;
        try
        {
            image = gui->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, g_iconSize, g_iconSize),
                                                       MyGUI::Align::Default, "Back", name);
        }
        catch (...)
        {
            image = NULL;
        }
        if (image == NULL)
            return NULL;
        image->setNeedMouseFocus(false);
        image->setNeedKeyFocus(false);
        image->setImageTexture(IconTexture());
        image->setVisible(false);
        g_icons[i].set(image);
        return image;
    }

    void HideFrom(int from)
    {
        for (int i = from; i < g_shown; ++i)
            if (MyGUI::Widget* const w = g_icons[i].get())
                w->setVisible(false);
        if (from < g_shown)
            g_shown = from;
    }

    void HideAll() { HideFrom(0); }

    // Точка значка - как у LtEast (снято с его DLL, 16.10.2026): кость
    // «Bip01 Head», «Head» или «head», если она правдоподобна (не дальше 12
    // по горизонтали и от -4 до +20 по высоте от позиции персонажа, не в
    // самой позиции), плюс отступ по вертикали мира. Кости нет - позиция
    // персонажа + 2,2 + отступ. Значок стоит на точке низом, зазор 4 пикс.
    // (Свои варианты - отступ пикселями экрана и в масштабе персонажа -
    // при приближении и отдалении камеры уводили значок от головы.)
    const std::string g_headBones[3] = { "Bip01 Head", "Head", "head" };

    bool BoneOk(const Ogre::Vector3& bone, const Ogre::Vector3& base)
    {
        if (bone.x != bone.x || bone.y != bone.y || bone.z != bone.z)
            return false;
        const Ogre::Vector3 d = bone - base;
        if (d.x < -12.0f || d.x > 12.0f || d.z < -12.0f || d.z > 12.0f || d.y < -4.0f || d.y > 20.0f)
            return false;
        return d.squaredLength() > 0.04f;
    }

    // Одна кость, под SEH: ненайденная может бросить исключение.
    bool TryBone(Character* c, int index, const Ogre::Vector3& base, Ogre::Vector3& out)
    {
        __try
        {
            const Ogre::Vector3 bone = c->getBoneWorldPosition(g_headBones[index]);
            if (!BoneOk(bone, base))
                return false;
            out = bone;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool SafePosition(Character* c, Ogre::Vector3& out)
    {
        __try
        {
            out = c->getPosition();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // bone - из кэша персонажа; -2 - найти и запомнить.
    bool HeadPosition(Character* c, int& bone, Ogre::Vector3& out)
    {
        Ogre::Vector3 base;
        if (!SafePosition(c, base))
            return false;
        if (bone == -2)
        {
            bone = -1;
            for (int i = 0; i < 3 && bone < 0; ++i)
                if (TryBone(c, i, base, out))
                    bone = i;
        }
        // кость вдруг пропала (смена модели) - позиция + 2,2, как у LtEast
        if (bone < 0 || !TryBone(c, bone, base, out))
        {
            out = base;
            out.y += 2.2f;
        }
        out.y += g_headGap;
        return true;
    }

    // Последнее, что выставлено значку, - чтобы не трогать MyGUI без нужды.
    struct IconState
    {
        MyGUI::IntCoord coord;
        MyGUI::Colour colour;
        float alpha;
    };
    IconState g_iconState[kPoolMax];

    void Draw(DWORD now)
    {
        UtilityT* const utility = Projection();
        MyGUI::RenderManager* const render = MyGUI::RenderManager::getInstancePtr();
        if (utility == NULL || render == NULL)
        {
            HideAll();
            return;
        }
        const MyGUI::IntSize view = render->getViewSize();
        const float pulse = 0.5f + 0.5f * static_cast<float>(sin((now % 1600) / 1600.0 * 6.2831853));
        int drawn = 0;
        for (size_t i = 0; i < g_markers.size() && drawn < g_maxMarkers; ++i)
        {
            Marker& marker = g_markers[i];
            Character* const c = marker.who.getCharacter();
            Ogre::Vector3 head;
            const int boneBefore = marker.bone;
            if (c == NULL || !HeadPosition(c, marker.bone, head))
                continue;
            if (boneBefore != marker.bone)
            {
                std::map<unsigned long long, CacheEntry>::iterator e = g_cache.find(Key(marker.who));
                if (e != g_cache.end())
                    e->second.bone = marker.bone;
            }
            float x = 0.0f, y = 0.0f;
            if (!SafeProject(utility, head, x, y))
                continue;
            // облачко реплики из атласа игры, 38x36; низом на точке, зазор 4 - как у LtEast
            const int width = g_iconSize;
            const int height = g_iconSize * 36 / 38;
            const int left = static_cast<int>(x) - width / 2;
            const int top = static_cast<int>(y) - height - 4;
            if (left + width < 0 || top + height < 0 || left > view.width || top > view.height)
                continue;
            const bool created = g_icons[drawn].get() == NULL;
            MyGUI::ImageBox* const icon = Icon(drawn);
            if (icon == NULL)
                break;
            IconState& st = g_iconState[drawn];
            const MyGUI::IntCoord coord(left, top, width, height);
            const bool unique = marker.unique;
            const MyGUI::Colour& colour = unique ? g_uniqueColour : g_genericColour;
            float alpha = g_alpha;
            if (unique && g_pulseUnique)
                alpha *= 0.55f + 0.45f * pulse;
            if (created || st.coord != coord)
            {
                icon->setCoord(coord);
                st.coord = coord;
            }
            if (created || st.colour != colour)
            {
                icon->setColour(colour);
                st.colour = colour;
            }
            if (created || st.alpha != alpha)
            {
                icon->setAlpha(alpha);
                st.alpha = alpha;
            }
            if (!icon->getVisible())
                icon->setVisible(true);
            ++drawn;
        }
        if (drawn > g_shown)
            g_shown = drawn;
        HideFrom(drawn);
        g_shown = drawn;
    }

    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    DWORD g_lastWorldFrame = 0;
    DWORD g_lastDebug = 0;

    void (*g_origMainLoop)(GameWorld*, float) = NULL;
    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;

    void MainLoop_hook(GameWorld* self, float time)
    {
        g_origMainLoop(self, time);
        const DWORD now = GetTickCount();
        // Мир не крутился дольше 2 с (меню, загрузка) - персонажи могли
        // смениться: кэш - заново, даже если значки были скрыты.
        if (g_lastWorldFrame != 0 && now - g_lastWorldFrame > 2000)
        {
            ForgetCache();
            g_markers.clear();
        }
        g_lastWorldFrame = now;
        if (!g_enabled || ou == NULL || ou->player == NULL || !HoldKey::Held(g_holdKey))
        {
            HideAll();
            return;
        }
        if (g_lastCandidates == 0 || now - g_lastCandidates >= kCandidateMs)
        {
            g_lastCandidates = now;
            RefreshCandidates(now);
        }
        ScanSome(now);
        Draw(now);
        if (g_debug && now - g_lastDebug >= 10000)
        {
            g_lastDebug = now;
            char note[200];
            sprintf_s(note, "RecruitMarkers: %u markers, %u queued, %u cached, %u scans, %u failures",
                      static_cast<unsigned>(g_markers.size()), static_cast<unsigned>(g_queue.size()),
                      static_cast<unsigned>(g_cache.size()), g_scanned, g_scanFailures);
            DebugLog(note);
        }
    }

    // В главном меню и на загрузке цикл мира не идёт - значки прячем.
    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);
        if (g_shown > 0 && GetTickCount() - g_lastWorldFrame > 300)
        {
            HideAll();
            ForgetCache();              // другой сейв - другие персонажи
            g_markers.clear();
        }
    }
}


// Страница в ModConfigMenu (вкладка «Моды» в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "RecruitMarkers", "Recruit Markers", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("A marker over the heads of characters you can recruit."));
    api->section(api, Tr("General"));
    api->toggle(api, kSec, "Enabled", Tr("Enabled"), Tr("Markers over characters you can recruit."), 1, 0);
    api->hotkey(api, kSec, "HoldKey", Tr("Show key"), Tr("The markers are shown while this key is held. NONE (Backspace when assigning) - always shown."), "ALT", 0);
    api->section(api, Tr("Who"));
    api->toggle(api, kSec, "ShowUnique", Tr("Unique characters"), Tr("Named characters who can join you."), 1, 0);
    api->toggle(api, kSec, "ShowGeneric", Tr("Ordinary recruits"), Tr("Bar recruits, drifters and other ordinary characters who can join you."), 1, 0);
    api->toggle(api, kSec, "ShowAnimals", Tr("Animals"), Tr("Animals that can join you."), 1, 0);
    api->toggle(api, kSec, "CheckConditions", Tr("Only if recruitable right now"), Tr("Check the conditions of the dialogue as the game does: money, relations, imprisonment and so on, for your nearest selected character. Off - everyone whose dialogue can end with joining."), 1, 0);
    api->section(api, Tr("Look"));
    api->colour(api, kSec, "UniqueColour", Tr("Unique colour"), Tr("Marker colour of unique characters, #RRGGBB."), "#fff499", 0);
    api->colour(api, kSec, "GenericColour", Tr("Ordinary colour"), Tr("Marker colour of ordinary recruits and animals, #RRGGBB."), "#ffbe66", 0);
    api->integer(api, kSec, "IconSize", Tr("Marker size"), Tr("Width in pixels."), 40, 12, 96, 0);
    api->integer(api, kSec, "Opacity", Tr("Opacity"), Tr("1-10 (10% steps)."), 8, 1, 10, 0);
    api->toggle(api, kSec, "PulseUnique", Tr("Pulse on unique"), Tr("The markers of unique characters gently pulse."), 1, 0);
    api->integer(api, kSec, "HeightAboveHead", Tr("Height above the head, cm"), Tr("How high over the head bone the marker hangs."), 40, 0, 300, 0);
    api->integer(api, kSec, "MaxDistance", Tr("Distance, m"), Tr("Characters farther than this from the centre of the camera are not marked."), 150, 20, 5000, 0);
    api->integer(api, kSec, "MaxMarkers", Tr("Markers at most"), Tr("The nearest ones are shown."), 64, 8, 128, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, kSec, "Debug", Tr("Detailed log"), Tr("In RE_Kenshi_log.txt: who was found recruitable, and every 10 seconds how many are marked and checked."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();
    // Хуки - и при Enabled=0: выключатель проверяется в каждом кадре.
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff), MainLoop_hook, &g_origMainLoop))
    {
        ErrorLog("RecruitMarkers: could not hook the game loop, no markers");
        return;
    }
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ForgottenGUI::update), GuiUpdate_hook, &g_origGuiUpdate))
        ErrorLog("RecruitMarkers: could not hook the interface update - markers may stay in the main menu");
    DebugLog("RecruitMarkers: installed");
}
