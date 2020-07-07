#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t  u8;
typedef uint16_t u16;

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

  for (int i=0; i<32; i++) {
    u8 v = bytes[i];
    u16 arg16, arg8;
    printf("0x%04x: ", i);
    switch(v) {
      case 0x00:
        arg8 = bytes[i+1];
        arg16 = (bytes[i+2] << 8) | bytes[i+3];
        printf("VAR[0x%02x] = 0x%04x\n", arg8, arg16);
        i+=3; break;
      case 0x04:
        arg16 = (bytes[i+1] << 8) | bytes[i+2];
        printf("call [0x%04x]\n", arg16);
        i+=2; break;
      case 0x06:
        printf("yield\n");
        break;
      case 0x08:
        arg8 = bytes[i+1];
        arg16 = (bytes[i+2] << 8) | bytes[i+3];
        printf("install task[0x%02x] = 0x%04x\n", arg8, arg16);
        i+=3; break;
      case 0x1a:
        printf("play music 0x%02x%02x%02x%02x%02x\n",
          bytes[i+1], bytes[i+2], bytes[i+3],
          bytes[i+4], bytes[i+5]);
        i+=5; break;
      default:
        printf("unk 0x%04x\n", v);
        break;
    }
  }

  fclose(f);
  free(bytes);
  return 0;

}
