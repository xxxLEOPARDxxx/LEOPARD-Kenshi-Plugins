#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <map>

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Item.h>
#include <kenshi/gui/InventoryGUI.h>
#include <mygui/MyGUI_SkinManager.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_WidgetManager.h>

#include "ExtraDurability.h"

namespace durability
{
namespace
{
const int TrackHeight = 4;
const int OverlayDepth = -1000;
const DWORD RefreshInterval = 125;
const unsigned RefreshBatch = 64;

struct BarGeometry
{
    int left;
    int top;
    int width;
    int fill;
};

bool ComputeBarGeometry(int width, int height, int position, float ratio, BarGeometry& bar)
{
    if (width < 14 || height < 15 || (position != 1 && position != 2) || !(ratio >= 0.0f))
        return false;

    const int side = width < 50 ? 5 : width >= 88 ? 22 : width / 4;
    const int space = width - 2 * side - 2;
    const float bounded = ratio > 1.0f ? 1.0f : ratio;
    const int fill = static_cast<int>(bounded * space + 0.5f);
    bar.left = side;
    bar.top = position == 1 ? 5 : height - 6 - TrackHeight;
    bar.width = space + 2;
    bar.fill = bounded > 0.0f && fill == 0 ? 1 : fill;
    return true;
}

struct Icon
{
    Item* identity;
    hand handle;
    MyGUI::Widget* track;
    int colourBand;

