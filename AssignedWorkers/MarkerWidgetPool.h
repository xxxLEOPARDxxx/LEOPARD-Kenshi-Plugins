#pragma once
#include <map>
#include <mygui/MyGUI_IUnlinkWidget.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_WidgetManager.h>

// One callback per module, O(log N) only when widgets are deleted.
// A fixed pool bounds both widget ownership and the reverse lookup.
template <int N> class MarkerWidgetPool : public MyGUI::IUnlinkWidget
{
    MyGUI::Widget* slots[N];
    std::map<MyGUI::Widget*, int> indices;
    bool registered;
public:
    MarkerWidgetPool() : registered(false) { for (int i=0; i<N; ++i) slots[i]=NULL; }
    MyGUI::Widget* Get(int i) const { return slots[i]; }
    void Set(int i, MyGUI::Widget* w)
    {
        if (!registered && w != NULL) {
            MyGUI::WidgetManager* manager=MyGUI::WidgetManager::getInstancePtr();
            if (manager == NULL) return;
            manager->registerUnlinker(this); registered=true;
        }
        if (slots[i] != NULL) indices.erase(slots[i]);
        slots[i]=w;
        if (w != NULL) indices[w]=i;
    }
    virtual void _unlinkWidget(MyGUI::Widget* w)
    {
        typename std::map<MyGUI::Widget*, int>::iterator it=indices.find(w);
        if (it == indices.end()) return;
        slots[it->second]=NULL;
        indices.erase(it);
    }
};
