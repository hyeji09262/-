#pragma once

#include <cmath>

template <typename Point> class CompanionFollow
{
  public:
    void Reset()
    {
        m_Ready = false;
    }

    Point Position(Point player, Point facing) const
    {
        return m_Ready ? m_Position : Beside(player, facing, 1.f);
    }

    template <typename Walkable>
    void Update(Point player, Point facing, float dt, Walkable walkable)
    {
        Point target = Beside(player, facing, 1.f);
        if (!walkable(target))
        {
            target = Beside(player, facing, -1.f);
        }
        if (!walkable(target))
        {
            target = player;
        }
        if (!m_Ready || std::hypot(m_Position.x - player.x, m_Position.y - player.y) > 3.5f)
        {
            m_Position = target;
            m_Ready = true;
            return;
        }

        const float blend = 1.f - std::exp(-8.f * dt);
        const Point next{m_Position.x + (target.x - m_Position.x) * blend,
                         m_Position.y + (target.y - m_Position.y) * blend};
        if (walkable(next))
        {
            m_Position = next;
        }
    }

  private:
    static Point Beside(Point player, Point facing, float side)
    {
        return {player.x - 1.15f * facing.x - side * .8f * facing.y,
                player.y - 1.15f * facing.y + side * .8f * facing.x};
    }

    Point m_Position{};
    bool m_Ready = false;
};