    explicit Icon(Item* item) : identity(item), handle(item->getHandle()), track(NULL), colourBand(-1) {}
};

typedef std::map<MyGUI::Widget*, Icon> Icons;
typedef std::map<MyGUI::Widget*, MyGUI::Widget*> TrackParents;
Icons icons;
Icons::iterator nextIcon = icons.end();
TrackParents trackParents;
MyGUI::WidgetManager* registeredWith = NULL;
enum SkinState { Unchecked, Available, Missing };
SkinState whiteSkin = Unchecked;

class Unlinker : public MyGUI::IUnlinkWidget
{
public:
    virtual void _unlinkWidget(MyGUI::Widget* widget)
    {
        const TrackParents::iterator parent = trackParents.find(widget);
        if (parent != trackParents.end()) {
            const Icons::iterator icon = icons.find(parent->second);
            if (icon != icons.end()) {
                icon->second.track = NULL;
                icon->second.colourBand = -1;
            }
            trackParents.erase(parent);
            return;
        }

        const Icons::iterator icon = icons.find(widget);
        if (icon == icons.end())
            return;
        if (nextIcon == icon)
            ++nextIcon;
        if (icon->second.track)
            trackParents.erase(icon->second.track);
        icons.erase(icon);
    }
};

Unlinker unlinker;

bool EnsureUnlinker()
{
    MyGUI::WidgetManager* const manager = MyGUI::WidgetManager::getInstancePtr();
    if (!manager)
        return false;
    if (registeredWith != manager) {
        icons.clear();
        nextIcon = icons.end();
        trackParents.clear();
        manager->registerUnlinker(&unlinker);
        registeredWith = manager;
        whiteSkin = Unchecked;
    }
    return true;
}

Item* ResolveItem(const Icon& icon)
{
    __try {
        Item* const item = icon.handle.getItem();
        return item == icon.identity ? item : NULL;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
}

void ReportErrorOnce(const char* message, bool& reported)
{
    if (!reported) {
        reported = true;
        ErrorLog(message);
    }
}

MyGUI::Widget* CreateTrack(MyGUI::Widget* root, const MyGUI::IntCoord& coord)
{
    if (whiteSkin == Missing)
        return NULL;
    if (whiteSkin == Unchecked) {
        MyGUI::SkinManager* const skins = MyGUI::SkinManager::getInstancePtr();
        if (!skins)
            return NULL;
        if (!skins->isExist("WhiteSkin")) {
            whiteSkin = Missing;
            ErrorLog("ExtraDurability: WhiteSkin missing, cannot show equipment condition bars");
            return NULL;
        }
        whiteSkin = Available;
    }

    MyGUI::Widget* const track = root->createWidget<MyGUI::Widget>("WhiteSkin", coord,
                                                                  MyGUI::Align::Left | MyGUI::Align::Top);
    if (!track)
        return NULL;
    try {
        trackParents.insert(std::make_pair(track, root));
        track->setUserString("ExtraDurability.ConditionBar", "1");
        track->setNeedMouseFocus(false);
        track->setNeedKeyFocus(false);
        // ItemMarkers uses +10000 for the rarity frame and -10000 for
        // badges. Our inset strip sits above the item and below the badges.
        track->setDepth(OverlayDepth);
    }
    catch (...) {
        MyGUI::WidgetManager::getInstance().destroyWidget(track);
        throw;
    }
    return track;
}

void HideBar(Icon& icon)
{
    if (icon.track && icon.track->getVisible())
        icon.track->setVisible(false);
}

void RefreshBar(MyGUI::Widget* root, Icon& icon, int position)
{
    if (!root->getInheritedVisible())
        return;
    Item* const item = ResolveItem(icon);
    const float ratio = item ? EquipmentConditionRatio(item) : -1.0f;
    BarGeometry geometry;
    if (!ComputeBarGeometry(root->getWidth(), root->getHeight(), position, ratio, geometry)) {
        HideBar(icon);
        return;
    }

    const MyGUI::IntCoord target(geometry.left, geometry.top, geometry.width, TrackHeight);
    if (!icon.track)
        icon.track = CreateTrack(root, target);
    MyGUI::Widget* const track = icon.track;
    if (!track)
        return;

    if (track->getCoord() != target)
        track->setCoord(target);

    const int band = !geometry.fill ? 0 : ratio > 0.5f ? 3 : ratio > 0.25f ? 2 : 1;
    if (icon.colourBand < 0 || (band == 0) != (icon.colourBand == 0))
        track->setColour(band ? MyGUI::Colour(0.08f, 0.08f, 0.08f)
                              : MyGUI::Colour(0.54f, 0.12f, 0.09f));

    // The fill is the only child of our track. Look it up on demand so an
    // independent MyGUI unlink can never leave a cached fill pointer.
    MyGUI::Widget* fill = track->getChildCount() ? track->getChildAt(0) : NULL;
    if (geometry.fill) {
        const MyGUI::IntCoord fillCoord(1, 1, geometry.fill, TrackHeight - 2);
        const bool createFill = fill == NULL;
        if (createFill) {
            fill = track->createWidget<MyGUI::Widget>("WhiteSkin", fillCoord,
                                                     MyGUI::Align::Left | MyGUI::Align::Top);
            if (fill) {
                fill->setNeedMouseFocus(false);
                fill->setNeedKeyFocus(false);
            }
        }
        if (fill) {
            if (fill->getCoord() != fillCoord)
                fill->setCoord(fillCoord);
            if (createFill || band != icon.colourBand) {
                const MyGUI::Colour colours[] = {
                    MyGUI::Colour(0.86f, 0.27f, 0.18f),
                    MyGUI::Colour(0.91f, 0.67f, 0.20f),
                    MyGUI::Colour(0.38f, 0.82f, 0.29f)
                };
                fill->setColour(colours[band - 1]);
            }
            if (!fill->getVisible())
                fill->setVisible(true);
        }
    }
    else if (fill && fill->getVisible())
        fill->setVisible(false);
    if (!track->getVisible())
        track->setVisible(true);
    icon.colourBand = band;
}

InventoryIcon* (*previousConstructor)(InventoryIcon*, Item*, const MyGUI::IntPoint&, MyGUI::Widget*) = NULL;

InventoryIcon* TrackIcon(InventoryIcon* self, Item* item, const MyGUI::IntPoint& position, MyGUI::Widget* parent)
{
    InventoryIcon* const result = previousConstructor(self, item, position, parent);
    if (!result || !HasConditionBar(item))
        return result;

    try {
        MyGUI::Widget* const root = result->getWidget();
        // Track equipment roots even while switched off to support live MCM
        // enabling. MyGUI destruction retires them without a close hook.
        if (root && EnsureUnlinker())
            icons.insert(std::make_pair(root, Icon(item)));
    }
    catch (...) {
        static bool reported = false;
        ReportErrorOnce("ExtraDurability: could not track an equipment icon", reported);
    }
    return result;
}
}

void TickConditionBars(float)
{
    static int lastPosition = 0;
    static DWORD lastRefresh = 0;
    if (icons.empty() || whiteSkin == Missing)
        return;

    const int position = ConditionBarPosition();
    if (!position) {
        if (lastPosition)
            for (Icons::iterator it = icons.begin(); it != icons.end(); ++it)
                HideBar(it->second);
        lastPosition = 0;
        return;
    }

    const DWORD now = GetTickCount();
    if (position == lastPosition && now - lastRefresh < RefreshInterval)
        return;
    lastPosition = position;
    lastRefresh = now;

    if (nextIcon == icons.end())
        nextIcon = icons.begin();
    // Inventory/trade windows may show hundreds of items. Update one
    // bounded slice of the existing icon roots at a time.
    for (unsigned i = 0; i < RefreshBatch && nextIcon != icons.end(); ++i) {
        const Icons::iterator current = nextIcon++;
        try {
            RefreshBar(current->first, current->second, position);
        }
        catch (...) {
            static bool reported = false;
            ReportErrorOnce("ExtraDurability: could not draw an equipment condition bar", reported);
        }
    }
}

void InstallConditionBar()
{
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&InventoryIcon::_CONSTRUCTOR),
                           &TrackIcon, &previousConstructor) != KenshiLib::SUCCESS)
        ErrorLog("ExtraDurability: equipment condition icon hook failed");
}
}
