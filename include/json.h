#ifndef JSON_H
#define JSON_H

#include "common.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void saveToJsonFile(const LocationData& data,int counter);
CellTowerData parseCellTower(const json& cellJson);

#endif
