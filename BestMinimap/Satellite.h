// Satellite - вид со спутника: снимки мира сверху плитками, их хранение и
// выдача потоку рисования миникарты. Подробно - в Satellite.cpp.
#pragma once

#include <string>
#include <vector>
#include <OgreVector3.h>

namespace Satellite
{
    // Плитка - квадрат мира kTileMetres x kTileMetres, от (tx*kTileMetres,
    // tz*kTileMetres); на снимке север (-Z мира) сверху, восток (+X) справа.
    const float kTileMetres = 1024.0f;

    // Вариант освещения плитки.
    enum Variant { DAY, NIGHT, VARIANT_COUNT };

    // ---- поток игры ----

    // Настройки: снимать ли сами, качество (пикселей на сторону плитки:
    // 1024, 2048, 4096, 8192), папка плиток.
    void Configure(bool capture, int quality, const std::string& folder);

    // Каждый кадр: по расписанию снимает недостающие плитки вокруг
    // cameraCentre (мировые координаты). hourOfDay - время суток игры (0-24).
    // allowed=false - не начинать новых снимков (начатый доснимается).
    void Tick(const Ogre::Vector3& cameraCentre, double hourOfDay, bool allowed);

    // По клавише: плитку под centre - сейчас, вариант - по времени суток.
    void CaptureNow(const Ogre::Vector3& centre, double hourOfDay);

    // Загрузка сейва или выход в меню: бросить начатый снимок.
    void Reset();

    // Плитки, снятые с прошлого вызова (их ключи): поток рисования их
    // перечитает. Звать, пока поток рисования свободен.
    void TakeFresh(std::vector<long long>& out);

    // Сколько плиток каждого варианта есть (для журнала).
    int StoredCount(Variant v);

    long long Key(int tx, int tz, Variant v);

    // ---- поток рисования ----

    // Уровень плитки для рисования: size x size пикселей ARGB.
    struct Level
    {
        int size;
        const unsigned* px;
    };

    // Уровень плитки не меньше wantSize (или самый подробный, если такого
    // нет). false - плитки этого варианта нет. Читает с диска при первой
    // нужде и держит в памяти, пока хватает бюджета.
    bool TileLevel(int tx, int tz, Variant v, int wantSize, Level& out);

    // Начало кадра: что отдано в этом кадре, держится в памяти до следующего.
    void BeginFrame();

    // Забыть перечитанные (снятые заново) плитки - перед рисованием кадра.
    void Forget(const std::vector<long long>& keys);
}
