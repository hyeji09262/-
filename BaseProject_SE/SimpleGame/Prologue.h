#pragma once
#include "WorldMonitor.h"
#include "RpgProgress.h"
#include <string>

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
bool TransformationUnlocked();
bool ConsumeWorldRequest();
void ResumeFromWorld();
bool WorldObjective(float& x, float& y, int& person, std::string& objective);
std::string InteractWorld();
WorldMonitor::Scene MonitorSnapshot();
} // namespace Prologue
