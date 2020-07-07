#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint16_t u16;

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_GLCOREARB
#endif

#include <GLFW/glfw3.h>

#define USE_TEX 1
#define NUM_BUFFERS 8

#define REAL_TEX_W 320
#define REAL_TEX_H 200
#define SCALE 1
#define TEX_W (REAL_TEX_W / SCALE)
#define TEX_H (REAL_TEX_H / SCALE)
//#define DEF_FNAME "images/lena.txt"

float buffer[TEX_W * TEX_H];

GLuint tex[NUM_BUFFERS];

#define bind_key_toggle(x,y) \
{ if (action == GLFW_PRESS && key == (x)) (y) = (y); if (action == GLFW_RELEASE && key == (x)) { (y) = ((y)+1) % NUM_BUFFERS ; printf(#x ", " #y "=%u\n", (y));} }

int mode = 1;
int tex_no = 0;
int drag_active = 0;
float mx = -1.f;
float my = -1.f;
float scale = 1.0f;

void draw_point(float *buffer, u16 page, u16 y, u16 x) {

  u16 offset = page * (320*200) + y*200 + x;
  buffer[offset] = 1.0f;

}

void texupdate() {

  glBindTexture(GL_TEXTURE_2D, tex[tex_no]);
  //for (int i=0; i<TEX_W*TEX_H; i++) buffer[i] = buffer[tex_no][i];
  glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX_W, TEX_H, GL_RED, GL_FLOAT, buffer );

}

void mouse_btn_callback(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    drag_active = 1;
  }
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    drag_active = 0;
  }

}

