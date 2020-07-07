#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t  u8;
typedef uint16_t u16;

u8 f8(u8 *bytes) {
  return bytes[0];
}

u16 f16(u8 *bytes) {
  return (u16)(bytes[0] << 8) | bytes[1];
}

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("usage: %s filename\n", argv[0]);
    return 0;
  }

  FILE *f = fopen(argv[1], "rb");

  if (NULL == f) {
    fprintf(stderr, "couldn't open %s\n", argv[1]);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  int n = ftell(f);
  rewind(f);

  printf("file=%s, size=%d\n", argv[1], n);

  u8 *bytes = malloc(n * sizeof(u8));
  fread(bytes, sizeof(u8), n, f);

  for (int i=0; i<n; i++) {
    u8 v = bytes[i];
    u16 arg16, arg8;
    printf("0x%04x: ", i);
    switch(v) {
      case 0x0:
        arg8 = bytes[i+1];
        arg16 = (bytes[i+2] << 8) | bytes[i+3];
        printf("VAR[0x%02x] = %3d\n", arg8, arg16);
        i+=3; break;
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
        printf("DRAW_POLY_SPRITE %02x 0x%02x%02x%02x%02x%02x\n",
          bytes[i], bytes[i+1], bytes[i+2], bytes[i+3], bytes[i+4], bytes[i+5]);
        i+=5; break;
      case 0x80 ... 0xff:
        printf("DRAW_POLY_BACKGROUND %02x 0x%02x%02x%02x\n",
          bytes[i], bytes[i+1], bytes[i+2], bytes[i+3]);
        i+=3; break;
      default:
        printf("unk 0x%04x\n", v);
        getchar();
        break;
    }
  }

  fclose(f);
  free(bytes);
  return 0;

}
