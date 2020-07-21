#include "defs.h"
#include "bitmaps.h"
#include "polygon.h"
#include "strings.h"

u8 page_front;
u8 page_back;
u8 page_current;
u8 current_fb;

u8 buffer8[4*320*200];
u32 pal32[16];

extern float palette_rgb[48];
void set_palette(u8 *p, u8 v) {
  printf("set palette %d", v);
  int offset = 32 * v;
  for (int i=0; i<16; i++) {
    int color = p[offset+i*2] << 8 | p[offset+i*2+1];
    int r = (color >> 8) & 15;
    int g = (color >> 4) & 15;
    int b =  color       & 15;
    r = (r<<4) | r;
    g = (g<<4) | g;
    b = (b<<4) | b;
    pal32[i] = 0xff000000 | (b<<16) | (g<<8) | r;
    palette_rgb[i*3] = r/255.0;
    palette_rgb[i*3+1] = g/255.0;
    palette_rgb[i*3+2] = b/255.0;
  }
}

#include "glutils.h"

void *work(void *args);

int main(int argc, char **argv) {
  return display_init(argc, argv);
}

typedef struct {
  int pc;
  int next_pc;
  int paused;
  int ticks;
  u16 stack[256];
  u8 stack_ptr;
} task;

#define NUM_TASKS 64
#define NUM_REGS 256

typedef struct {
  int ticks;
  task tasks[NUM_TASKS];
  u8 vars[NUM_REGS];
} vm;

typedef struct {
  u8 *palette;
  u8 *bytecode;
  u8 *polygons0;
  u8 *polygons1;
} data;

void init_tasks(vm *v) {
  for (int i=0; i<NUM_TASKS; i++) {
    v->tasks[i] = (task){ -1, -1, 0, 0, {0}, 0 };
  }
  v->tasks[0].pc = 0;
}

s16 to_signed(u16 value, u16 bits) {
  u16 mask = 1 << (bits - 1);
  return value - ((value & mask) << 1);
}

void print_stack(task *t) {
  printf("stack: ");
  for (int i=t->stack_ptr; i>0; i--) {
    printf("%d: 0x%04x ", i-1, t->stack[i]);
  }
}

u8 get_page(u8 num) {
  switch (num) {
    case 0xff: return page_back; break;
    case 0xfe: return page_front; break;
    case 0x0 ... 0x3: return num; break;
    default: printf("illegal page\n"); return 0; break;
  }
}

void fill_page(u8 num, u8 color) {
  u8 pg = get_page(num);
  fill_buf(buffer8+pg*PAGE_SIZE, PAGE_SIZE, color);
}

#define F8(c, t) ((c)[(t)->pc++])
#define F16(c, t) (F8(c,t) << 8 | F8(c,t))
#define PUSH(t, v) ((t)->stack[(t)->stack_ptr++] = (v))
#define POP(t) ((t)->stack[--((t)->stack_ptr)])

/* jump table */
void *ops[256];
void unk(vm *v, task *t, data *d) { printf("unimplemented\n"); gl_ok=0; }

void movi(vm *v, task *t, data *d) {
  u8 arg0 = F8(d->bytecode, t);
  s16 imm = to_signed(F16(d->bytecode, t), 16);
  v->vars[arg0] = imm;
  printf("MOVI VAR[0x%02x] = %d", arg0, imm);
}

void call(vm *v, task *t, data *d) {
  u16 arg = F16(d->bytecode, t);
  printf("call 0x%04x ", arg);
  PUSH(t, t->pc);
  t->pc = arg;
  print_stack(t);
}

void ret(vm *v, task *t, data *d) {
  u16 prev_pc = POP(t);
  t->pc = prev_pc;
  print_stack(t);
}

void yield(vm *v, task *t, data *d) { printf("yield"); t->paused = 1; }

void select_fb(vm *v, task *t, data *d) { current_fb = get_page(F8(d->bytecode, t)); printf("select fb %d", current_fb); }

void fillpg(vm *v, task *t, data *d) { printf("fill page"); u8 num = F8(d->bytecode, t); u8 color = F8(d->bytecode, t); fill_page(num, color); }

void update_display(vm *v, task *t, data *d) {
  u8 arg0 = F8(d->bytecode, t);
  v->vars[0xf7] = 0;
  tex_update_needed=1;
  printf("update_display 0x%02x", arg0);
}

void install_task(vm *v, task *t, data *d) {
  u8 num = F8(d->bytecode, t);
  u16 off = F16(d->bytecode, t);
  v->tasks[num].next_pc = off;
  printf("install_task %d -> 0x%04x", num, off);
}

void remove_task(vm *v, task *t, data *d) {
  t->pc = -1;
  t->paused = 1;
  printf("remove task");
}
void put_pixel(u8 page, u16 x, u16 y, u8 color) {
  u16 offset = page*PAGE_SIZE + (y*SCR_W+x);
  buffer8[offset] = color;
}

