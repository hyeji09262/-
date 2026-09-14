#pragma once

class Renderer;

namespace LevelOne
{
void Initialize(Renderer* renderer);
void Resize(int width, int height);
void Key(unsigned char key, bool down);
void Update(float deltaTime);
void Draw();
void Pause();
void Save();
} // namespace LevelOne
