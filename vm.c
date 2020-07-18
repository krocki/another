#include "defs.h"
#include "bitmaps.h"
#include "polygon.h"

u8 buffer8[4*320*200];
#include "glutils.h"

void *work(void *args);

int main(int argc, char **argv) {
  return display_init(argc, argv);
}

typedef struct {
  int pc;
  int paused;
  int ticks;
} task;

#define NUM_TASKS 64

typedef struct {
  int ticks;
  task tasks[NUM_TASKS];
} vm;

void init_tasks(vm *v) {
  for (int i=0; i<NUM_TASKS; i++) {
    v->tasks[i] = (task){ -1, 0, 0 };
  }
  v->tasks[0].pc = 0;
}

void *ops[256];
void unk(task *t, u8 *p, u8 *c, u8 *poly) { printf("unimplemented"); }
void yield(task *t, u8 *p, u8 *c, u8 *poly) { printf("yield"); t->paused = 1; }

void init_ops(void(* def)(task*,u8*,u8*,u8*)) {
  for (int i=0; i<256; i++) ops[i] = def;
  ops[0x06] = &yield;
}

void execute_task(vm *v, int i, u8 *pal, u8 *code, u8 *poly) {
  task *t = &(v->tasks[i]);
  while (!v->tasks[i].paused && gl_ok) {
    u8 op = code[t->pc++];
    printf("\n[%12.6f s] task %d tick %d off 0x%04x op 0x%02x | ", get_time() - t0, i, (t->ticks)++, t->pc-1, op);
    ((void(*)(task*,u8*,u8*,u8*))ops[op])(t, pal, code, poly);
    usleep(100000);
  }
}

void run_tasks(vm *v, u8 *pal, u8 *code, u8 *poly) {
  for (int i=0; i<NUM_TASKS; i++) {
    if (-1 != v->tasks[i].pc) {
      execute_task(v, i, pal, code, poly);
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

  int i=0;
  while (gl_ok) {
    if (1==paused) { if (1!=step) {usleep(1000); continue; } else {step=0;}}
    putchar('.'); fflush(stdout);
    run_tasks(&vm0, palette.bytes, bytecode.bytes, polygons.bytes);
    usleep(100000);
    if (i>=MAX_TICKS && MAX_TICKS>0) paused=1;
    i++;
  }

  free(palette.bytes);
  free(bytecode.bytes);
  free(polygons.bytes);
  return 0;
}
