#pragma once

#include "RpgProgress.h"
#include "WorldMonitor.h"

class Renderer;
namespace Hunting
{
class PrimitiveModels;
}

namespace ParkQuest
{
void Open(Renderer& renderer, const Hunting::PrimitiveModels& models, const Rpg::Progress& progress);
void Resize(int width, int height);
void Key(unsigned char key, bool down);
void Pause();
void Update(float dt);
void Draw();
bool ConsumeExit();
bool Defeated();
void Restock(int healthPotions, int manaPotions);
bool RescuedDog();
const Rpg::Progress& Progress();
WorldMonitor::Scene MonitorSnapshot();
} // namespace ParkQuest
