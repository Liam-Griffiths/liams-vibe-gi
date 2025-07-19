/**
 * Behaviour.cpp - Implementation of the base Behaviour class
 */

#include "Behaviour.h"
#include "../include/TransformComponent.h"

Behaviour::Behaviour() 
    : parentEntity(nullptr), started(false), enabled(true) {
}

TransformComponent* Behaviour::getTransform() const {
    if (parentEntity) {
        return parentEntity->getComponent<TransformComponent>();
    }
    return nullptr;
} 