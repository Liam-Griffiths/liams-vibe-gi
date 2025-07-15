#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <map>
#include <string>
#include <glm/glm.hpp>
#include "../include/Shader.h"

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

class TextRenderer {
public:
    TextRenderer(const std::string& font, unsigned int size, Shader* shader);
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, glm::mat4 projection);
private:
    std::map<char, Character> Characters;
    unsigned int VAO, VBO;
    Shader* textShader;
    void LoadFont(const std::string& font, unsigned int size);
};

#endif 