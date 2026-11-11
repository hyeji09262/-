#pragma once

namespace CompanionFollow
{
// The right-hand offset rotates in world space with the player's unit facing.
template <typename Point> Point RightOf(Point player, Point facing)
{
    constexpr float offset = 1.15f;
    return {player.x - offset * facing.y, player.y + offset * facing.x};
}

} // namespace CompanionFollow
