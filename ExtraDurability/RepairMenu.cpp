// SPDX-License-Identifier: GPL-3.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ExtraDurability.h"

#include <windows.h>

#include <algorithm>
#include <set>
#include <vector>

#include <Debug.h>

#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Gear.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/gui/ContextMenu.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_RenderManager.h>

namespace durability {
namespace {

const char RepairPanelTag[] = "ExtraDurabilityRepairPanel";
const std::string RepairPickerName("ExtraDurabilityRepairItemPicker");

typedef void (*ShowContextMenu)(ContextMenu*, bool, RootObject*);
typedef void (*UpdateContextMenu)(ContextMenu*);

ShowContextMenu originalShowContextMenu = 0;
UpdateContextMenu originalUpdateContextMenu = 0;
bool pickerOpen = false;

enum RepairCategory { RepairWeapon, RepairArmour };

typedef std::vector<Gear*> GearList;

struct RepairChoices
{
    GearList weapons;
    GearList armour;

    GearList& For(RepairCategory category)
    {
        return category == RepairArmour ? armour : weapons;
    }
};

struct RepairChoice
{
    hand item;
    explicit RepairChoice(const Gear& gear) : item(gear.handle) {}
};

struct RepairPanel
{
    ContextMenuGUI* menu;
    hand owner;
    bool repairOnly;
    MyGUI::IntCoord nativeCoord;
    MyGUI::IntCoord expandedCoord;

