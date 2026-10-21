#pragma once
#include "WorldMonitor.h"
#include "RpgProgress.h"
class Renderer;

namespace QuestTwo
{
void Initialize(Renderer* renderer);
void Resize(int width, int height);
void Open(bool unlocked, const Rpg::Progress& inherited, int fragments);
void Pause();
void Key(unsigned char key, bool down);
void Update(float dt);
void Draw();
WorldMonitor::Scene MonitorSnapshot();
} // namespace QuestTwo
