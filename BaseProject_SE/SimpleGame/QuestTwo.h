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
bool ConsumeWorldRequest();
bool Defeated();
const Rpg::Progress& Progress();
void SyncGrowth(const Rpg::Progress& source);
void Restock(int healthPotions, int manaPotions);
bool Finished();
bool NeedsPotionShop();
bool ShopProgress(Rpg::Progress& progress);
bool BuyPotion(bool magic, std::string& message);
void StoreWorldPotions(int healthPotions, int magicPotions);
void StoreParkProgress(const Rpg::Progress& progress, bool foundFragment);
bool SchoolObjective(int currentRoom, float playerX, float playerY, int& room, float& x, float& y,
                     std::string& label);
void EnterFromSchool(int room, float x, float y);
void Key(unsigned char key, bool down);
void Update(float dt);
void Draw();
WorldMonitor::Scene MonitorSnapshot();
} // namespace QuestTwo
