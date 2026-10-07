#pragma once

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
} // namespace Prologue
