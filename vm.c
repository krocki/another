#include "defs.h"
#include "bitmaps.h"
#include "polygon.h"

typedef struct {
  s16 regs[256];
  u16 tasks[64];
  u16 stack[256];
  u8 stack_ptr;
  u16 pc;
  u32 ticks;
  u8 current_fb;
  u8 task_paused;
} another_vm;

u32 pal32[16];

u8 page_front;
u8 page_back;
u8 page_current;

u8 buffer8[PAGE_SIZE * 4];
u32 buffer32[PAGE_SIZE * 4];

void draw_shape(u8 *p, u16 offset, u8 color, u8 zoom, u8 x, u8 y);

void put_pixel(u8 page, u16 x, u16 y, u8 color) {
  u16 offset = page*PAGE_SIZE + (y*SCR_W+x);
  buffer8[offset] = color;
  buffer32[offset] = pal32[color];
}

void draw_bitmap(u8 id) {

  printf("draw bitmap %u\n", id);
  unsigned char *buf = bitmaps[id];
  printf("%s\n", bitmaps[id]);
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

#include "glutils.h"

void *work(void *args);

u8 get_page(u8 num) {
  switch (num) {
    case 0xff: return page_back; break;
    case 0xfe: return page_front; break;
    case 0x0 ... 0x3: return num; break;
    default: printf("illegal page\n"); return 0; break;
  }
}

void fill_page(u8 num, u8 color) {
  printf("fill page(page=0x%02x, color=0x%02x)\n", num, color);
  u8 pg = get_page(num);
  fill_buf(buffer8+pg*PAGE_SIZE, PAGE_SIZE, color);
}

void update_display(u8 num) {
  printf("update display(num=0x%02x)\n", num);
  /* set palette */
}

s16 to_signed(u16 value, u16 bits) {
  u16 mask = 1 << (bits - 1);
  return value - ((value & mask) << 1);
}

extern float palette_rgb[48];
void set_palette(u8 *p, u8 v) {
  printf("set palette %d\n", v);
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

#define F8  (code[vm->pc++])
#define F16 (F8 << 8 | F8)

void tick(another_vm *vm, u8 *pal, u8 *code, u8 *polygons1, u8 *polygons2) {

  printf("[tick: %u, pc=0x%04x] ", vm->ticks, vm->pc);
  u8 op = F8;
  printf("op=0x%02x ", op);

  sprintf(debug_msg, "[%u, pc=0x%04x, op=0x%02x]", vm->ticks, vm->pc, op);
  u8 arg0, arg1;
  u16 arg;

  switch (op) {
    case 0x80 ... 0xff:
      printf("DRAW_POLY_BACKGROUND\n");
      arg = ((((u16)op) << 8 | F8) << 1) & 0xfffe;
      arg0 = F8; arg1 = F8;
      if ((arg1 - 199) > 0) { arg0 += arg1 - 199; arg0 = 199; }
      draw_shape(polygons1, arg, 0xff, 64, arg0, arg1);
      break;
    case 0x40 ... 0x7f:
      printf("DRAW_POLY_SPRITE\n");
      break;
    case 0x20 ... 0x3f:
      printf("ILLEGAL OP\n");
      break;
    case 0x00 ... 0x1f:
      switch (op) {
        case 0x0:
          // SET VAR
          arg0 = F8;
          s16 imm = to_signed(F8 << 8 | F8, 16);
          vm->regs[arg0] = imm;
          printf("VAR[0x%02x] = %d\n", arg0, imm);
          break;
        case 0x4:
          // CALL
          arg = F16;
          printf("CALL 0x%04x\n", arg);
          vm->stack[vm->stack_ptr++] = vm->pc;
          printf("stack[%04x %04x %04x %04x]\n",
          vm->stack[0], vm->stack[1], vm->stack[2], vm->stack[3]);
          vm->pc = arg;
          break;
        case 0x5:
          // RETURN
          vm->pc = vm->stack[vm->stack_ptr-1]; vm->stack_ptr--;
          printf("RETURN 0x%04x\n", vm->pc);
          break;
        case 0x6:
          // yield
          vm->task_paused = 1;
          printf("YIELD TASK\n");
          break;
        case 0x7:
          // JMP word
          arg = F16;
          printf("JUMP 0x%04x\n", arg);
          vm->pc = arg;
          break;
        case 0x8:
          // INSTALL task
          arg0 = F8;
          arg = F8 << 8 | F8;
          vm->tasks[arg0] = arg;
          printf("install task [%u] = 0x%04x\n", arg0, arg);
          break;
        case 0xb:
          // SET PALETTE
          arg = F16;
          set_palette(pal, arg >> 8);
          break;
        case 0xd:
          // SELECT fb
          arg0 = F8;
          printf("SELECT FB (%02x), prev=%02x\n", arg0, vm->current_fb);
          vm->current_fb = get_page(arg0);
          break;
        case 0xe:
          // FILL page
          arg0 = F8; arg1 = F8;
          fill_page(arg0, arg1);
          break;
        case 0x10:
          // update display
          arg0 = F8;
          vm->regs[0xf7] = 0;
          update_display(arg0);
          break;
        case 0x19:
          // load resources
          arg = F8 << 8 | F8;
          printf("load resources %d\n", arg);
          if (arg == 18 || arg == 19)
            draw_bitmap(arg);
          break;
        case 0x1a:
          printf("PLAY MUSIC 0x%04x 0x%04x 0x%02x\n", F8 << 8 | F8, F8 << 8 | F8, F8);
          break;
        default:
          printf("NOT IMPLEMENTED!\n");
          break;
      }
      break;
  }
  vm->ticks++;
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
  printf("draw_shape code=0x%04x %02x\n", code, data[0]);
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

int main(int argc, char **argv) {
  return display_init(argc, argv);
  //return display_init(argc, argv);
}

void *work(void *args) {

  while (!gl_ok) usleep(10);

  byte_array palette, bytecode, polygons;
  read_array(&palette,  "file17");
  read_array(&bytecode, "file18");
  read_array(&polygons, "./dump/poly19");
  init_bitmaps();

  u32 DEFAULT_TICKS = 50;
  u32 ticks = args ? strtoul((char*)args, NULL, 10) : DEFAULT_TICKS;
  u32 MAX_TICKS = ticks;

  printf("MAX_TICKS=%u\n", MAX_TICKS);

  another_vm vm;
  memset(&vm, 0, sizeof vm);

  int i=0;
  while (gl_ok) {
    if (1==paused) { if (1!=step) {usleep(1000); continue; } else {step=0;}}
    printf("[%12.6f s] ", get_time() - t0);
    tick(&vm, palette.bytes, bytecode.bytes, polygons.bytes, NULL);
    tex_update_needed=1;
    usleep(10000);
    if (i>=MAX_TICKS) paused=1;
    i++;
  }

  free(palette.bytes);
  free(bytecode.bytes);
  free(polygons.bytes);
  return 0;
}
