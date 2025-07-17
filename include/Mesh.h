
// Mesh.h
#ifndef MESH_H
#define MESH_H

#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <string>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    unsigned int VAO;

    Mesh(std::vector<Vertex> verts);
    static std::unique_ptr<Mesh> createSphere(float radius, int sectors = 20, int stacks = 20);
    static std::unique_ptr<Mesh> createPlane(float width, float height, int segmentsX = 1, int segmentsY = 1);
    static std::unique_ptr<Mesh> createCube();
    static std::unique_ptr<Mesh> loadFromOBJ(const std::string& filepath);
    void Draw(unsigned int shaderID);

private:
    unsigned int VBO;
    void setupMesh();
};

#endif // MESH_H 