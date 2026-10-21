#pragma once
// Метки WantedMap для других плагинов (сейчас - для BestMinimap).
//
// WantedMap экспортирует WantedMap_GetMarkers: копию своих текущих меток
// (тех, что он рисует на большой карте). Список обновляется, пока открыта
// большая карта, - так WantedMap устроен с самого начала, чтобы не искать
// цели каждый кадр. Значит, метки на миникарте - «на момент последнего
// взгляда на карту»: открыл карту - метки освежились.
//
// Подключение у читателя:
//
//   #include <WantedMapApi.h>
//   HMODULE m = GetModuleHandleA("WantedMap.dll");
//   WantedMap_GetMarkersFn get = m ? reinterpret_cast<WantedMap_GetMarkersFn>(
//       GetProcAddress(m, "WantedMap_GetMarkers")) : NULL;
//   WantedMap_Marker buf[64];
//   const int n = get ? get(buf, 64) : 0;
//
// Совместимость: поля только дописываются в конец, size растёт. Читатель
// передаёт свой размер структуры в каждом элементе не обязан: WantedMap
// заполняет ровно sizeof(WantedMap_Marker) своей версии, поэтому
// структуру менять только дописыванием и с новой версией API.

#define WANTEDMAP_API_VERSION 1

struct WantedMap_Marker
{
    float x, y, z;          // мировые координаты
    float r, g, b;          // цвет метки, как на большой карте
    int accuracy;           // 0 точно, 1 отряд далеко, 2 поселение, 3 убит
    char label[96];         // подпись (UTF-8), обрезана
};

// Возвращает число записанных меток (не больше max).
typedef int (__cdecl *WantedMap_GetMarkersFn)(WantedMap_Marker* out, int max);
