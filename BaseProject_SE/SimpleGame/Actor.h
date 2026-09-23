#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Game
{
struct Transform
{
    float x = 0;
    float y = 0;
    float z = 0;
    float scale = 1;
};

enum class RenderLayer
{
    Background,
    Ground,
    Shadow,
    World,
    Overlay,
    UserInterface
};

// A scene object owns its local pose, visibility and rendering behavior, not GPU resources.
class Actor
{
  public:
    using DrawFunction = std::function<void(const Actor&, const Transform&)>;

    explicit Actor(std::string name) : m_Name(std::move(name))
    {
    }

    virtual ~Actor() = default;
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;

    const std::string& Name() const
    {
        return m_Name;
    }

    Actor* Parent() const
    {
        return m_Parent;
    }

    const std::vector<Actor*>& Children() const
    {
        return m_Children;
    }

    Transform WorldTransform() const
    {
        if (!m_Parent)
        {
            return local;
        }
        Transform parent = m_Parent->WorldTransform();
        return {parent.x + local.x * parent.scale, parent.y + local.y * parent.scale,
                parent.z + local.z * parent.scale, parent.scale * local.scale};
    }

    bool IsVisible() const
    {
        return enabled && visible && (!m_Parent || m_Parent->IsVisible());
    }

    Transform local;
    RenderLayer layer = RenderLayer::World;
    bool enabled = true;
    bool visible = true;
    DrawFunction draw;

  private:
    friend class SceneGraph;
    std::string m_Name;
    Actor* m_Parent = nullptr;
    std::vector<Actor*> m_Children;
    unsigned long long m_LastSync = 0;
    bool m_Persistent = false;
};
} // namespace Game
