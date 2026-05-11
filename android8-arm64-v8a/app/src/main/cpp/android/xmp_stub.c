#include "xmp.h"
#include <string.h>

xmp_context xmp_create_context(void) { return (xmp_context)0x1; }
void xmp_free_context(xmp_context ctx) { (void)ctx; }
int xmp_set_player(xmp_context ctx, int param, int value) { (void)ctx; (void)param; (void)value; return 0; }
int xmp_load_module_from_memory(xmp_context ctx, const void *mem, long size) { (void)ctx; (void)mem; (void)size; return -1; }
int xmp_start_player(xmp_context ctx, int rate, int flags) { (void)ctx; (void)rate; (void)flags; return -1; }
void xmp_end_player(xmp_context ctx) { (void)ctx; }
void xmp_release_module(xmp_context ctx) { (void)ctx; }
int xmp_set_position(xmp_context ctx, int pos) { (void)ctx; (void)pos; return 0; }
int xmp_play_buffer(xmp_context ctx, void *buffer, int size, int loop) { (void)ctx; (void)loop; if (buffer && size > 0) memset(buffer, 0, (size_t)size); return -1; }
