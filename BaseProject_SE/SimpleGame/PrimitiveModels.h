#pragma once
#include <array>
#include "Renderer.h"

namespace Hunting
{
enum class ModelId
{
    PlayerIdle,
    PlayerWalk0,
    PlayerWalk1,
    PlayerWalk2,
    PlayerWalk3,
    PlayerCast,
    Slime0,
    Slime1,
    Slime2,
    Slime3,
    Shade0,
    Shade1,
    Shade2,
    Shade3,
    Tree,
    Rock,
    Shelter,
    Campfire,
    Coin,
    Potion,
    Crystal,
    Shadow,
    Count
};

class PrimitiveModels
{
  public:
    bool loadedFromFile = false;
    bool storedToFile = false;
    void Initialize();
    const std::vector<Renderer::ModelVertex>& Get(ModelId id) const;

  private:
    using Mesh = std::vector<Renderer::ModelVertex>;
    std::array<Mesh, static_cast<int>(ModelId::Count)> m_Models;
    bool m_Initialized = false;
    bool Load();
    bool Save() const;
    void Generate();
};
} // namespace Hunting
