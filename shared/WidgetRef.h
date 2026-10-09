#pragma once
// Ссылка на свой виджет без поиска по имени.
//
// Зачем. gui->findWidgetT(имя) обходит всё дерево интерфейса игры, включая
// скрытые окна, - тысячи виджетов. Моды Emkej (Organize-*, Map-markers)
// спрашивали так «есть ли моя панель» по нескольку раз за кадр, и чаще
// всего ответ был «нет» - то есть каждый раз полный обход.
//
// Как. MyGUI сообщает всем зарегистрированным IUnlinkWidget о каждом
// удаляемом виджете, вместе с вложенными (так же InputManager забывает
// фокус удалённого). Запомнили указатель при создании - и он сам
// обнулится, когда виджет удалим мы или игра вместе со своим окном.
//
// Объект WidgetRef заводить статическим (глобальным). Из списка MyGUI он
// не выписывается: живёт до конца процесса, как и DLL плагина.

#include <mygui/MyGUI_IUnlinkWidget.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_WidgetManager.h>

class WidgetRef : public MyGUI::IUnlinkWidget
{
public:
    WidgetRef() : m_widget(0), m_registered(false) {}

    MyGUI::Widget* get() const { return m_widget; }

    void set(MyGUI::Widget* widget)
    {
        if (!m_registered && widget != 0)
        {
            MyGUI::WidgetManager* const manager = MyGUI::WidgetManager::getInstancePtr();
            if (manager == 0)
                return;                 // без менеджера не узнаем об удалении - не запоминаем
            manager->registerUnlinker(this);
            m_registered = true;
        }
        m_widget = widget;
    }

    void reset() { m_widget = 0; }

    virtual void _unlinkWidget(MyGUI::Widget* widget)
    {
        if (widget == m_widget)
            m_widget = 0;
    }

private:
    MyGUI::Widget* m_widget;
    bool m_registered;
};
