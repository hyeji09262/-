#pragma once
#include "WorldMonitor.h"
#include "RpgProgress.h"

class Renderer;

namespace LevelOne
{
void Initialize(Renderer* renderer);
void Resize(int width, int height);
void Key(unsigned char key, bool down);
void Update(float deltaTime);
void Draw();
void Pause();
void Resume(bool fromVillage);
bool ConsumeWorldRequest();
bool Defeated();
const Rpg::Progress& Progress();
void Restock(const Rpg::Progress& supplies);
WorldMonitor::Scene MonitorSnapshot();
} // namespace LevelOne
