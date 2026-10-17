#pragma once
#include <cstdint>

// An Entity is simply a unique integer ID used as a lightweight handle.
using Entity = uint32_t;

// A sentinel value representing an invalid or null entity.
constexpr Entity NULL_ENTITY = 0xFFFFFFFF;