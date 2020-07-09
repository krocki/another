#include "defs.h"

u8 f8(u8 *bytes) {
  return bytes[0];
}

u16 f16(u8 *bytes) {
  return (u16)(bytes[0] << 8) | bytes[1];
}

void draw_shape(u8 p, u8 o, u8 op, u8 zoom, u8 x, u8 y) {
  printf("draw_shape 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", p, o, op, zoom, x, y);
}

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("usage: %s filename\n", argv[0]);
    return 0;
  }

  byte_array bytecode;
  read_array(&bytecode, argv[1]);
  u8 *bytes = bytecode.bytes;

  for (int i=0; i<bytecode.n; i++) {
    u8 v = bytes[i];
    u16 arg16, arg8;
    u8 zoom=0, polygons=0, x=0, y=0;
    printf("0x%04x: ", i);
    switch(v) {
      case 0x0:
        arg8 = bytes[i+1];
        arg16 = (bytes[i+2] << 8) | bytes[i+3];
        printf("VAR[0x%02x] = %3d\n", arg8, arg16);
        i+=3; break;
      case 0x1:
        printf("var(...) = var(...)\n");
        i+=2; break;
      case 0x3:
        arg8 = bytes[i+1];
        arg16 = (bytes[i+2] << 8) | bytes[i+3];
        printf("VAR[0x%02x] += %3d\n", arg8, arg16);
        i+=3; break;
      case 0x4:
        arg16 = (bytes[i+1] << 8) | bytes[i+2];
        printf("call [0x%04x]\n", arg16);
        i+=2; break;
      case 0x5:
        printf("return from call\n"); break;
      case 0x6:
        printf("yield\n"); break;
      case 0x7:
        arg16 = f16(&bytes[i+1]);
        printf("JUMP 0x%04x\n", arg16);
        i+=2; break;
      case 0x8:
        arg8 = bytes[i+1];
        arg16 = (bytes[i+2] << 8) | bytes[i+3];
        printf("install task[%2d] = 0x%04x\n", arg8, arg16);
        i+=3; break;
      case 0x9:
        arg8 = bytes[i+1];
        arg16 = (bytes[i+2] << 8) | bytes[i+3];
        printf("jmp if var(var[0x%02x] = 0x%04x\n", arg8, arg16);
        i+=3; break;
      case 0xa:
        printf("jmpif(var(..), @....)\n");
        i+=5; break;
      case 0xb:
        arg16 = f16(bytes+i+1);
        printf("set palette [%3d]\n", arg16);
        i+=2; break;
      case 0xc:
        printf("change tasks state [0x%02x, 0x%02x 0x%02x]\n", bytes[i+1], bytes[i+2], bytes[i+3]);
        i+=3; break;
      case 0xd:
        arg8 = f8(bytes+i+1);
        printf("select page [%2d]\n", arg8);
        i++; break;
      case 0xe:
        arg16 = f16(bytes+i+1);
        printf("fill page 0x%02x%02x\n",
          arg16<<8, arg16);
        i+=2; break;
      case 0xf:
        arg16 = f16(bytes+i+1);
        printf("copy page 0x%02x%02x\n",
          arg16<<8, arg16);
        i+=2; break;
      case 0x10:
        arg8 = f8(bytes+i+1);
        printf("update display %d\n", arg8);
        i++; break;
      case 0x11:
        printf("remove task\n"); break;
      case 0x12:
        printf("draw string 0x%02x%02x%02x%02x%02x\n",
          bytes[i+1], bytes[i+2], bytes[i+3],
          bytes[i+4], bytes[i+5]);
        i+=5; break;
      case 0x18:
        printf("play sound\n");
        i+=5; break;
      case 0x19:
        arg16 = (bytes[i+1] << 8) | bytes[i+2];
        printf("update resources [%3d]\n", arg16);
        i+=2; break;
      case 0x1a:
        printf("play music 0x%02x%02x%02x%02x%02x\n",
          bytes[i+1], bytes[i+2], bytes[i+3],
          bytes[i+4], bytes[i+5]);
        i+=5; break;
      case 0x40 ... 0x7f:
        printf("DRAW_POLY_SPRITE opcode %02x\n", v);
        u16 offset = (f16(bytes+1+i) << 1) & 0xfffe;
        x = f8(bytes+3+i);
        i+=3;
        if (0 == (v & 0x20)) {
          if (0 == (v & 0x10)) {
            x = (x << 8) | f8(bytes+1+i);
            i++;
          } else { /* x = vars(x) */ }
        } else {
          if (v & 0x10) {
            x += 256;
          }
        }
        y=f8(bytes+i+i); i++;
        if (0 == (v & 0x8)) {
          if (0 == (v & 0x4)) {
            y = (y << 8) | f8(bytes+i+1); i++;
          } else { }
        }
        if (0 == (v & 0x2)) {
          if (v & 0x1) {
            zoom = f8(bytes+i+1); i++;
          }
        } else {
          if (v & 0x1) {
            /* .... */
          } else {
            zoom = f8(bytes+i+1); i++;
          }
        }
        draw_shape(polygons, offset, 0xff, zoom, x, y);
        break;
      case 0x80 ... 0xff:
        printf("DRAW_POLY_BACKGROUND %02x 0x%02x%02x%02x\n",
          bytes[i], bytes[i+1], bytes[i+2], bytes[i+3]);
        draw_shape(polygons, offset, 0xff, 64, x, y);
        i+=3; break;
      default:
        printf("unk 0x%04x\n", v);
        getchar();
        break;
    }
  }

  free(bytecode.bytes);
  return 0;
}
