/**
 * LightComponent.h - Point Light Component for Entity-Component-System
 * 
 * Defines the LightComponent class that represents point lights in the scene.
 * This component can be attached to entities to make them emit light, affecting
 * both direct lighting and global illumination calculations.
 * 
 * Light Model:
 * - Point light with position, color, intensity, and radius
 * - Physically-based attenuation with distance
 * - Supports both direct lighting and global illumination
 * - Color and intensity can be adjusted independently
 * 
 * Usage:
 * - Attach to an entity with a TransformComponent for positioning
 * - Used by shadow mapping system for shadow generation
 * - Contributes to radiance cascades global illumination
 * - Affects final composite lighting calculations
 */

#ifndef LIGHTCOMPONENT_H
#define LIGHTCOMPONENT_H

#include "Entity.h"
#include <glm/glm.hpp>

/**
 * LightComponent - Point light source for scene illumination
 * 
 * Represents a point light that emits light in all directions from its position.
 * The light has color, intensity, and radius parameters that control its
 * appearance and influence on the scene lighting.
 */
class LightComponent : public Component {
public:
    // Light Properties
    glm::vec3 color;            ///< Light color (RGB values, typically 0-1 range)
    float intensity;            ///< Light intensity multiplier (brightness)
    float radius;               ///< Light influence radius for attenuation

    /**
     * Constructor - Initialize light with default or specified parameters
     * 
     * @param col      Light color (default: white (1,1,1))
     * @param intens   Light intensity (default: 1.0)
     * @param rad      Light radius for attenuation (default: 2.0 units)
     */
    LightComponent(glm::vec3 col = glm::vec3(1.0f), float intens = 1.0f, float rad = 2.0f)
        : color(col), intensity(intens), radius(rad) {}
};

#endif // LIGHTCOMPONENT_H 