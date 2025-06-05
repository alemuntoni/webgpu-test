
#include "application.h"

#include <iostream>

int main(int argc, char *argv[])
{
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    Application app;

    if (!app.initialize()) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    while (app.isRunning()) {
        app.mainLoop();
    }

    app.terminate();

    glfwTerminate();
}
