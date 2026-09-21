#pragma once
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include "Actor.h"

namespace Game
{
// Persistent named actors are synchronized from gameplay state; untouched actors are retired.
// Render callbacks must not mutate the graph during traversal.
class SceneGraph
{
  public:
    void BeginSync()
    {
        ++m_Sync;
    }

    Actor* Find(const std::string& name) const
    {
        auto found = m_Actors.find(name);
        return found == m_Actors.end() ? nullptr : found->second.get();
    }

    Actor& Place(const std::string& name, Transform local, RenderLayer layer,
                 Actor::DrawFunction draw = {}, Actor* parent = nullptr)
    {
        Actor* actor = Find(name);
        if (!actor)
        {
            auto owned = std::make_unique<Actor>(name);
            actor = owned.get();
            m_Actors.emplace(name, std::move(owned));
            m_Order.push_back(actor);
        }
        SetParent(*actor, parent);
        actor->local = local;
        actor->layer = layer;
        actor->draw = std::move(draw);
        actor->m_LastSync = m_Sync;
        return *actor;
    }

    void SetParent(Actor& actor, Actor* parent)
    {
        if (Find(actor.Name()) != &actor || (parent && Find(parent->Name()) != parent))
        {
            throw std::invalid_argument("Actors must belong to the same scene");
        }
        for (Actor* ancestor = parent; ancestor; ancestor = ancestor->Parent())
        {
            if (ancestor == &actor)
            {
                throw std::invalid_argument("Scene graph cycle");
            }
        }
        if (actor.m_Parent == parent)
        {
            return;
        }
        Detach(actor);
        actor.m_Parent = parent;
        if (parent)
        {
            parent->m_Children.push_back(&actor);
        }
    }

    void Remove(const std::string& name)
    {
        Actor* actor = Find(name);
        if (!actor)
        {
            return;
        }
        while (!actor->m_Children.empty())
        {
            Remove(actor->m_Children.back()->Name());
        }
        // Copy before erasing: name may refer to the actor's own string.
        std::string key = actor->Name();
        Detach(*actor);
        m_Order.erase(std::remove(m_Order.begin(), m_Order.end(), actor), m_Order.end());
        m_Actors.erase(key);
    }

    void EndSync()
    {
        std::vector<std::string> retired;
        for (const Actor* actor : m_Order)
        {
            if (actor->m_LastSync != m_Sync)
            {
                retired.push_back(actor->Name());
            }
        }
        for (const auto& name : retired)
        {
            Remove(name);
        }
    }

    void Clear()
    {
        m_Order.clear();
        m_Actors.clear();
    }

    void Render(RenderLayer first, RenderLayer last) const
    {
        struct Entry
        {
            const Actor* actor;
            Transform world;
        };

        std::vector<Entry> visible;
        visible.reserve(m_Order.size());
        for (const Actor* actor : m_Order)
        {
            if (actor->IsVisible() && actor->draw && actor->layer >= first && actor->layer <= last)
            {
                visible.push_back({actor, actor->WorldTransform()});
            }
        }
        std::stable_sort(visible.begin(), visible.end(),
                         [](const Entry& a, const Entry& b)
                         {
                             if (a.actor->layer != b.actor->layer)
                             {
                                 return a.actor->layer < b.actor->layer;
                             }
                             // UI uses insertion order; world depth ignores height.
                             return a.actor->layer != RenderLayer::UserInterface &&
                                    a.world.x + a.world.y < b.world.x + b.world.y;
                         });
        for (const Entry& entry : visible)
        {
            entry.actor->draw(*entry.actor, entry.world);
        }
    }

  private:
    static void Detach(Actor& actor)
    {
        if (actor.m_Parent)
        {
            auto& siblings = actor.m_Parent->m_Children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), &actor), siblings.end());
            actor.m_Parent = nullptr;
        }
    }

    std::unordered_map<std::string, std::unique_ptr<Actor>> m_Actors;
    std::vector<Actor*> m_Order;
    unsigned long long m_Sync = 0;
};
} // namespace Game
