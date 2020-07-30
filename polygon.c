#include "defs.h"
#include "polygon.h"

int num_poly = 0;

void draw_point(u8 pg, u8 color, int x, int y) {
  if (x < 0 || x >= SCR_W || y<0 || y >= SCR_H) return;
  u32 offset = pg*PAGE_SIZE+y*SCR_W+x;
  if (0x11 == color) buffer8[offset] = buffer8[y*SCR_W + x];
  else if (0x10 == color) buffer8[offset] |= 8;
  else buffer8[offset] = color;
  //buffer8[x+SCR_W*y+pg*PAGE_SIZE] = color;
}

void draw_line(int pg, u8 color, int y, int x1, int x2) {

  //printf("draw line pg=%d, col=%d, y=%d, x1=%d, x2=%d\n", pg, color, y, x1, x2);
  if (x1 > x2) { int tmp=x1; x1=x2; x2=tmp; }
  if (x1 >= SCR_W || x2 < 0) return;
  if (x1 < 0) x1=0;
  if (x2 >= SCR_W) x2 = SCR_W - 1;
  int offset = pg * PAGE_SIZE + y*SCR_W;
  if (color == 0x11) {
    //assert pg != 0
    for (int j=x1; j<x2; j++)
      buffer8[offset+j] = 1;
  } else if (color == 0x10) {
    for (int i=x1; i<x2; i++)
      buffer8[offset+i] |= 8;
  }
  else {
    for (int i=offset+x1; i<offset+x2+1; i++)
      buffer8[i] = color;
  }
}

void draw_polygon(u8 pg, u8 color, vertex *v, u8 n) {

  int i=0;
  int j=n-1;
  int a = v[i].y; int b = v[j].y;
  int scanline = a < b ? a : b;
  int f2 = v[i++].x << 16;
  int f1 = v[j--].x << 16;

  for (int c=n-2; c!=0; c-=2) {
    int h1 = v[j].y - v[j+1].y;
    int step1 = ((v[j].x - v[j+1].x) << 16) / (0==h1?1:h1);
    j-=1;
    int h2 = v[i].y - v[i-1].y;
    int step2 = ((v[i].x - v[i-1].x) << 16) / (0==h2?1:h2);
    i+=1;
    f1 = (f1 & 0xffff0000) | 0x7fff;
    f2 = (f2 & 0xffff0000) | 0x8000;
    if (0==h2) {
      f1 += step1; f2 += step2;
    } else {
      for (int k=0; k<h2; ++k) {
        if (scanline >= 0) {
          draw_line(pg, color, scanline, f1 >> 16, f2 >> 16);
        }
        f1 += step1; f2 += step2; scanline += 1;
        if (scanline >= SCR_H) return;
      }
    }
  }
}

void fill_polygon(u8 *data, u32 offset, u8 color, u8 zoom, u16 x, u16 y) {

  u16 w = (data[offset++] * zoom/64);
  u16 h = (data[offset++] * zoom/64);

  int x1 = (x * SCALE - w * SCALE/2 - 1);
  int x2 = (x * SCALE + w * SCALE/2 - 1);
  int y1 = (y * SCALE - h * SCALE/2 );
  int y2 = (y * SCALE + h * SCALE/2 );

  if ( x1 >= SCR_W || x2 < 0 || y1 >= SCR_H || y2 < 0)
    return;

  u8 count = data[offset++];
  printf("poly w=%d h=%d x=%d y=%d x1=%d, y1=%d, vcnt=%d poly #=%d\n", w, h, x, y, x1, y1, count, num_poly++);

  //draw_point(1, color, x, y);

  for (u16 i=0; i<count; i++) {
    int vx = x1 + ((data[offset++] * zoom/64)) * SCALE;
    int vy = y1 + ((data[offset++] * zoom/64)) * SCALE;
    vertices[i] = (vertex){.id=i, .x=vx, .y=vy};
    printf("vertex %d: vx %d, vy %d\n", i, vx, vy);
  }
  if (4 == count && 0 == w & h <=1 ) {
    draw_point(current_page0, color, x1, y1);
  } else {
    draw_polygon(current_page0, color, vertices, count);
    tex_update_needed=1;
  }
}

void draw_shape_parts(u8 *data, u32 offset, u8 zoom, u16 x, u16 y) {

  u16 x0 = x - ( data[offset++] * zoom / 64 );
  u16 y0 = y - ( data[offset++] * zoom / 64 );
  u8 count = data[offset++];

  for (u16 i=0; i<=count; i++) {
    u32 addr = ( data[offset] << 8 ) | data[offset+1];
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
void draw_shape(u8 *data, u32 offset, u8 color, u8 zoom, u16 x, u16 y) {

  printf("draw_shape offset=0x%04x ", offset);
  u8 code = data[offset++];
  printf("code=0x%04x %02x\n", code, data[0]);

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

