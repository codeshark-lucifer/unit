#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <typeinfo>
#include "entity.h"
#include "pool.h"
#include "api.h"

ENGINE_API uint32_t GetOrRegisterComponentTypeId(const char *name);

// Helper class to generate a unique ID for each component type across all modules.
class ComponentIdCounter
{
public:
    template <typename T>
    static uint32_t GetId()
    {
        static uint32_t id = GetOrRegisterComponentTypeId(typeid(T).name());
        return id;
    }
};

// The central Registry managing entities and component pools.
class Registry
{
public:
    Registry() = default;
    ~Registry() = default;

    // Creates a new entity, recycling dead IDs if available.
    Entity CreateEntity()
    {
        Entity newEntity;
        if (!freeEntities.empty())
        {
            newEntity = freeEntities.back();
            freeEntities.pop_back();
        }
        else
        {
            newEntity = nextEntityId++;
        }
        return newEntity;
    }

    // Destroys an entity and cleans up all of its components across all pools.
    bool DestroyEntity(Entity entity)
    {
        // Tell every active pool to clean up this entity's component
        for (auto &pool : pools)
        {
            if (pool)
            {
                pool->Remove(entity);
            }
        }

        // Recycle the entity ID for future creation
        freeEntities.push_back(entity);
        return true;
    }

    // Assigns a component type to an entity with forwarded constructor arguments.
    template <typename T, typename... Args>
    T &Assign(Entity entity, Args &&...args)
    {
        uint32_t typeId = ComponentIdCounter::GetId<T>();

        // Ensure the pools vector has enough capacity
        if (typeId >= pools.size())
        {
            pools.resize(typeId + 1);
        }

        // Initialize the pool for this component type if it doesn't exist yet
        if (!pools[typeId])
        {
            pools[typeId] = std::make_unique<Pool<T>>();
        }

        // Cast to the correct Pool<T> and emplace the component
        auto *pool = static_cast<Pool<T> *>(pools[typeId].get());
        return pool->Emplace(entity, std::forward<Args>(args)...);
    }

    // Retrieves a pointer to an entity's component, or nullptr if it doesn't have it.
    template <typename T>
    T *Get(Entity entity)
    {
        uint32_t typeId = ComponentIdCounter::GetId<T>();
        if (typeId >= pools.size() || !pools[typeId])
        {
            return nullptr;
        }

        auto *pool = static_cast<Pool<T> *>(pools[typeId].get());
        return pool->Get(entity);
    }

    // Checks if an entity has a specific component.
    template <typename T>
    bool Has(Entity entity)
    {
        uint32_t typeId = ComponentIdCounter::GetId<T>();
        if (typeId >= pools.size() || !pools[typeId])
        {
            return false;
        }

        auto *pool = static_cast<Pool<T> *>(pools[typeId].get());
        return pool->Has(entity);
    }

    // 1. Single Component View: Returns all entities that have component T
    template <typename T>
    const std::vector<Entity> &View()
    {
        uint32_t typeId = ComponentIdCounter::GetId<T>();
        if (typeId >= pools.size() || !pools[typeId])
        {
            static const std::vector<Entity> emptyVector;
            return emptyVector; // Return an empty list if no pool exists yet
        }

        auto *pool = static_cast<Pool<T> *>(pools[typeId].get());
        return pool->GetEntities();
    }

    // 2. Multi-Component View: Returns entities that have ALL specified components (e.g., Position AND Velocity)
    template <typename T1, typename T2, typename... Rest>
    std::vector<Entity> View()
    {
        std::vector<Entity> result;

        // Start with the view of the first component type
        const auto &baseEntities = View<T1>();

        for (Entity entity : baseEntities)
        {
            // Check if the entity also has all the remaining components
            if ((Has<T2>(entity) && ... && (Has<Rest>(entity))))
            {
                result.push_back(entity);
            }
        }

        return result;
    }

private:
    uint32_t nextEntityId = 0;                 // Counter for generating new entity IDs
    std::vector<Entity> freeEntities;          // Stack of recycled entity IDs
    std::vector<std::unique_ptr<IPool>> pools; // List of heterogeneous component pools
};