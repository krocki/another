#include "defs.h"

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

double get_time() {
  struct timeval tv; gettimeofday(&tv, NULL);
  return (tv.tv_sec + tv.tv_usec * 1e-6);
}

#define SCALE 1
#define SCR_W 320
#define SCR_H 200
#define PAGE_SIZE (SCR_W*SCR_H)

#define USE_GL 1
#if USE_GL
#define NUM_BUFFERS 4
#define USE_TEX 1
#define TEX_W (SCR_W / SCALE)
#define TEX_H (SCR_H / SCALE)
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_GLCOREARB
#endif

#include <GLFW/glfw3.h>
GLuint tex[NUM_BUFFERS];
float buffer[4 * TEX_W * TEX_H];
int tex_update_needed = 1;
int tex_no = 0;
double t0;
int gl_ok=0;
char debug_msg[256] = "\0";
#define AUTO_REFRESH 60
#define OFFSET 64
static GLFWwindow* window;
#endif

typedef struct {
  int id;
} vertex;

typedef struct {
  u8 regs[256];
  u16 pc;
  u32 ticks;
} another_vm;

u8 page_front;
u8 page_back;
u8 page_current;

u8 buffer8[PAGE_SIZE * 4];

void *work(void *args);

u8 get_page(u8 num) {
  switch (num) {
    case 0xff: return page_back; break;
    case 0xfe: return page_front; break;
    case 0x0 ... 0x3: return num; break;
    default: printf("illegal page\n"); return 0; break;
  }

}

void fill_buf(u8 *buf, u16 len, u8 val) {
  while (len-- > 0) *buf++ = val;
}

void fill_page(u8 num, u8 color) {
  printf("fill page(page=0x%02x, color=0x%02x)\n", num, color);
  u8 pg = get_page(num);
  fill_buf(buffer8+pg*PAGE_SIZE, PAGE_SIZE, color);
}

#define F8 (code[vm->pc++])
void tick(another_vm *vm, u8 *code) {

  printf("[tick: %u, pc=0x%04x] ", vm->ticks, vm->pc);
  u8 op = F8;
  printf("op=0x%02x ", op);

  sprintf(debug_msg, "[%u, pc=0x%04x, op=0x%02x]", vm->ticks, vm->pc, op);
  u8 arg0, arg1;
  u16 arg;

  switch (op) {
    case 0x80 ... 0xff:
      printf("DRAW_POLY_BACKGROUND\n");
      break;
    case 0x40 ... 0x7f:
      printf("DRAW_POLY_SPRITE\n");
      break;
    case 0x20 ... 0x3f:
      printf("ILLEGAL OP\n");
      break;
    case 0x00 ... 0x1f:
      switch (op) {
        case 0x4:
          // CALL
          arg = F8 << 8 | F8;
          printf("CALL 0x%04x\n", arg);
          vm->pc = arg;
          break;
        case 0xe:
          // FILL page
          arg0 = F8; arg1 = F8;
          fill_page(arg0, arg1);
          break;
        default:
          printf("NOT IMPLEMENTED!\n");
          break;
      }
      break;
  }
  vm->ticks++;
}

void draw_shape(u8 *p, u16 offset, u8 color, u8 zoom, u8 x, u8 y);

u8 current_page0 = 0;

void draw_point(u8 pg, u8 color, u8 x, u8 y) {
  printf("draw_point pg=%x, color=%x, x=%x, y=%x\n", pg, color, x, y);
}

void draw_polygon(u8 pg, u8 color, vertex *vertices) {
  printf("draw_polygon pg=%x, color=%x, vertices=%p\n", pg, color, vertices);
}

void fill_polygon(u8 *data, u16 offset, u8 color, u8 zoom, u8 x, u8 y) {

  printf("fill_polygon=0x%04x\n", offset);
  u8 w = (data[offset++] * zoom/64);
  u8 h = (data[offset++] * zoom/64);
  int x1 = (x * SCALE - w * SCALE/2 );
  int x2 = (x * SCALE + w * SCALE/2 );
  int y1 = (y * SCALE - h * SCALE/2 );
  int y2 = (y * SCALE + h * SCALE/2 );

  if ( x1 >= SCR_W || x2 < 0 || y1 >= SCR_H || y2 < 0)
    return;

  u8 count = data[offset++];
  printf("fill_polygon w=0x%04x h=0x%04x x=0x%04x y=0x%04x cnt=%d\n", w, h, x, y, count);

  vertex *vertices = NULL;

  for (u16 i=0; i<=count; i++) {
    int vx = x1 + ((data[offset++] * zoom/64)) * SCALE;
    int vy = y1 + ((data[offset++] * zoom/64)) * SCALE;
    printf("vertex %d x1=%d y1=%d vx=%d, vy=%d\n", i, x1, y1, vx, vy);
  }
  if (4 == count && 0 == w & h <=1 ) {
    draw_point(current_page0, color, x1, y1);
  } else {
    draw_polygon(current_page0, color, vertices);
  }

}

