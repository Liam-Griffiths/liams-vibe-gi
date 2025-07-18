#ifndef MATERIALCOMPONENT_H
#define MATERIALCOMPONENT_H

#include "Entity.h"
#include "Material.h"
#include <memory>

class MaterialComponent : public Component {
public:
    std::unique_ptr<Material> material;
    
    MaterialComponent();
    MaterialComponent(std::unique_ptr<Material> mat);
    virtual ~MaterialComponent() = default;
    
    // Helper function to create PBR material
    static std::unique_ptr<MaterialComponent> createPBR(const std::string& baseName);
    static std::unique_ptr<MaterialComponent> createPBR(const std::string& baseName, const glm::vec2& tiling, float heightScale = 0.02f);
    static std::unique_ptr<MaterialComponent> createSolid(const glm::vec3& color, float roughness = 0.5f, float metallic = 0.0f);
};

#endif // MATERIALCOMPONENT_H 