#ifndef FULLSCREENQUAD_H
#define FULLSCREENQUAD_H

class FullscreenQuad {
public:
    unsigned int VAO, VBO;
    
    FullscreenQuad();
    ~FullscreenQuad();
    
    void render();
    
private:
    void setupQuad();
};

#endif // FULLSCREENQUAD_H 