void draw_shape_parts(u8 *data, u16 offset, u8 zoom, u8 x, u8 y) {

  u8 x0 = x - ( data[offset++] * zoom / 64 );
  u8 y0 = y - ( data[offset++] * zoom / 64 );
  u8 count = data[offset++];

  printf("draw_shape_parts x0=0x%02x, y0=0x%02x, count=0x%02x\n", x0, y0, count);

  for (u16 i=0; i<=count; i++) {
    u16 addr = ( data[offset] << 8 ) | data[offset+1];
    offset += 2;
    u16 x1 = x0 + ( data[offset++] * zoom/64 );
    u16 y1 = y0 + ( data[offset++] * zoom/64 );
    u8 color = 0xff;
    if (addr & 0x8000) {
      color = data[offset] & 0x7f; offset += 2;
    }
    draw_shape(data, ((addr<<1) & 0xfffe), color, zoom, x1, y1);
  }
}

void draw_shape(u8 *data, u16 offset, u8 color, u8 zoom, u8 x, u8 y) {

  printf("draw_shape offset=0x%04x\n", offset);
  u8 code = data[offset++];
  printf("draw_shape code=0x%04x\n", code);
  if (code >= 0xc0) {
    if (color & 0x80) {
      color = code & 0x3f;
    }
    fill_polygon(data, offset, color, zoom, x, y);
  } else {
    if (2 == (code & 0x3f)) {
      draw_shape_parts(data, offset, zoom, x, y);
    }
  }
}

void draw_poly_background(u8 op0, u8 op1, u8 x, u8 y, u8 *p) {

  u16 offset = (((op0 << 8) | op1) << 1) & 0xfffe;
  int h = y - 199;
  if (h > 0) { y = 199; x += h; }

  printf("draw_poly_bkgd offset = 0x%04x, x = 0x%02x, y = 0x%02x, h = 0x%02x\n", offset, x, y, h);

  draw_shape(p, offset, 0xff, 64, x, y);
}

//int main(int argc, char **argv) {
//
//  byte_array palette, bytecode, polygons;
//  read_array(&palette,  "file17");
//  read_array(&bytecode, "file18");
//  read_array(&polygons, "file19");
//
//  u32 MAX_TICKS = 2;
//
//  another_vm vm;
//  memset(&vm, 0, sizeof vm);
//
//  for (u32 i=0; i<MAX_TICKS; i++)
//    tick(&vm, bytecode.bytes);
//
//  free(palette.bytes);
//  free(bytecode.bytes);
//  free(polygons.bytes);
//
//  return 0;
//}

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

void fill(float *buf, size_t len) {
  for (size_t j=0; j<len; j++) {
    buf[j] = rand() / (float)(RAND_MAX + 1.0f);
  }
}

//void texupdate() {
//
//  printf("tex %d update\n", tex_no);
//  for (int j=0; j<4; j++)
//  {
//    glBindTexture(GL_TEXTURE_2D, tex[j]);
//    for (int i=0; i<TEX_W*TEX_H*4; i++) buffer[i] = (float)(buffer8[i])/255.0f;
//    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX_W, TEX_H, GL_RED, GL_FLOAT, &buffer[j*TEX_W*TEX_H] );
//  }
void texupdate() {

  printf("tex %d update\n", tex_no);
  //{
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    //int rr = j / 2;
    //int cc = j % 2;
    //for (int i=0; i<TEX_W*TEX_H; i++) buffer[rr*TEX_H*TEX_W+i+TEX_W*cc] = (float)(buffer8[rr*TEX_H*TEX_W+i+TEX_W*cc])/255.0f;
    for (int i=0; i<4*TEX_W*TEX_H; i++) buffer[i] = (float)(buffer8[i])/255.0f;
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX_W, 4*TEX_H, GL_RED, GL_FLOAT, buffer );
  //}

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
  //printf("%f %f, %f %f, data[%d][%d] = %f\n", x, y, mx, my, ypos, xpos, data[tex_no][xpos + ypos*TEX_W]);
}

int mode = 1;
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
      case GLFW_KEY_0:
        texupdate(); break;
      case GLFW_KEY_1:
        fill(buffer, 4*PAGE_SIZE); texupdate(); break;
      default:
      break;
    }
  }
}

static void change_size_callback(GLFWwindow* window, int width, int height) {
  //WIDTH = width;
  //HEIGHT = height;
}

