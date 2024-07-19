#include "Common.h"

#include <algorithm>
#include <iostream>

#include "Util/Util.h"

constexpr float SPEED = 25;
constexpr float MAX_SPEED = 256;

void process_input_for_player(EntityTransform& transform, const Input& input) noexcept
{
    auto keys = input.keys;
    sf::Vector2f change{};
    if ((keys & InputKeyPress::W) == InputKeyPress::W && transform.is_grounded)
    {
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

    // Do basic collision detection + response!
    apply_map_collisions(transform);

    // transform.position += velocity;

    // transform.position += change * input.dt;
}

void apply_map_collisions(EntityTransform& transform)
{
    auto& velocity = transform.velocity;
    auto& position = transform.position;

    auto next_position = position + velocity;
    auto next_tile_position = sf::Vector2i{next_position / TILE_SIZE};

    auto tile_position = sf::Vector2i{position / TILE_SIZE};

    transform.is_grounded = false;

    for (int y = tile_position.y - 1; y <= tile_position.y + 1; y++)
    {
        for (int x = tile_position.x - 1; x <= tile_position.x + 1; x++)
        {

        }
    }

    
    
                // auto tile_position = sf::Vector2i{sf::Vector2f{(float)x, (float)y} / TILE_SIZE};
            if (velocity.x > 0)
            {
                if (get_tile(tile_position.x + 1, tile_position.y) )
                   // get_tile(tile_position.x + 1, tile_position.y + 1))
                {
                    velocity.x = 0;
                    next_position.x = tile_position.x * TILE_SIZE;
                }
            }
            else if (velocity.x < 0)
            {
                if (get_tile(tile_position.x, tile_position.y))
                {
                    velocity.x = 0;
                    next_position.x = tile_position.x * TILE_SIZE + TILE_SIZE;
                }
            }

            if (velocity.y > 0)
            {
                if (get_tile(tile_position.x, tile_position.y + 1))
                {
                    velocity.y = 0;
                    next_position.y = tile_position.y * TILE_SIZE;
                    transform.is_grounded = true;
                }
            }
            else if (velocity.y < 0)
            {
                if (get_tile(tile_position.x, tile_position.y))
                {
                    velocity.y = 0;
                    next_position.y = tile_position.y * TILE_SIZE + TILE_SIZE;
                }
            }
    

    if (!transform.is_grounded)
    {
        transform.velocity.y += 0.35;
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
