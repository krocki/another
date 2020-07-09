#include "defs.h"

#define SCALE 1
#define SCR_W 320
#define SCR_H 200

typedef struct {
  int id;
} vertex;

typedef struct {
  u8 regs[256];
  u16 pc;
  u32 ticks;
} another_vm;

#define F8 (code[vm->pc++])
void tick(another_vm *vm, u8 *code) {

  printf("[tick: %u, pc=0x%04x] ", vm->ticks, vm->pc);
  u8 op = F8;
  printf("op=0x%02x ", op);
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

int main(int argc, char **argv) {

  byte_array palette, bytecode, polygons;
  read_array(&palette,  "file17");
  read_array(&bytecode, "file18");
  read_array(&polygons, "file19");
  
  u32 MAX_TICKS = 2;

  another_vm vm;
  memset(&vm, 0, sizeof vm);

  for (u32 i=0; i<MAX_TICKS; i++)
    tick(&vm, bytecode.bytes);

  free(palette.bytes);
  free(bytecode.bytes);
  free(polygons.bytes);

  return 0;
}
