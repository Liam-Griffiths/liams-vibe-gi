
// Mesh.h
#ifndef MESH_H
#define MESH_H

#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    unsigned int VAO;

    Mesh(std::vector<Vertex> verts);
    void Draw(unsigned int shaderID);

private:
    unsigned int VBO;
    void setupMesh();
};

#endif // MESH_H 