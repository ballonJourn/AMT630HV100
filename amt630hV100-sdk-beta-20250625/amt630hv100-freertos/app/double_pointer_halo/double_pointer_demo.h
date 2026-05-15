#ifndef POINTER_DEMO_H
#define POINTER_DEMO_H


#ifdef __cplusplus
extern "C" {
#endif

// ¹âÔÎµ×Í¼aRGB8888
extern unsigned int* get_halo_image(void);

extern unsigned short* get_bk_image(void);

int double_pointer_halo_draw (void);

int double_pointer_halo_init (int width, int height);

int double_pointer_halo_exit (void);

#ifdef __cplusplus
}
#endif

#endif



