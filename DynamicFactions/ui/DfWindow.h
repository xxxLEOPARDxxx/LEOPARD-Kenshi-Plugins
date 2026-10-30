#pragma once
// Окно состояния фракций по хоткею (CTRL+F10): для наблюдения и тестов.
// Только читает состояние. Только поток интерфейса (ForgottenGUI::update).

#include <string>
#include <vector>

#include "core/DfBrain.h"
#include "core/DfState.h"

// Нажатие хоткея: открыть или закрыть.
void DfWindowToggle();

bool DfWindowIsOpen();

// Раз в секунду, пока окно открыто. snaps[i] - живой снимок фракции i;
// members < 0 - снимка нет (фракция игры не найдена). Текст ставится
// только при изменении.
// hires[i] - что с наймом у фракции i (пусто - не идёт).
void DfWindowUpdate(const std::vector<DfFactionState>& states, const std::vector<DfSnapshot>& snaps,
                    const std::vector<std::string>& hires, const std::vector<std::string>& work, const DfRules& rules);

// Каждый кадр: закрыть окно, если нажали крестик.
void DfWindowFrame();
