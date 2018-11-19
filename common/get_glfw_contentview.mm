#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

void* get_glfw_contentview(GLFWwindow *win) {
  NSWindow* w = glfwGetCocoaWindow(win);
  return (void*)CFBridgingRetain(w.contentView);
}