static GLFWwindow* open_window(const char* title, GLFWwindow* share, int posX, int posY) {

    GLFWwindow* window;

    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(TEX_W, 4*TEX_H, title, NULL, share);
    if (!window) return NULL;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowPos(window, posX, posY);
    glfwShowWindow(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_btn_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetWindowSizeCallback(window, change_size_callback);

    return window;

}

int display_init() {

  int x, y, width;

  t0=get_time();
  if (!glfwInit()) return -1;

  gl_ok=1;
  printf("%9.6f, GL_init OK\n", get_time()-t0);

  window = open_window("quad", NULL, OFFSET, OFFSET);
  if (!window) { glfwTerminate(); return -1; }

  glfwGetWindowPos(window, &x, &y);
  glfwGetWindowSize(window, &width, NULL);
  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  while (!glfwWindowShouldClose(window)) {

  }

  glfwTerminate();
  printf("%9.6f, GL terminating\n", get_time()-t0);
  gl_ok=0;

  return 0;
}

int main2(int argc, char **argv) {

  pthread_t work_thread;

  if (pthread_create(&work_thread, NULL, work, NULL)) { fprintf(stderr, "couln't create a thread\n"); return -1; }

  display_init();

  if (pthread_join(work_thread, NULL)) {
  printf("pthread_join error\n"); return -2; }

  return 0;

}

int main(int argc, char **argv) {

  GLuint width = TEX_W;
  GLuint height = TEX_H;

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

  window = glfwCreateWindow(
    width, 4*height, "GLSL test", NULL, NULL);

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TEX_W, 4*TEX_H, 0, GL_RED, GL_FLOAT, buffer);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  //}

  fill(buffer, TEX_W*4*TEX_H);
  for (int j=0; j<4; j++)
  for (int r=0; r<TEX_H; r++)
  for (int c=0; c<TEX_W; c++) {
    u16 i=r*TEX_W+c;
    if (j==0) buffer8[i+j*TEX_W*TEX_H] = 128;
    if (j==1) buffer8[i+j*TEX_W*TEX_H] = 64;
    if (j==2) buffer8[i+j*TEX_W*TEX_H] = 48;
    if (j==3) buffer8[i+j*TEX_W*TEX_H] = 96;
    //buffer8[i+j*TEX_W*TEX_H] = j==0?(u8)(buffer[i+j*TEX_W*TEX_H]*255.0f) : 200.0f;
    printf("i=%d, b8=%u, b=%f\n", i+j*TEX_W*TEX_H, buffer8[i+j*TEX_W*TEX_H], buffer[i+j*TEX_W*TEX_H]);
  }
  tex_update_needed=1;

  GLuint cursor     = glGetUniformLocation(shader_prog, "cursor");
  GLuint colormap_a = glGetUniformLocation(shader_prog, "colormap_a");
  GLuint colormap_b = glGetUniformLocation(shader_prog, "colormap_b");
  GLuint colormap_c = glGetUniformLocation(shader_prog, "colormap_c");
  GLuint colormap_d = glGetUniformLocation(shader_prog, "colormap_d");

  float a[4][3] = { {0.5f, 0.5f, 0.5f }, {0.5f, 0.5f, 0.5f }, {0.0f, 0.0f, 0.0f }, {0.5f, 0.5f, 0.5f }};

  float b[4][3] = { {0.5f,  0.5f, 0.5f },{ 0.5f, 0.5f, 0.5f }, {-1.0f,-1.0f,-1.0f },{-0.5f,-0.5f,-0.5f }};

  float c[4][3] = { {1.0f, 1.0f, 1.0f }, {2.0f, 1.0f, 0.0f }, {1.0f, 1.0f, 1.0f }, {1.0f, 1.0f, 1.0f }};

  float d[4][3] = { {0.0f,0.33f,0.67f }, {0.5f, 0.2f, 0.25f}, {0.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 0.0f }};

  glfwSetCursorPosCallback(window, mouse_pos_callback);
  glfwSetMouseButtonCallback(window, mouse_btn_callback);
  glfwSetKeyCallback(window, key_callback);

  /* main loop */
  int vsync = 1;
  glfwSwapInterval(vsync);

  gl_ok=1; t0 = get_time();
  pthread_t work_thread;

  if (pthread_create(&work_thread, NULL, work, NULL)) { fprintf(stderr, "couln't create a thread\n"); return -1; }

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

void *work(void *args) {

  while (!gl_ok) usleep(10);

  byte_array palette, bytecode, polygons;
  read_array(&palette,  "file17");
  read_array(&bytecode, "file18");
  read_array(&polygons, "file19");

  u32 MAX_TICKS = 4;

  another_vm vm;
  memset(&vm, 0, sizeof vm);

  int i=0;
  while (gl_ok && i<MAX_TICKS) {
    tick(&vm, bytecode.bytes);
    tex_update_needed=1;
    usleep(10000);

    printf("[%12.6f s] work\n", get_time() - t0);
    i++;
  }

  free(palette.bytes);
  free(bytecode.bytes);
  free(polygons.bytes);
  return 0;
}
