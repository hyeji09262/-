#pragma once
#include "WorldMonitor.h"
#include "RpgProgress.h"

class Renderer;

namespace Prologue
{
void Initialize(Renderer* renderer);
void Resize(int width, int height);
void Key(unsigned char key, bool down);
void CameraKey(int key, bool down);
void Pause();
void Update(float deltaTime);
void Draw();
bool QuestTwoStart(Rpg::Progress& progress, int& fragments);
bool ConsumeQuestTwoRequest();
WorldMonitor::Scene MonitorSnapshot();
} // namespace Prologue