    RepairPanel(ContextMenuGUI& gui, Character& character, bool only)
        : menu(&gui), owner(character.handle), repairOnly(only),
          nativeCoord(gui.getMainWidget()->getCoord()), expandedCoord(nativeCoord) {}
};

bool IsMenuOwner(Character* owner)
{
    return owner && owner->isPlayerCharacter() && !owner->isDead() && !owner->isUnconcious();
}

void AddChoice(Item* item, std::set<hand>& seen, RepairChoices& out)
{
    Gear* gear = item ? item->isGear() : 0;
    if (!gear || !IsSupported(gear) || !IsDamaged(gear) ||
        gear->handle.isNull() || !seen.insert(gear->handle).second)
        return;

    out.For(gear->isArmour() ? RepairArmour : RepairWeapon).push_back(gear);
}

void AddInventory(Inventory* inventory, std::set<hand>& seen, RepairChoices& out)
{
    if (!inventory)
        return;

    const lektor<Item*>& items = inventory->getAllItems();
    for (uint32_t i = 0; i < items.size(); ++i)
        AddChoice(items[i], seen, out);
}

void CollectChoices(Character& owner, RepairChoices& out)
{
    Inventory* main = owner.getInventory();
    if (!main)
        return;

    std::set<hand> seen;
    const lektor<InventorySection*>& sections = main->getAllSections();
    for (uint32_t i = 0; i < sections.size(); ++i)
    {
        InventorySection* section = sections[i];
        if (section && section->isAnEquippedItemSection)
        {
            const Ogre::vector<InventorySection::SectionItem>::type& items = section->getItems();
            for (size_t j = 0; j < items.size(); ++j)
                AddChoice(items[j].item, seen, out);
        }
    }
    AddInventory(main, seen, out);

    ContainerItem* backpack = owner.hasABackpackOn();
    Inventory* bag = backpack ? backpack->getInventory() : 0;
    if (bag != main)
        AddInventory(bag, seen, out);
}

MyGUI::Widget* FindRepairPanel(MyGUI::Widget* root)
{
    if (root)
    {
        for (size_t i = 0; i < root->getChildCount(); ++i)
        {
            MyGUI::Widget* child = root->getChildAt(i);
            if (child->isUserString(RepairPanelTag))
                return child;
        }
    }
    return 0;
}

void RemoveRepairPanel(ContextMenuGUI& menu)
{
    MyGUI::Widget* root = menu.getMainWidget();
    MyGUI::Widget* panel = FindRepairPanel(root);
    if (!panel)
        return;

    const RepairPanel* state = panel->getUserData<RepairPanel>(false);
    if (state)
    {
        if (root->getCoord() == state->expandedCoord)
            root->setCoord(state->nativeCoord);
        if (state->repairOnly)
            menu.setVisible(false);
    }
    MyGUI::Gui::getInstance().destroyWidget(panel);
}

MyGUI::IntCoord FitScreen(MyGUI::IntCoord coord)
{
    const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
    coord.top = (std::max)(0, (std::min)(coord.top, view.height - coord.height));
    coord.left = (std::max)(0, (std::min)(coord.left, view.width - coord.width));
    return coord;
}

bool IsMenuClick(MyGUI::MouseButton button)
{
    return button == MyGUI::MouseButton::Left || button == MyGUI::MouseButton::Right;
}

MyGUI::Widget* GetPicker()
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    return gui ? gui->findWidget<MyGUI::Widget>(RepairPickerName, false) : 0;
}

void DestroyPicker(MyGUI::Widget* picker)
{
    pickerOpen = false;
    if (picker)
        MyGUI::Gui::getInstance().destroyWidget(picker);
}

void SelectRepairItem(MyGUI::ListBox* sender, size_t index)
{
    if (!sender || !sender->getInheritedVisible() || index >= sender->getItemCount())
        return;

    MyGUI::Widget* picker = sender->getParent();
    const hand* ownerHandle = picker->getUserData<hand>(false);
    const RepairChoice* stored = sender->getItemDataAt<RepairChoice>(index, false);
    if (!ownerHandle || !stored)
        return;

    const hand choice = stored->item;
    Character* owner = ownerHandle->getCharacter();
    if (!IsMenuOwner(owner))
        return;

    try
    {
        Item* item = choice.getItem();
        Gear* gear = item ? item->isGear() : 0;
        // Hide before calling gameplay code. The next context update destroys
        // the popup after MyGUI has finished dispatching this release.
        picker->setVisible(false);
        if (gear)
            TryRepair(owner, gear);
    }
    catch (...)
    {
        ErrorLog("Extra Durability (HARD): repair item selection failed");
    }
}

void NotifyRepairItem(MyGUI::ListBox* sender, const MyGUI::IBNotifyItemData& info)
{
    if (info.notify == MyGUI::IBNotifyItemData::MouseReleased && info.id == MyGUI::MouseButton::Right)
        SelectRepairItem(sender, info.index);
}

void ShowItemPicker(ContextMenuGUI& menu, const hand& owner,
                    RepairCategory category, const GearList& choices)
{
    const MyGUI::IntCoord origin = menu.getMainWidget()->getAbsoluteCoord();
    const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
    const int rows = static_cast<int>((std::min)(choices.size(), size_t(6)));
    const int width = (std::min)((std::max)(origin.width, 300), view.width);
    const int height = (std::min)(54 + rows * 18, view.height);
    if (width < 80 || height < 70)
        return;

    MyGUI::Gui& gui = MyGUI::Gui::getInstance();
    if (pickerOpen)
        DestroyPicker(GetPicker());
    MyGUI::Widget* picker = gui.createWidget<MyGUI::Widget>(
        "Kenshi_FloatingPanelSkin", FitScreen(MyGUI::IntCoord(origin.left, origin.top, width, height)),
        MyGUI::Align::Default, "Popup", RepairPickerName);
    try
    {
        picker->setUserData(MyGUI::Any(owner));
        MyGUI::TextBox* title = picker->createWidget<MyGUI::TextBox>(
            "Kenshi_GenericTextBoxFlat", MyGUI::IntCoord(4, 3, width - 8, 24), MyGUI::Align::Default);
        title->setCaption(MyGUI::UString(Tr(category == RepairArmour ? "Repair Armour" : "Repair Weapon")));
        title->setTextAlign(MyGUI::Align::Center);
        MyGUI::ListBox* list = picker->createWidget<MyGUI::ListBox>(
            "Kenshi_ListBoxEmpty", MyGUI::IntCoord(4, 22, width - 8, height - 26), MyGUI::Align::Default);
        for (size_t i = 0; i < choices.size(); ++i)
        {
            Gear* gear = choices[i];
            list->addItem(MyGUI::UString("[" + ConditionLabel(gear) + "] " + gear->getName()),
                          MyGUI::Any(RepairChoice(*gear)));
        }
        list->setActivateOnClick(true);
        list->eventListMouseItemActivate += MyGUI::newDelegate(&SelectRepairItem);
        list->eventNotifyItem += MyGUI::newDelegate(&NotifyRepairItem);
        pickerOpen = true;
        menu.setVisible(false);
    }
    catch (...)
    {
        DestroyPicker(picker);
        throw;
    }
}

void SelectRepairCategory(MyGUI::Widget* sender, int, int, MyGUI::MouseButton button)
{
    if (!sender->getInheritedVisible() || !IsMenuClick(button))
        return;

    MyGUI::Widget* panel = sender->getParent()->getParent();
    RepairPanel* state = panel->getUserData<RepairPanel>(false);
    const RepairCategory* category = sender->getUserData<RepairCategory>(false);
    if (!state || !category)
        return;

    Character* owner = state->owner.getCharacter();
    if (!IsMenuOwner(owner))
        return;

    try
    {
        RepairChoices allChoices;
        CollectChoices(*owner, allChoices);
        const GearList& choices = allChoices.For(*category);
        if (choices.size() == 1)
        {
            Gear* item = choices.front();
            state->menu->setVisible(false);
            TryRepair(owner, item);
        }
        else if (!choices.empty())
        {
            ShowItemPicker(*state->menu, state->owner, *category, choices);
        }
    }
    catch (...)
    {
        ErrorLog("Extra Durability (HARD): repair category selection failed");
    }
}

void AddRepairButton(MyGUI::Widget& panel, const MyGUI::IntCoord& slotCoord,
                     const MyGUI::IntCoord& buttonCoord, RepairCategory category,
                     const std::string& caption, bool enabled)
{
    MyGUI::Widget* slot = panel.createWidget<MyGUI::Widget>(
        "PanelEmpty", slotCoord, MyGUI::Align::Default);
    MyGUI::Button* button = slot->createWidget<MyGUI::Button>(
        "Kenshi_Button1", buttonCoord, MyGUI::Align::Default);
    button->setCaption(MyGUI::UString(caption));
    button->setUserData(MyGUI::Any(category));
    button->setEnabled(enabled);
    button->eventMouseButtonReleased += MyGUI::newDelegate(&SelectRepairCategory);
}

bool AddRepairPanel(ContextMenuGUI& menu, Character& owner, const RepairChoices& choices, bool repairOnly)
{
    MyGUI::Widget* root = menu.getMainWidget();
    MyGUI::Widget* options = menu.optionsList;
    if (!root || !options)
        return false;
    const MyGUI::IntPoint origin = root->getAbsolutePosition();
    const MyGUI::IntPoint optionOrigin = options->getAbsolutePosition();
    int y = optionOrigin.top - origin.top;
    for (size_t i = 0; i < options->getChildCount(); ++i)
    {
        MyGUI::Widget* child = options->getChildAt(i);
        if (child->getVisible())
            y = (std::max)(y, child->getAbsoluteRect().bottom - origin.top);
    }

    MyGUI::IntCoord slotCoord = menu.optionCoords;
    if (slotCoord.height < 5 || slotCoord.height > 80)
        slotCoord.height = 26;
    slotCoord.top = 0;
    if (slotCoord.width <= 0)
        slotCoord.width = options->getWidth();
    MyGUI::IntCoord buttonCoord = menu.buttonCoords;
    if (buttonCoord.width <= 0 || buttonCoord.height <= 0)
        buttonCoord = MyGUI::IntCoord(4, 0, slotCoord.width - 8, slotCoord.height);

    // The parent owns these rows. Vanilla can iterate optionsList without
    // finding any additional command IDs or dispatching our callbacks.
    MyGUI::Widget* panel = root->createWidget<MyGUI::Widget>(
        "PanelEmpty", MyGUI::IntCoord(optionOrigin.left - origin.left, y, options->getWidth(),
                                      2 * slotCoord.height), MyGUI::Align::Default);
    try
    {
        panel->setUserString(RepairPanelTag, "1");
        panel->setUserData(MyGUI::Any(RepairPanel(menu, owner, repairOnly)));
        AddRepairButton(*panel, slotCoord, buttonCoord, RepairWeapon,
                        Tr("Repair Weapon"), !choices.weapons.empty());
        slotCoord.top += slotCoord.height;
        AddRepairButton(*panel, slotCoord, buttonCoord, RepairArmour,
                        Tr("Repair Armour"), !choices.armour.empty());
        RepairPanel& state = *panel->getUserData<RepairPanel>();
        MyGUI::IntCoord coord = state.nativeCoord;
        coord.height = (std::max)(coord.height, panel->getTop() + panel->getHeight() + 4);
        state.expandedCoord = FitScreen(coord);
        root->setCoord(state.expandedCoord);
        return true;
    }
    catch (...)
    {
        MyGUI::Gui::getInstance().destroyWidget(panel);
        throw;
    }
}

ContextMenuGUI* FindMatchingMenu(ContextMenu& context, RootObject* target)
{
    if (context.menuGUI && context.menuGUI->contextMenuTarget == target)
        return context.menuGUI;
    if (context.menuGUI2 && context.menuGUI2->contextMenuTarget == target)
        return context.menuGUI2;
    return 0;
}

void PrepareRepairOnlyMenu(ContextMenuGUI& menu, const std::string& name)
{
    menu.name = name;
    menu.nameText->setCaption(MyGUI::UString(name));
    MyGUI::Widget* root = menu.getMainWidget();
    const MyGUI::IntPoint mouse = MyGUI::InputManager::getInstance().getMousePosition();
    root->setPosition(mouse);

    // Empty native orders return before rendering. A reused GUI can still
    // contain rows from the last show. Native show recreates those on demand.
    MyGUI::Widget* options = menu.optionsList;
    for (size_t i = 0; i < options->getChildCount(); ++i)
        options->getChildAt(i)->setVisible(false);
}

void ShowRepairMenu(ContextMenu* self, bool open, RootObject* target)
{
    if (open && self && target)
    {
        try
        {
            if (pickerOpen)
                DestroyPicker(GetPicker());
            if (self->menuGUI)
                RemoveRepairPanel(*self->menuGUI);
            if (self->menuGUI2 && self->menuGUI2 != self->menuGUI)
                RemoveRepairPanel(*self->menuGUI2);
        }
        catch (...)
        {
            ErrorLog("Extra Durability (HARD): could not reset the repair menu");
        }
    }
    originalShowContextMenu(self, open, target);
    if (!open || !self || !target || self->delayedDestroyFlag)
        return;

    try
    {
        ContextMenuGUI* menu = FindMatchingMenu(*self, target);
        if (!menu)
            return;
        Character* owner = menu->contextMenuTarget.getCharacter();
        if (!IsMenuOwner(owner) || !HasRepairKit(owner))
            return;

        const bool repairOnly = !menu->getVisible();
        if (repairOnly)
        {
            if (!menu->getMainWidget() || !menu->nameText || !menu->optionsList)
                return;
            PrepareRepairOnlyMenu(*menu, self->contextMenuName);
        }
        RepairChoices choices;
        CollectChoices(*owner, choices);
        if (AddRepairPanel(*menu, *owner, choices, repairOnly) && repairOnly)
            menu->setVisible(true);
    }
    catch (...)
    {
        ErrorLog("Extra Durability (HARD): could not create repair menu");
    }
}

void UpdateRepairMenu(ContextMenu* self)
{
    originalUpdateContextMenu(self);
    if (!pickerOpen)
        return;

    try
    {
        MyGUI::Widget* picker = GetPicker();
        if (!picker || !picker->getVisible())
        {
            DestroyPicker(picker);
            return;
        }

        const hand* ownerHandle = picker->getUserData<hand>(false);
        const bool escape = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        const bool clicking = ((GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON)) & 0x8000) != 0;
        const MyGUI::IntPoint mouse = MyGUI::InputManager::getInstance().getMousePosition();
        if (!ou || ou->isLoadingFromASaveGame() || !ownerHandle ||
            !IsMenuOwner(ownerHandle->getCharacter()) || escape ||
            (clicking && !picker->getAbsoluteRect().inside(mouse)))
            DestroyPicker(picker);
    }
    catch (...)
    {
        ErrorLog("Extra Durability (HARD): could not update repair picker");
        pickerOpen = false;
    }
}

}  // namespace

void InstallRepairMenu()
{
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&ContextMenu::update),
                           &UpdateRepairMenu, &originalUpdateContextMenu) != KenshiLib::SUCCESS ||
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&ContextMenu::showContextMenu),
                           &ShowRepairMenu, &originalShowContextMenu) != KenshiLib::SUCCESS)
        ErrorLog("Extra Durability (HARD): could not hook character context menu");
}
}  // namespace durability
