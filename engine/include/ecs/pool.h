#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include "entity.h"

// Non-template base interface so the Registry can manage 
// different component pools polymorphically.
struct IPool
{
    virtual ~IPool() = default;
    
    // Removes a component for a given entity (triggered when an entity is destroyed).
    virtual void Remove(Entity entity) = 0;
};

// Concrete component pool template
template <typename T>
class Pool : public IPool
{
public:
    // Checks if the specified entity owns this component.
    bool Has(Entity entity) const
    {
        return entity < sparse.size() && sparse[entity] != NULL_ENTITY;
    }

    // Retrieves a pointer to the entity's component, or nullptr if it doesn't exist.
    T* Get(Entity entity)
    {
        if (!Has(entity)) return nullptr;
        return &data[sparse[entity]];
    }

    // Constructs and assigns a component to an entity in-place.
    template <typename... Args>
    T& Emplace(Entity entity, Args&&... args)
    {
        // Resize the sparse array if the entity ID exceeds current capacity
        if (entity >= sparse.size()) {
            sparse.resize(entity + 1, NULL_ENTITY);
        }

        // If the entity already has this component, update it and return
        if (Has(entity)) {
            data[sparse[entity]] = T(std::forward<Args>(args)...);
            return data[sparse[entity]];
        }

        // Append the new component to the dense array for tight memory packing
        size_t denseIndex = data.size();
        data.push_back(T(std::forward<Args>(args)...));
        denseEntity.push_back(entity);

        // Map the entity ID to its index in the dense array
        sparse[entity] = denseIndex;

        return data.back();
    }

    // Removes the component from the entity using a fast "swap and pop" strategy.
    void Remove(Entity entity) override
    {
        if (!Has(entity)) return;

        size_t targetDenseIndex = sparse[entity];
        size_t lastDenseIndex = data.size() - 1;

        // Swap the target component with the last component to avoid memory holes
        data[targetDenseIndex] = std::move(data[lastDenseIndex]);
        denseEntity[targetDenseIndex] = denseEntity[lastDenseIndex];

        // Update the sparse map for the moved entity
        sparse[denseEntity[targetDenseIndex]] = targetDenseIndex;

        // Clear the removed entity's sparse entry
        sparse[entity] = NULL_ENTITY;

        // Pop the back element off the dense vectors
        data.pop_back();
        denseEntity.pop_back();
    }

    // Expose dense data arrays for high-performance system loops
    const std::vector<T>& GetData() const { return data; }
    const std::vector<Entity>& GetEntities() const { return denseEntity; }

private:
    std::vector<T> data;             // Contiguously packed component values
    std::vector<Entity> denseEntity; // Maps dense index back to Entity ID
    std::vector<Entity> sparse;      // Maps Entity ID to dense index (Sparse Set)
};