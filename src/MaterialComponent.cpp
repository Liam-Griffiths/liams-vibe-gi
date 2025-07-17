#include "../include/MaterialComponent.h"

MaterialComponent::MaterialComponent() {
    material = std::make_unique<Material>();
}

MaterialComponent::MaterialComponent(std::unique_ptr<Material> mat) : material(std::move(mat)) {}

std::unique_ptr<MaterialComponent> MaterialComponent::createPBR(const std::string& baseName) {
    auto material = std::make_unique<Material>();
    material->loadPBRMaterial(baseName);
    return std::make_unique<MaterialComponent>(std::move(material));
}

std::unique_ptr<MaterialComponent> MaterialComponent::createSolid(const glm::vec3& color, float roughness, float metallic) {
    auto material = std::make_unique<Material>(color, roughness, metallic);
    return std::make_unique<MaterialComponent>(std::move(material));
} 