void draw_bitmap(u8 id) {

  printf(", draw bitmap %u ", id);
  unsigned char *buf = bitmaps[id];
  u32 offset=0;
  for (u16 y=0; y<200; ++y) {
    for (u16 x=0; x<320; x+=8) {
      for (u8 b=0; b<8; ++b) {
        s8 mask = 1 << (7-b);
        s8 color = 0;
        for (u32 p=0; p<4; ++p) {
          if (0 != (buf[offset+p*8000] & mask)) {
            color |= (1 << p);
          }
        }
        put_pixel(0, x+b, y, color);
      }
      offset+=1;
    }
  }
}
void update_res(vm *v, task *t, data *d) {
  u16 arg = F16(d->bytecode, t);
  if (arg == 18 || arg == 19)
    draw_bitmap(arg);
  printf("update res 0x%04x", arg);
}

void play_sound(vm *v, task *t, data *d) {
  u16 arg = F16(d->bytecode, t);
  u8 arg0 = F8(d->bytecode, t);
  u8 arg1 = F8(d->bytecode, t);
  printf("play sound %04x %02x %02x", arg, arg0, arg1);
}

void play_music(vm *v, task *t, data *d) {
  u16 arg0 = F16(d->bytecode, t);
  u16 arg1 = F16(d->bytecode, t);
  u8 arg2 = F8(d->bytecode, t);
  printf("play music 0x%04x 0x%04x 0x%02x", arg0, arg1, arg2);
}

void mov(vm *v, task *t, data *d) {
  u8 dst = F8(d->bytecode, t);
  u8 src = F8(d->bytecode, t);
  printf("MOV var[%u] = var[%u]", dst, src);
  v->vars[dst] = v->vars[src];
}

void add(vm *v, task *t, data *d) {
  u8 dst = F8(d->bytecode, t);
  u8 src = F8(d->bytecode, t);
  printf("ADD var[%u] += var[%u]", dst, src);
  v->vars[dst] += v->vars[src];
}

void copy_page(vm *v, task *t, data *d) {
  u8 src = F8(d->bytecode, t);
  u8 dst = F8(d->bytecode, t);
  printf("copy page var[%u] += var[%u]", dst, src);
  /* copy page */
}

void addi(vm *v, task *t, data *d) {
  u8 n = F8(d->bytecode, t);
  s16 imm = to_signed(F16(d->bytecode, t), 16);
  printf("ADDI var[%u] += %d", n, imm);
  v->vars[n] += imm;
}

void jmp_nz(vm *v, task *t, data *d) {
  u8 n = F8(d->bytecode, t);
  v->vars[n] -= 1;
  u16 addr = F16(d->bytecode, t);
  printf("JMP NZ %u 0x%04x", v->vars[n], addr);
  if (0 != v->vars[n]) t->pc = addr;
}

void draw_char(u8 page, char chr, u8 color, int x, int y) {
  if (x < (320/8) && y < (200 - 8)) {
    for (u8 j=0; j<8; j++) {
      u8 mask = font[(chr-32) * 8 + j];
      for (u8 i=0; i<8; i++) {
        if (0 != (mask & (1 << (7-i)))) {
          put_pixel(page, x*8+i, y+j, color);
        }
      }
    }
  }
}

void draw_string(vm *v, task *t, data *d) {
  u16 num = F16(d->bytecode, t);
  u8 x = F8(d->bytecode, t);
  u8 y = F8(d->bytecode, t);
  u8 color = F8(d->bytecode, t);

  const char *s = strings_en[num];
  u8 x0 = x;
  printf("draw_string 0x%04x, x=%d, y=%d, [%s]", num, x, y, s);
  for (int i=0; i<strlen(s); i++) {
    char chr = s[i];
    if (10 == chr) { y+= 8; x = x0; }
    else { draw_char(current_page0, chr, color, x, y);
           x+=1;
    }
  }
}

void jmp_c(vm *v, task *t, data *d) {
  u8 op = F8(d->bytecode, t);
  u8 b = v->vars[F8(d->bytecode, t)];
  u8 a;
  if (op & 0x80) a = v->vars[F8(d->bytecode, t)];
  else if (op & 0x40) a = to_signed(F16(d->bytecode, t), 16);
  else a = F8(d->bytecode, t);
  printf("JMP C op=%u b=0x%02x, a=0x%02x", op, b, a);
  u16 addr = F16(d->bytecode, t);
  switch (op & 7) {
    case 0:
      if (b == a) t->pc = addr;
      break;
    case 1:
      if (b != a) t->pc = addr;
      break;
    case 2:
      if (b > a) t->pc = addr;
      break;
    case 3:
      if (b >= a) t->pc = addr;
      break;
    case 4:
      if (b > a) t->pc = addr;
      break;
    case 5:
      if (b <= a) t->pc = addr;
      break;
    default:
      break;
  }
}

