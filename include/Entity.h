
// Entity.h
#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include <memory>

class Component;

class Entity {
public:
    void addComponent(std::unique_ptr<Component> component);
    template <typename T>
    T* getComponent() {
        for (auto& comp : components) {
            if (T* c = dynamic_cast<T*>(comp.get())) {
                return c;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Component>> components;
};

class Component {
public:
    virtual ~Component() = default;
};

#endif // ENTITY_H 