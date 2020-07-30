#define GLFW_TRUE 1
#define NUM_BUFFERS 4
#define USE_TEX 1
#define TEX_W (SCR_W / SCALE)
#define TEX_H (SCR_H / SCALE)
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#endif

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

void mouse_pos_callback(GLFWwindow* window, double x, double y);
void mouse_btn_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void *work(void *args);
void texupdate();

float palette_rgb[3*16];

void fill(float *buf, size_t len) {
  for (size_t j=0; j<len; j++) {
    buf[j] = rand() / (float)(RAND_MAX + 1.0f);
  }
}

GLuint tex[NUM_BUFFERS];
float buffer[4 * TEX_W * TEX_H];
int tex_update_needed = 1;
int tex_no = 0;
int mode = 0;
double t0;
char debug_msg[256] = "\0";
#define AUTO_REFRESH 60
#define OFFSET 64
static GLFWwindow* window;

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

int display_init(int argc, char **argv) {

  GLuint width = 2*TEX_W;
  GLuint height = 2*TEX_H;

  GLFWwindow *window = NULL;
  const GLubyte *renderer;
  const GLubyte *version;
  GLuint vbo, tex_vbo, vao;
  GLuint vert_shader, frag_shader;
  GLuint shader_prog;

  float scale = 1.0f;
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

  //window = glfwCreateWindow( width, 4*height, "GLSL test", NULL, NULL);
  window = glfwCreateWindow( 2*width, 2*height, "Another VM", NULL, NULL);

  if (!window) {
    fprintf(stderr,
      "couldn't initialize GLFWwindow\n");
    glfwTerminate();
    return -1;
  }

  glfwSetWindowPos(window, OFFSET, OFFSET);

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

  //for (int i=0; i<NUM_BUFFERS; i++) {
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TEX_W, 4*TEX_H, 0, GL_RED, GL_FLOAT, buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 2*TEX_W, 2*TEX_H, 0, GL_RED, GL_FLOAT, buffer);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  //}

  fill(buffer, TEX_W*4*TEX_H);

  for (int j=0; j<4; j++)
  for (int r=0; r<TEX_H; r++)
  for (int c=0; c<TEX_W; c++) {
    u16 i=r*TEX_W+c;
    buffer8[i+j*TEX_W*TEX_H] = 0;
  }
  tex_update_needed=1;

  glfwSetCursorPosCallback(window, mouse_pos_callback);
  glfwSetMouseButtonCallback(window, mouse_btn_callback);
  glfwSetKeyCallback(window, key_callback);

  /* main loop */
  int vsync = 1;
  glfwSwapInterval(vsync);

  gl_ok=1; t0 = get_time();

  pthread_t work_thread;

  if (pthread_create(&work_thread, NULL, work, argv[1])) { fprintf(stderr, "couln't create a thread\n"); return -1; }

  GLuint palette = glGetUniformLocation(shader_prog, "palette");

  while (!glfwWindowShouldClose(window)) {

    /* clear */
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_prog);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, tex[tex_no]);

    if (1 == tex_update_needed) {
      texupdate(); tex_update_needed = 0;
    }

    glfwSetWindowTitle(window, debug_msg);

    glUniform1fv(palette, 16*3, palette_rgb);

    /* draw */
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwPollEvents();
    if (GLFW_PRESS == glfwGetKey(
      window, GLFW_KEY_ESCAPE))
      glfwSetWindowShouldClose(window, 1);
    glfwSwapBuffers(window);
  }

  gl_ok=0;
  glfwTerminate();
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &tex_vbo);
  glDeleteTextures(NUM_BUFFERS, tex);

  if (pthread_join(work_thread, NULL)) {
  printf("pthread_join error\n"); return -2; }

  if (NULL != img) free(img);
  return 0;
}

void texupdate() {

  glBindTexture(GL_TEXTURE_2D, tex[0]);
  //for (int i=0; i<4*TEX_W*TEX_H; i++) buffer[i] = (float)(buffer8[i])/255.0f;
  for (int k=0; k<4; k++)
  for (int i=0; i<TEX_H; i++)
  for (int j=0; j<TEX_W; j++) {
    buffer[(2*i+k%2)*TEX_W+j+((k>>1)<<1)*TEX_W*TEX_H] = (float)(buffer8[k*TEX_W*TEX_H+i*TEX_W+j])/255.0f;
  }
  //glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX_W, 4*TEX_H, GL_RED, GL_FLOAT, buffer );
  glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 2*TEX_W, 2*TEX_H, GL_RED, GL_FLOAT, buffer);

}

int drag_active = 0;
float mx = -1.f;
float my = -1.f;

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
}

#define bind_key_toggle(x,y) \
{ if (action == GLFW_PRESS && key == (x)) (y) = (y); if (action == GLFW_RELEASE && key == (x)) { (y) = ((y)+1) % NUM_BUFFERS ; printf(#x ", " #y "=%u\n", (y));} }
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  bind_key_toggle(GLFW_KEY_T,     tex_no);
  bind_key_toggle(GLFW_KEY_M,     mode);
  if (action==GLFW_PRESS &&
    key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  else if (action==GLFW_RELEASE) {
    switch(key) {
      case GLFW_KEY_0: mode=0; break;
      case GLFW_KEY_1: mode=1; break;
      case GLFW_KEY_2: mode=2; break;
      case GLFW_KEY_3: mode=3; break;
      case GLFW_KEY_SPACE: paused^=1; break;
      case GLFW_KEY_S: step=1; break;
      default: break;
    }
    texupdate();
  }
}
