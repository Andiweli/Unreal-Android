#ifndef UE1_ANDROID_XMP_STUB_H
#define UE1_ANDROID_XMP_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* xmp_context;

#define XMP_INTERP_NEAREST 0
#define XMP_INTERP_LINEAR  1
#define XMP_INTERP_SPLINE  2
#define XMP_PLAYER_INTERP  1

xmp_context xmp_create_context(void);
void xmp_free_context(xmp_context ctx);
int xmp_set_player(xmp_context ctx, int param, int value);
int xmp_load_module_from_memory(xmp_context ctx, const void *mem, long size);
int xmp_start_player(xmp_context ctx, int rate, int flags);
void xmp_end_player(xmp_context ctx);
void xmp_release_module(xmp_context ctx);
int xmp_set_position(xmp_context ctx, int pos);
int xmp_play_buffer(xmp_context ctx, void *buffer, int size, int loop);

#ifdef __cplusplus
}
#endif
#endif
