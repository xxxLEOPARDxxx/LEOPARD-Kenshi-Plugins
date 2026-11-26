#pragma once

class ProductionBuilding;

namespace finite_ore {
bool IsNaturalOre(ProductionBuilding* building);
bool IsManagedOre(ProductionBuilding* building);
bool BeginOreOutput(ProductionBuilding* building);
void EndOreOutput(ProductionBuilding* building, bool produced);
bool InstallOutputHook();
}
