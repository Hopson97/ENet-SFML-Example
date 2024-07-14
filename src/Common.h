#pragma once

#include <cstdint>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Time.hpp>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

enum InputKeyPress
{
    NONE = 0,
    W = 1,
    A = 1 << 1,
    S = 1 << 2,
    D = 1 << 3,
};

struct Input
{
    u32 sequence = 0;
    float dt = 0;
    u8 keys = InputKeyPress::NONE;
};

struct EntityTransform
{
    sf::Vector2f position;
    sf::Vector2f velocity;
};

struct EntityCommon
{
    EntityTransform transform;
    i16 id = -1;
    bool active;
};

void process_input_for_player(EntityTransform& transform, const Input& input) noexcept;