// Satellite - снимок мира сверху (проба «вида со спутника»).
#pragma once

#include <string>
#include <OgreVector3.h>

namespace Satellite
{
    // Сделать снимки квадрата sizeMetres вокруг centerWorld (мировые
    // координаты, как у персонажей) с нескольких высот и сохранить их в
    // folder картинками BMP. Работает несколько кадров: см. Tick.
    void RequestProbe(const Ogre::Vector3& centerWorld, float sizeMetres, const std::string& folder);

    // Каждый кадр игры, из потока игры (там же, где рисует MyGUI).
    void Tick();

    // Загрузка или выход в меню: бросить начатое.
    void Reset();
}
