
// Window.h
#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    bool shouldClose();
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getGLFWWindow() const;

private:
    GLFWwindow* window;
};

#endif // WINDOW_H 