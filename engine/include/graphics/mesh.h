#pragma once
#include "vk/vertex.h"
#include <vector>
#include <cstdint>

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};