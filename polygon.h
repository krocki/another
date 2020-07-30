#include "defs.h"

extern int tex_update_needed;
extern u8 current_page0;
extern u8 current_page1;
extern u8 current_page2;
extern u8 buffer8[4*320*200];
extern u32 pal32[16];

typedef struct {
  int id;
  int x, y;
} vertex;

#define MAX_VERTICES 256
vertex vertices[MAX_VERTICES];

extern int num_poly;
extern void draw_point(u8 pg, u8 color, int x, int y);
extern void draw_line(int pg, u8 color, int y, int x1, int x2);
extern void draw_polygon(u8 pg, u8 color, vertex *v, u8 n);
extern void fill_polygon(u8 *data, u32 offset, u8 color, u8 zoom, u16 x, u16 y);
extern void draw_shape_parts(u8 *data, u32 offset, u8 zoom, u16 x, u16 y);
extern void draw_shape(u8 *data, u32 offset, u8 color, u8 zoom, u16 x, u16 y);
