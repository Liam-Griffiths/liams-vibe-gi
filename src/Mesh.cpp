
// Mesh.cpp
#include "../include/Mesh.h"
#include <GLFW/glfw3.h> // For OpenGL
#include <OpenGL/gl3.h>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>

Mesh::Mesh(std::vector<Vertex> verts) : vertices(verts) {
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // tangent attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // bitangent attribute
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);
}

void Mesh::Draw(unsigned int shaderID) {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<unsigned int>(vertices.size()));
    glBindVertexArray(0);
}

std::unique_ptr<Mesh> Mesh::createSphere(float radius, int sectors, int stacks) {
    std::vector<Vertex> vertices;
    for (int y = 0; y < stacks; ++y) {
        for (int x = 0; x < sectors; ++x) {
            // Quad vertices
            float theta1 = static_cast<float>(y) / stacks * 3.14159265359f;
            float theta2 = static_cast<float>(y + 1) / stacks * 3.14159265359f;
            float phi1 = static_cast<float>(x) / sectors * 2 * 3.14159265359f;
            float phi2 = static_cast<float>(x + 1) / sectors * 2 * 3.14159265359f;
            // Vertex 1
            glm::vec3 p1(radius * sin(theta1) * cos(phi1), radius * sin(theta1) * sin(phi1), radius * cos(theta1));
            glm::vec3 n1 = glm::normalize(p1);
            glm::vec2 uv1(static_cast<float>(x) / sectors, static_cast<float>(y) / stacks);
            
            // Vertex 2
            glm::vec3 p2(radius * sin(theta2) * cos(phi1), radius * sin(theta2) * sin(phi1), radius * cos(theta2));
            glm::vec3 n2 = glm::normalize(p2);
            glm::vec2 uv2(static_cast<float>(x) / sectors, static_cast<float>(y + 1) / stacks);
            
            // Vertex 3
            glm::vec3 p3(radius * sin(theta1) * cos(phi2), radius * sin(theta1) * sin(phi2), radius * cos(theta1));
            glm::vec3 n3 = glm::normalize(p3);
            glm::vec2 uv3(static_cast<float>(x + 1) / sectors, static_cast<float>(y) / stacks);
            
            // Vertex 4
            glm::vec3 p4(radius * sin(theta2) * cos(phi2), radius * sin(theta2) * sin(phi2), radius * cos(theta2));
            glm::vec3 n4 = glm::normalize(p4);
            glm::vec2 uv4(static_cast<float>(x + 1) / sectors, static_cast<float>(y + 1) / stacks);
            
            // Calculate tangents (simplified for sphere)
            glm::vec3 tangent = glm::normalize(glm::cross(n1, glm::vec3(0, 1, 0)));
            glm::vec3 bitangent = glm::normalize(glm::cross(n1, tangent));
            
            // Triangle 1
            vertices.push_back({p1, n1, uv1, tangent, bitangent});
            vertices.push_back({p2, n2, uv2, tangent, bitangent});
            vertices.push_back({p3, n3, uv3, tangent, bitangent});
            // Triangle 2
            vertices.push_back({p3, n3, uv3, tangent, bitangent});
            vertices.push_back({p2, n2, uv2, tangent, bitangent});
            vertices.push_back({p4, n4, uv4, tangent, bitangent});
        }
    }
    return std::make_unique<Mesh>(vertices);
}

std::unique_ptr<Mesh> Mesh::createPlane(float width, float height, int segmentsX, int segmentsY) {
    std::vector<Vertex> vertices;
    
    // For tiling, we want UV coordinates to repeat across the surface
    float tileScale = 4.0f; // Tile the texture 4 times across each dimension
    
    // Generate vertices
    for (int y = 0; y <= segmentsY; ++y) {
        for (int x = 0; x <= segmentsX; ++x) {
            float xPos = (static_cast<float>(x) / segmentsX - 0.5f) * width;
            float zPos = (static_cast<float>(y) / segmentsY - 0.5f) * height;
            
            // Scale UV coordinates for tiling
            float u = static_cast<float>(x) / segmentsX * tileScale;
            float v = static_cast<float>(y) / segmentsY * tileScale;
            
            glm::vec3 position(xPos, 0.0f, zPos);
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            glm::vec2 texCoords(u, v);
            glm::vec3 tangent(1.0f, 0.0f, 0.0f);
            glm::vec3 bitangent(0.0f, 0.0f, -1.0f);
            
            // Create triangles for each quad
            if (x < segmentsX && y < segmentsY) {
                float u1 = (x + 1.0f) / segmentsX * tileScale;
                float v1 = (y + 1.0f) / segmentsY * tileScale;
                
                // First triangle
                vertices.push_back({position, normal, texCoords, tangent, bitangent});
                
                glm::vec3 pos1(xPos + width / segmentsX, 0.0f, zPos);
                glm::vec2 uv1(u1, v);
                vertices.push_back({pos1, normal, uv1, tangent, bitangent});
                
                glm::vec3 pos2(xPos, 0.0f, zPos + height / segmentsY);
                glm::vec2 uv2(u, v1);
                vertices.push_back({pos2, normal, uv2, tangent, bitangent});
                
                // Second triangle
                vertices.push_back({pos1, normal, uv1, tangent, bitangent});
                
                glm::vec3 pos3(xPos + width / segmentsX, 0.0f, zPos + height / segmentsY);
                glm::vec2 uv3(u1, v1);
                vertices.push_back({pos3, normal, uv3, tangent, bitangent});
                
                vertices.push_back({pos2, normal, uv2, tangent, bitangent});
            }
        }
    }
    
    return std::make_unique<Mesh>(vertices);
}

