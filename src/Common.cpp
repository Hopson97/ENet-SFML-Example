#include "Common.h"

#include <algorithm>
#include <limits>
#include <print>

#include "Util/Util.h"

constexpr float SPEED = 25;
constexpr float MAX_SPEED = TILE_SIZE;

void process_input_for_player(EntityTransform& transform, const Input& input) noexcept
{
    auto keys = input.keys;
    sf::Vector2f change{};
    if ((keys & InputKeyPress::W) == InputKeyPress::W && transform.is_grounded)
    {
        change += {0, -SPEED};
        change += {0, -SPEED * 25};
    }
    if ((keys & InputKeyPress::A) == InputKeyPress::A)
    {
        change += {-SPEED, 0};
    }
    if ((keys & InputKeyPress::S) == InputKeyPress::S)
    {
        change += {0, SPEED};
    }
    if ((keys & InputKeyPress::D) == InputKeyPress::D)
    {
        change += {SPEED, 0};
    }
    auto& velocity = transform.velocity;
    velocity += change * input.dt;
    velocity.x = std::clamp(velocity.x, -MAX_SPEED, MAX_SPEED) * 0.94f;
    velocity.y = std::clamp(velocity.y, -MAX_SPEED, MAX_SPEED) * 0.98f;

    apply_map_collisions(transform);
}

void apply_map_collisions(EntityTransform& transform)
{
    auto& velocity = transform.velocity;
    auto& position = transform.position;
    auto i_pos = sf::Vector2i{position};
    auto& size = transform.size;

    auto next_position = position + velocity;
    //auto next_tile_position = sf::Vector2i{next_position / TILE_SIZE};

    auto tile_position = sf::Vector2i{position / TILE_SIZE};

    transform.is_grounded = false;
    if (velocity.x > 0)
    {
        for (int y = i_pos.y; y <= i_pos.y + size.y; y += I_TILE_SIZE / 2)
        {
            auto x_tile = static_cast<int>((position.x + size.x + velocity.x) / TILE_SIZE);
            auto y_tile = static_cast<int>(y / TILE_SIZE);
            if (get_tile(x_tile, y_tile))
            {
                velocity.x = 0;
                next_position.x = x_tile * TILE_SIZE - size.x;
            }
        }
    }
    else if (velocity.x < 0)
    {
        for (int y = i_pos.y; y <= i_pos.y + size.y; y += I_TILE_SIZE / 2)
        {
            auto x_tile = static_cast<int>((position.x + velocity.x) / TILE_SIZE);
            auto y_tile = static_cast<int>(y / TILE_SIZE);
            if (get_tile(x_tile, y_tile))
            {
                velocity.x = 0;
                next_position.x = x_tile * TILE_SIZE + TILE_SIZE;
            }
        }
    }

    if (velocity.y > 0)
    {

        for (int x = i_pos.x; x <= i_pos.x + size.x; x += I_TILE_SIZE / 2)
        {
            auto x_tile = static_cast<int>(x / TILE_SIZE);
            int y_tile = (position.y + size.y + velocity.y) / TILE_SIZE;
            if (get_tile(x_tile, y_tile))
            {
                velocity.y = 0;
                next_position.y = y_tile * TILE_SIZE - size.y;
                transform.is_grounded = true;
            }
        }
    }
    else if (velocity.y < 0)
    {

        for (int x = i_pos.x; x <= i_pos.x + size.x; x += static_cast<int>(TILE_SIZE) / 2)
        {
            auto x_tile = static_cast<int>(x / TILE_SIZE);
            int y_tile = (position.y + velocity.y - 1) / TILE_SIZE;
            if (get_tile(x_tile, y_tile))
            {
                velocity.y = 0;
                next_position.y = y_tile * TILE_SIZE + TILE_SIZE;
            }
        }

    }

    if (!transform.is_grounded)
    {
        transform.velocity.y += 0.35f;
    }

    position = next_position;
}

int get_tile(int x, int y)
{
    if (x < 0 || x >= MAP_SIZE || y < 0 || y >= MAP_SIZE)
    {
        return 0;
    }
    return MAP[y * MAP_SIZE + x];
}