void mouse_pos_callback(GLFWwindow* window, double x, double y) {

  int xpos, ypos;
  int width, height;
  glfwGetWindowSize(window, &width, &height);
  mx = x/(double)width;
  my = y/(double)height;
  xpos = (int)(mx * TEX_W);
  ypos = (int)(my * TEX_H);
  //printf("%f %f, %f %f, data[%d][%d] = %f\n", x, y, mx, my, ypos, xpos, data[tex_no][xpos + ypos*TEX_W]);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  bind_key_toggle(GLFW_KEY_T,     tex_no);
  bind_key_toggle(GLFW_KEY_M,     mode);
  if (action==GLFW_PRESS &&
    key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  else if (action==GLFW_RELEASE) {
    switch(key) {
      //case GLFW_KEY_0:
      //  diag();
      //  texupdate(); break;
      //case GLFW_KEY_1:
      //  r();
      //  texupdate(); break;
      default:
      break;
    }
  }
}

const char *vs_name = "./glsl/tex_vs.glsl";
const char *fs_name = "./glsl/tex_fs.glsl";

char *load_src(const char *file) {
  FILE *f = fopen(file, "r");
  if (!f) {
    fprintf(stderr,
      "couldn't open %s\n", file);
    return NULL;
  }
  fseek(f, 0L, SEEK_END);
  int len = ftell(f);
  rewind(f);

  char *src = malloc(len+1);
  size_t cnt = fread(src, len, 1, f);
  if (0==cnt)
    fprintf(stderr, "fread failed\n");
  fclose(f);
  src[len] = '\0';
  return src;
}

void check_err(const char *m, GLuint *s) {
  GLint res = GL_FALSE;
  int log_len;
  glGetShaderiv(*s, GL_COMPILE_STATUS, &res);
  glGetShaderiv(*s, GL_INFO_LOG_LENGTH, &log_len);
  if (log_len > 0) {
    char *message = malloc(log_len+1);
    glGetShaderInfoLog(*s, log_len, NULL, message);
    printf("%s: %s\n", m, message);
    free(message);
  }
}

void load_shaders(GLuint *v, const char *vf,
                  GLuint *f, const char *ff) {
  char *v_src = load_src(vf);
  char *f_src = load_src(ff);
  *v = glCreateShader(GL_VERTEX_SHADER);
  *f = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(*v, 1, (const char*const*)&v_src, NULL);
  glShaderSource(*f, 1, (const char*const*)&f_src, NULL);
  glCompileShader(*v);
  glCompileShader(*f);
  free(v_src);
  free(f_src);

  /* check */
  check_err("vertex shader", v);
  check_err("fragment shader", f);
}

#define echo(x...) \
  do { \
    puts(#x) ; \
    x; \
  } while (0)

void fill(float *buf) {
  for (int row=0; row<TEX_H; row++)
    for (int j=0; j<TEX_W; j++) {
      buf[j+row*TEX_W] = 0.25f * j; //rand() / (float)(RAND_MAX + 1.0f);
      printf("buf[%d+%d*TEX_W] = %f\n", j, row, buf[j+row*TEX_W]);
    }
}

int main(int argc, char **argv) {

  //GLuint width = argc > 1 ?  strtol(argv[1], NULL, 10) : TEX_W;
  //GLuint height = argc > 2 ?  strtol(argv[2], NULL, 10) : TEX_H;
  GLuint width = TEX_W;
  GLuint height = TEX_H;
  //char *fname = argc > 1 ? argv[1] : DEF_FNAME;

  GLFWwindow *window = NULL;
  const GLubyte *renderer;
  const GLubyte *version;
  GLuint vbo, tex_vbo, vao;
  GLuint vert_shader, frag_shader;
  GLuint shader_prog;

  float size = scale;

  GLfloat vertices[] = {
   -size,  size,
    size,  size,
    size, -size,
    size, -size,
   -size, -size,
   -size,  size
  };

  GLfloat texcoords[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f
  };

  if (!glfwInit()) {
    fprintf(stderr,
      "couldn't initialize glfw3\n");
    return -1;
  }

  //glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
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
    sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &tex_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
  glBufferData(GL_ARRAY_BUFFER,
    sizeof(texcoords), texcoords, GL_STATIC_DRAW);

  /* use the vbo and use 2 float per 'variable' */
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  load_shaders(&vert_shader, vs_name, &frag_shader, fs_name);

  shader_prog = glCreateProgram();
  glAttachShader(shader_prog, frag_shader);
  glAttachShader(shader_prog, vert_shader);
  glLinkProgram(shader_prog);

  float *img = malloc(TEX_H * TEX_W * sizeof(float));

  glGenTextures( NUM_BUFFERS, tex );
  glActiveTexture(GL_TEXTURE0);

  for (int i=0; i<NUM_BUFFERS; i++) {
    glBindTexture(GL_TEXTURE_2D, tex[i]);
    fill(buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TEX_W, TEX_H, 0, GL_RED, GL_FLOAT, buffer);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }

  GLuint cursor     = glGetUniformLocation(shader_prog, "cursor");
  GLuint colormap_a = glGetUniformLocation(shader_prog, "colormap_a");
  GLuint colormap_b = glGetUniformLocation(shader_prog, "colormap_b");
  GLuint colormap_c = glGetUniformLocation(shader_prog, "colormap_c");
  GLuint colormap_d = glGetUniformLocation(shader_prog, "colormap_d");

  float a[4][3] = { {0.5f, 0.5f, 0.5f }, {0.5f, 0.5f, 0.5f },
                    {0.0f, 0.0f, 0.0f }, {0.5f, 0.5f, 0.5f }};

  float b[4][3] = { {0.5f,  0.5f, 0.5f },{ 0.5f, 0.5f, 0.5f },
                    {-1.0f,-1.0f,-1.0f },{-0.5f,-0.5f,-0.5f }};

  float c[4][3] = { {1.0f, 1.0f, 1.0f }, {2.0f, 1.0f, 0.0f },
                    {1.0f, 1.0f, 1.0f }, {1.0f, 1.0f, 1.0f }};

  float d[4][3] = { {0.0f,0.33f,0.67f }, {0.5f, 0.2f, 0.25f},
                    {0.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 0.0f }};

  glfwSetCursorPosCallback(window, mouse_pos_callback);
  glfwSetMouseButtonCallback(window, mouse_btn_callback);
  glfwSetKeyCallback(window, key_callback);

  /* main loop */
  int vsync = 1;
  glfwSwapInterval(vsync);

  while (!glfwWindowShouldClose(window)) {

    /* clear */
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_prog);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, tex[tex_no]);

    glUniform2f(cursor,     mx, my);
    glUniform3f(colormap_a, a[mode][0], a[mode][1], a[mode][2]);
    glUniform3f(colormap_b, b[mode][0], b[mode][1], b[mode][2]);
    glUniform3f(colormap_c, c[mode][0], c[mode][1], c[mode][2]);
    glUniform3f(colormap_d, d[mode][0], d[mode][1], d[mode][2]);

    /* draw */
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwPollEvents();
    if (GLFW_PRESS == glfwGetKey(
      window, GLFW_KEY_ESCAPE))
      glfwSetWindowShouldClose(window, 1);
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &tex_vbo);
  glDeleteTextures(NUM_BUFFERS, tex);

  if (NULL != img) free(img);
  return 0;
}
