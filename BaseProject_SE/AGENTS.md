# Game Development Guide

## Project identity

- This repository is for the Game Software Engineering course.
- Build a single-player, real-time-rendered 2.5D quarter-view open-world RPG.
- The player is a student who suddenly becomes a magical girl and grows through regional boss battles.
- The setting is a contemporary fantasy school and coastal town threatened by nightmares. The story expands from saving the school to saving the town and ultimately Earth. Use pink/lilac magical-girl costumes, star motifs, modern buildings and cherry blossoms, with danger balanced by school-life humor.

## Core player loop

1. Explore a world region and discover blocked routes, secrets, NPCs, and a regional boss.
2. Defeat the boss to gain a magic core and skill points.
3. Spend skill points in a magic branch. The core's base traversal ability is always unlocked so the main story cannot be blocked.
4. Use new traversal magic to access new regions, optional rewards, lore, equipment, and upgrades.
5. Become stronger, uncover the origin of the nightmares threatening Earth, and progress toward the final boss.

## Game and world rules

- Use world coordinates `(x, y, z)` for all gameplay. `z` represents 2.5D height.
- Convert world positions to quarter-view screen positions in the renderer. Do not make game logic depend directly on screen pixels.
- Render only the camera-visible area. Divide the open world into chunks (the initial target is 32 by 32 tiles per chunk).
- Depth-sort world sprites using their projected ground position so characters correctly appear in front of or behind scenery.
- Keep UI in screen space and world entities in world space.
- Treat the game as single-player only. Do not add networking, online services, or multiplayer assumptions unless the user explicitly changes scope.

## Progression principles

- Every boss reward must matter in combat, exploration, or both.
- Prefer magic that changes traversal over simple damage increases.
- Example branches: shadow (dash and hidden paths), lightning (activate devices), gravity (move obstacles), and time (restore paths or briefly stop time).
- Optional areas should reward equipment, spells, lore, NPC progress, or meaningful upgrades rather than filler collectibles.
- Preserve a clear vertical slice before expanding the world: movement, collision, one small region, one NPC interaction, one enemy, one boss, one traversal unlock, and one newly accessible reward.

## Technical direction

- The current stack is C++ with OpenGL, FreeGLUT, and GLEW in a Visual Studio solution.
- Extend the existing renderer incrementally; do not replace the engine, graphics library, or project structure without explicit approval.
- Keep rendering, input, game state, world/chunk data, and entity behavior separated as the codebase grows.
- Route placed world objects and HUD composites through Actor and SceneGraph; keep child effects and shadows attached to their owner, and let the graph handle render layers and ground-depth sorting. Do not add a second ad-hoc object sorting list.
- Define character attachment points in CharacterVisual.h; wand meshes and spell origins must share the same socket coordinates, including mirrored cast poses.
- Use delta-time-based updates for real-time gameplay.
- Keep static terrain/scenery actors retained until the map changes, and cull before sorting the scene render queue. Preserve transparent painter order when batching.
- Keep immutable model meshes in the renderer GPU cache. Batch compatible geometry, text, and shader effects; do not flush per text label or water tile.
- Instrument new potentially expensive systems with Performance::Scope, Count, or Gauge using stable dotted English names and explicit units. Compare Release builds using JSONL performance windows; distinguish CPU inclusive time, asynchronous GPU timeline time, presentation wait, and frame interval. Never claim a measured improvement without runtime evidence.
- Keep project assets and runtime dependencies intentional. Do not commit Visual Studio workspace files or generated build artifacts.

## Code formatting

- Follow the project's .clang-format for first-party C++ code (four spaces, Allman braces, approximately 100 columns).
- Put statements on separate lines; avoid compressed single-line functions and control flow.
- Separate function definitions and logical phases with blank lines so initialization, updates, rendering, and cleanup are easy to scan.
- Preserve existing names and behavior during formatting-only changes.
- Do not format vendored files under SimpleGame/Dependencies or generated build files.

## Working agreements

- The user normally performs builds and verifies runtime results. Do not build or run the game unless the user explicitly requests it or it is necessary to resolve a specific issue.
- Before implementing a feature, preserve the project identity and core loop above. Flag any request that would materially change them.
- Prefer small, focused changes that keep the project buildable in Visual Studio.
- When adding a system, state its relationship to the current game loop and list any manual validation the user should perform.
