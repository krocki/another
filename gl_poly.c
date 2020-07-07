#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_GLCOREARB
#endif

#include <GLFW/glfw3.h>

int main(int argc, char **argv) {

  GLuint width = argc > 1 ?
    strtol(argv[1], NULL, 10) : 800;
  GLuint height = argc > 2 ?
    strtol(argv[2], NULL, 10) : 600;

  GLFWwindow *window = NULL;
  const GLubyte *renderer;
  const GLubyte *version;
  GLuint vbo, vao;
  GLuint vert_shader, frag_shader;
  GLuint shader_prog;

  GLfloat vertices[] = {
     0.0f,  0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f
  };

  const char *vertex_shader =
    "#version 410\n"
    "in vec3 vp;"
    "void main () {"
    "  gl_Position = vec4(vp, 1.0);"
    "}";

  const char *fragment_shader =
    "#version 410\n"
    "out vec4 frag_color;"
    "void main () {"
    "  frag_color = vec4(0.5, 0.0, 0.5, 1.0);"
    "}";

  if (!glfwInit()) {
    fprintf(stderr,
      "couldn't initialize glfw3\n");
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,
  GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE,
  GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(
    width, height, "GLSL test", NULL, NULL);

  if (!window) {
    fprintf(stderr,
      "couldn't initialize GLFWwindow\n");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  renderer = glGetString(GL_RENDERER);
  version = glGetString(GL_VERSION);

  fprintf(stdout,
    "GL_RENDERER: %s\n"
    "GL_VERSION: %s\n",
    renderer, version);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  /* allocate gpu's memory for vertices */
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER,
    9 * sizeof(GLfloat),
    vertices, GL_STATIC_DRAW);

  /* use the vbo and use 3 float per 'variable' */
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT,
    GL_FALSE, 0, NULL);

  /* vertex shader */
  vert_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert_shader, 1,
    &vertex_shader, NULL);
  glCompileShader(vert_shader);

  /* fragment shader */
  frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader, 1,
    &fragment_shader, NULL);
  glCompileShader(frag_shader);

  /* link glsl program */
  shader_prog = glCreateProgram();
  glAttachShader(shader_prog, frag_shader);
  glAttachShader(shader_prog, vert_shader);
  glLinkProgram(shader_prog);

  /* main loop */
  while (!glfwWindowShouldClose(window)) {
    /* clear */
    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_prog);
    glBindVertexArray(vao);

    /* draw */
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwPollEvents();
    if (GLFW_PRESS == glfwGetKey(
      window, GLFW_KEY_ESCAPE))
      glfwSetWindowShouldClose(window, 1);
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}