void jmp(vm *v, task *t, data *d) {
  u16 off = F16(d->bytecode, t);
  t->pc = off;
  printf("JMP 0x%04x", off);
}

void set_pal(vm *v, task *t, data *d) {
  u16 arg = F16(d->bytecode, t);
  set_palette(d->palette, arg >> 8);
}

void init_ops(void(* def)(vm *v, task*, data *d)) {
  for (int i=0; i<256; i++) ops[i] = def;
  ops[0x00] = &movi;
  ops[0x01] = &mov;
  ops[0x02] = &add;
  ops[0x03] = &addi;
  ops[0x04] = &call;
  ops[0x05] = &ret;
  ops[0x06] = &yield;
  ops[0x0d] = &select_fb;
  ops[0x07] = &jmp;
  ops[0x08] = &install_task;
  ops[0x09] = &jmp_nz;
  ops[0x0a] = &jmp_c;
  ops[0x0b] = &set_pal;
  ops[0x0e] = &fillpg;
  ops[0x0f] = &copy_page;
  ops[0x10] = &update_display;
  ops[0x11] = &remove_task;
  ops[0x12] = &draw_string;
  //ops[0x18] = &play_sound;
  ops[0x19] = &update_res;
  ops[0x1a] = &play_music;
}

void draw_shape(u8 *data, u16 offset, u8 color, u8 zoom, u8 x, u8 y) {

  printf("draw_shape offset=0x%04x", offset);
}
void draw_sprite(vm *v, task *t, data *d, u8 op) {
  u16 arg = ((((u16)op) << 8 | F8(d->bytecode, t)) << 1) & 0xfffe;
  u8 arg0 = F8(d->bytecode, t);
  u8 arg1 = F8(d->bytecode, t);
  u8 arg2 = F8(d->bytecode, t);
}

void draw_poly_bkgd(vm *v, task *t, data *d, u8 op) {
  u16 arg = ((((u16)op) << 8 | F8(d->bytecode, t)) << 1) & 0xfffe;
  u8 arg0 = F8(d->bytecode, t);
  u8 arg1 = F8(d->bytecode, t);
  if ((arg1 - 199) > 0) { arg0 += arg1 - 199; arg0 = 199; }
  draw_shape(d->polygons0, arg, 0xff, 64, arg0, arg1);
}

void execute_task(vm *v, int i, data *d) {
  task *t = &(v->tasks[i]);
  v->tasks[i].ticks = 0;
  while (!v->tasks[i].paused && gl_ok) {
    u8 op = d->bytecode[t->pc++];
    printf("\n[%12.6f s, ticks %07d] task %2d tick %4d off 0x%04x op 0x%02x | ", get_time() - t0, v->ticks++, i, (t->ticks)++, t->pc-1, op);
    switch (op) {
      case 0x80 ... 0xff:
        draw_poly_bkgd(v, t, d, op);
        break;
      case 0x40 ... 0x7f:
        draw_sprite(v, t, d, op);
        break;
      default:
        ((void(*)(vm*, task*, data*))ops[op])(v, t, d);
        break;
    }
    usleep(500);
  }
}

void run_tasks(vm *v, data *d) {
  for (int i=0; i<NUM_TASKS; i++) {
    if (-1 != v->tasks[i].next_pc) {
      v->tasks[i].pc = v->tasks[i].next_pc;
      v->tasks[i].next_pc = -1;
    }
  }

  for (int i=0; i<NUM_TASKS; i++) {
    if (-1 != v->tasks[i].pc) {
      v->tasks[i].paused = 0;
      execute_task(v, i, d);
    }
  }
}

void *work(void *args) {

  while (!gl_ok) usleep(10);

  byte_array palette, bytecode, polygons;
  read_array(&palette,  "file17");
  read_array(&bytecode, "file18");
  read_array(&polygons, "./dump/poly19");
  init_bitmaps();

  int DEFAULT_TICKS = -1;
  int MAX_TICKS = args ? strtol((char*)args, NULL, 10) : DEFAULT_TICKS;

  printf("MAX_TICKS=%d\n", MAX_TICKS);
  vm vm0;
  init_tasks(&vm0);
  init_ops(&unk);
  init_strings();

  data d = {palette.bytes, bytecode.bytes, polygons.bytes};

  int i=0;
  while (gl_ok) {
    if (1==paused) { if (1!=step) {usleep(1000); continue; } else {step=0;}}
    putchar('.'); fflush(stdout);
    run_tasks(&vm0, &d);
    usleep(50000);
    if (i>=MAX_TICKS && MAX_TICKS>0) paused=1;
    i++;
  }

  free(palette.bytes);
  free(bytecode.bytes);
  free(polygons.bytes);
  return 0;
}