std::unique_ptr<Mesh> Mesh::createCube() {
    std::vector<Vertex> vertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

        // Back face
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

        // Left face
        {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},

        // Right face
        {{ 0.5f,  0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},

        // Bottom face
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},

        // Top face
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}
    };
    
    return std::make_unique<Mesh>(vertices);
}

std::unique_ptr<Mesh> Mesh::loadFromOBJ(const std::string& filepath) {
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec3> temp_normals;
    std::vector<Vertex> final_vertices;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << filepath << std::endl;
        return nullptr;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            // Vertex position
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        }
        else if (prefix == "vn") {
            // Vertex normal
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "f") {
            // Face (assuming triangulated faces)
            std::string vertex1, vertex2, vertex3;
            iss >> vertex1 >> vertex2 >> vertex3;
            
            // Parse each vertex (format: v/vt/vn or v//vn or just v)
            for (const std::string& vertexStr : {vertex1, vertex2, vertex3}) {
                int vertexIndex = -1;
                int normalIndex = -1;
                
                // Check if the vertex string contains slashes (v/vt/vn format)
                if (vertexStr.find('/') != std::string::npos) {
                    std::istringstream vertexStream(vertexStr);
                    std::string component;
                    
                    // Get vertex index
                    std::getline(vertexStream, component, '/');
                    vertexIndex = std::stoi(component) - 1; // OBJ is 1-indexed
                    
                    // Skip texture coordinate
                    std::getline(vertexStream, component, '/');
                    
                    // Get normal index
                    if (std::getline(vertexStream, component, '/') && !component.empty()) {
                        normalIndex = std::stoi(component) - 1; // OBJ is 1-indexed
                    }
                } else {
                    // Simple format: just vertex index
                    vertexIndex = std::stoi(vertexStr) - 1; // OBJ is 1-indexed
                }
                
                Vertex vertex;
                if (vertexIndex >= 0 && vertexIndex < temp_vertices.size()) {
                    vertex.Position = temp_vertices[vertexIndex];
                } else {
                    std::cerr << "Invalid vertex index: " << vertexIndex << std::endl;
                    continue;
                }
                
                if (normalIndex >= 0 && normalIndex < temp_normals.size()) {
                    vertex.Normal = temp_normals[normalIndex];
                } else {
                    // Calculate normal if not provided (will be calculated later)
                    vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                
                // Set default texture coordinates and tangents (could be improved)
                vertex.TexCoords = glm::vec2(0.0f);
                vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
                
                final_vertices.push_back(vertex);
            }
        }
    }
    
    file.close();
    
    // If no normals were provided, calculate them
    if (temp_normals.empty() && final_vertices.size() >= 3) {
        for (size_t i = 0; i < final_vertices.size(); i += 3) {
            if (i + 2 < final_vertices.size()) {
                glm::vec3 v1 = final_vertices[i + 1].Position - final_vertices[i].Position;
                glm::vec3 v2 = final_vertices[i + 2].Position - final_vertices[i].Position;
                glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
                
                final_vertices[i].Normal = normal;
                final_vertices[i + 1].Normal = normal;
                final_vertices[i + 2].Normal = normal;
            }
        }
    }
    
    std::cout << "Loaded OBJ file: " << filepath << " with " << final_vertices.size() << " vertices" << std::endl;
    return std::make_unique<Mesh>(final_vertices);
} 