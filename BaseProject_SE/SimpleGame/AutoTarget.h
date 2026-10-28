#pragma once
#include <cmath>

namespace AutoTarget
{
// Measure the 60-degree cone with the quarter-view axis ratio, retaining world-unit range.
// The cone stays anchored to the initial facing; no target keeps that direction.
struct Selection
{
    float originX, originY;
    float x, y;
    float directionX, directionY;
    float facingX, facingY;
    float distance;
    bool found = false;

    Selection(float px, float py, float dx, float dy, float range)
        : originX(px), originY(py), x(px), y(py), distance(range)
    {
        float length = std::hypot(dx, dy);
        if (length < .001f)
        {
            dx = .70710678f;
            dy = -.70710678f;
            length = 1;
        }
        directionX = dx / length;
        directionY = dy / length;
        facingX = directionX;
        facingY = directionY;
    }

    bool Consider(float px, float py, bool alive, bool visible = true)
    {
        const float length = std::hypot(px - originX, py - originY);
        if (!alive || !visible || length < .001f || length >= distance)
        {
            return false;
        }
        constexpr float cosHalfAngle = .86602540f; // cos(30 degrees).
        const float aimX = facingX - facingY;
        const float aimY = (facingX + facingY) * .5f;
        const float targetX = (px - originX) - (py - originY);
        const float targetY = ((px - originX) + (py - originY)) * .5f;
        const float alignment = (targetX * aimX + targetY * aimY) /
                                (std::hypot(aimX, aimY) * std::hypot(targetX, targetY));
        if (alignment < cosHalfAngle)
        {
            return false;
        }
        x = px;
        y = py;
        distance = length;
        directionX = (x - originX) / length;
        directionY = (y - originY) / length;
        found = true;
        return true;
    }
};
} // namespace AutoTarget
