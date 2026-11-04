#pragma once

#define GLFW_INCLUDE_NONE
#include "Dependencies/glfw-3.4/include/GLFW/glfw3.h"

namespace GameInput
{
inline unsigned char Character(int key)
{
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
    {
        return static_cast<unsigned char>('a' + key - GLFW_KEY_A);
    }
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    {
        return static_cast<unsigned char>('0' + key - GLFW_KEY_0);
    }
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9)
    {
        return static_cast<unsigned char>('0' + key - GLFW_KEY_KP_0);
    }
    switch (key)
    {
        case GLFW_KEY_SPACE:
            return ' ';
        case GLFW_KEY_ESCAPE:
            return 27;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            return '\r';
        default:
            return 0;
    }
}

inline bool SprintHeld()
{
    GLFWwindow* window = glfwGetCurrentContext();
    return window && glfwGetWindowAttrib(window, GLFW_FOCUSED) &&
           (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
}
} // namespace GameInput
