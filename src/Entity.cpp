
// Entity.cpp
#include "../include/Entity.h"
#include "../scripts/Behaviour.h"

void Entity::addComponent(std::unique_ptr<Component> component) {
    // If it's a Behaviour, set the parent entity
    if (auto behaviour = dynamic_cast<Behaviour*>(component.get())) {
        behaviour->setEntity(this);
    }
    components.push_back(std::move(component));
} 