#pragma once
// Окно состояния фракций по хоткею (CTRL+F10): для наблюдения и тестов.
// Только читает состояние. Только поток интерфейса (ForgottenGUI::update).

#include <vector>

#include "core/DfBrain.h"
#include "core/DfState.h"

// Нажатие хоткея: открыть или закрыть.
void DfWindowToggle();

bool DfWindowIsOpen();

// Раз в секунду, пока окно открыто. snaps[i] - живой снимок фракции i;
// members < 0 - снимка нет (фракция игры не найдена). Текст ставится
// только при изменении.
void DfWindowUpdate(const std::vector<DfFactionState>& states, const std::vector<DfSnapshot>& snaps,
                    const DfRules& rules);

// Каждый кадр: закрыть окно, если нажали крестик.
void DfWindowFrame();
