/**
 * Behaviour.h - Base class for script components (similar to Unity's MonoBehaviour)
 * 
 * This base class provides a common interface for all script components,
 * including lifecycle methods like Start, Update, and common functionality
 * that all behaviours might need.
 */

#ifndef BEHAVIOUR_H
#define BEHAVIOUR_H

#include "../include/Entity.h"

// Forward declarations
class TransformComponent;
class Entity;

/**
 * Behaviour - Base class for all script components
 * 
 * Provides a Unity-like MonoBehaviour interface with lifecycle methods
 * and common functionality. All custom script components should inherit
 * from this class to get standardized behavior management.
 */
class Behaviour : public Component {
public:
    /**
     * Constructor
     */
    Behaviour();

    /**
     * Virtual destructor
     */
    virtual ~Behaviour() = default;

    /**
     * Called once when the component is first created/added to an entity
     * Override this to initialize your component
     */
    virtual void Start() {}

    /**
     * Called every frame
     * @param deltaTime Time elapsed since last frame in seconds
     */
    virtual void Update(float deltaTime) {}

    /**
     * Called every frame after Update (useful for camera follow, etc.)
     * @param deltaTime Time elapsed since last frame in seconds
     */
    virtual void LateUpdate(float deltaTime) {}

    /**
     * Called when the component is destroyed
     * Override this to clean up resources
     */
    virtual void OnDestroy() {}

    /**
     * Get the entity this behaviour is attached to
     * @return Pointer to the parent entity
     */
    Entity* getEntity() const { return parentEntity; }

    /**
     * Get a component from the parent entity
     * @return Pointer to the component, or nullptr if not found
     */
    template<typename T>
    T* getComponent() const {
        if (parentEntity) {
            return parentEntity->getComponent<T>();
        }
        return nullptr;
    }

    /**
     * Get the transform component from the parent entity (convenience method)
     * @return Pointer to TransformComponent, or nullptr if not found
     */
    TransformComponent* getTransform() const;

    /**
     * Set the parent entity (called by the entity system)
     * @param entity The entity this behaviour belongs to
     */
    void setEntity(Entity* entity) { parentEntity = entity; }

    /**
     * Check if this behaviour has been started
     * @return True if Start() has been called
     */
    bool hasStarted() const { return started; }

    /**
     * Mark this behaviour as started (called by the behaviour system)
     */
    void markStarted() { started = true; }

    /**
     * Enable or disable this behaviour
     * @param enabled Whether the behaviour should be active
     */
    void setEnabled(bool enabled) { this->enabled = enabled; }

    /**
     * Check if this behaviour is enabled
     * @return True if the behaviour is active
     */
    bool isEnabled() const { return enabled; }

protected:
    Entity* parentEntity = nullptr;     ///< The entity this behaviour is attached to
    bool started = false;               ///< Whether Start() has been called
    bool enabled = true;                ///< Whether this behaviour is active

private:
    // Prevent copying
    Behaviour(const Behaviour&) = delete;
    Behaviour& operator=(const Behaviour&) = delete;
};

#endif // BEHAVIOUR_H 