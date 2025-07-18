#include <stdio.h>
#include "chip.h"
#include "board.h"


#ifdef TP_SUPPORT
extern int goodix_init(void);
extern int gt675x_init(void);
extern int ft6336u_init(void);

int tp_init(void)
{
#if defined(TP_USE_GT9XX)
	printf("TP select gt9xx.\n");
	return goodix_init();
#elif defined(TP_USE_GA657X)
	printf("TP select ga657x.\n");
	return gt675x_init();
#elif defined(TP_USE_FT6336U)
	printf("TP select ft6336u.\n");
	return ft6336u_init();
#else
	#error "Please select one tp IC"
#endif

	return -1;
}
#endif
