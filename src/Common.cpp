#include "Common.h"

#include <algorithm>
#include <iostream>

constexpr float SPEED = 256;
constexpr float MAX_SPEED = 1000;

void process_input_for_player(EntityTransform& transform, const Input& input) noexcept
{
    auto keys = input.keys;
    sf::Vector2f change{};
    if ((keys & InputKeyPress::W) == InputKeyPress::W)
    {
        change += {0, -SPEED};
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
    //auto& velocity = transform.velocity;
    //velocity += change * input.dt;
    //velocity.x = std::clamp(velocity.x, -MAX_SPEED, MAX_SPEED) * 0.94f;
    //velocity.y = std::clamp(velocity.y, -MAX_SPEED, MAX_SPEED) * 0.94f;
    //transform.position += velocity;

    transform.position += change * input.dt;
}