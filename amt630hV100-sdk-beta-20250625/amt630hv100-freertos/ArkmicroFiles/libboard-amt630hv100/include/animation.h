#ifndef _ANIMATION_H
#define _ANIMATION_H

typedef struct {
	unsigned int magic;
	int hasBootlogo;
	int bootlogoDisplayTime;
	int aniCount;
	int aniWidth;
	int aniHeight;
	int aniFps;
	int aniDelayHideTime;
	unsigned int aniSize;
	unsigned int checksum;
	unsigned int reserved[2];
}BANIHEADER;

int animation_init(void);
void animation_start(void);
/* replay animation even if animation is already playing */
void animation_restart(void);
void animation_stop(void);
int get_animation_status(void);

#endif
