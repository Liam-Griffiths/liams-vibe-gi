#include "../include/MaterialComponent.h"
#include <iostream>

MaterialComponent::MaterialComponent() {
    material = std::make_unique<Material>();
}

MaterialComponent::MaterialComponent(std::unique_ptr<Material> mat) : material(std::move(mat)) {}

std::unique_ptr<MaterialComponent> MaterialComponent::createPBR(const std::string& baseName) {
    auto material = std::make_unique<Material>();
    material->loadPBRMaterial(baseName);
    return std::make_unique<MaterialComponent>(std::move(material));
}

std::unique_ptr<MaterialComponent> MaterialComponent::createPBR(const std::string& baseName, const glm::vec2& tiling, float heightScale) {
    auto material = std::make_unique<Material>();
    material->loadPBRMaterial(baseName);
    material->tiling = tiling;
    material->heightScale = heightScale;
    return std::make_unique<MaterialComponent>(std::move(material));
}

std::unique_ptr<MaterialComponent> MaterialComponent::createSolid(const glm::vec3& color, float roughness, float metallic) {
    auto material = std::make_unique<Material>(color, roughness, metallic);
    return std::make_unique<MaterialComponent>(std::move(material));
}

std::unique_ptr<MaterialComponent> MaterialComponent::createEmissive(const glm::vec3& color, const glm::vec3& emission, float roughness, float metallic) {
    auto material = std::make_unique<Material>(color, roughness, metallic, emission);
    return std::make_unique<MaterialComponent>(std::move(material));
} 