#ifndef _FB_QUEUE_H_
#define _FB_QUEUE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

typedef struct _queue_s {
	struct _queue_s			*prev;
	struct _queue_s			*next;
} queue_s;

typedef struct _fb_queue_s {
	void				*prev;
	void				*next;
	unsigned int	fb_base;
} fb_queue_s;

/* queue toolbox procedure */
void			queue_initialize		(queue_s *queue);
void			queue_insert			(queue_s *entry, queue_s *queue);
void			queue_delete			(queue_s *entry);
queue_s *	queue_delete_next		(queue_s *queue);
int			queue_empty				(queue_s *queue);
queue_s *	queue_head				(queue_s *queue);
queue_s *	queue_tail				(queue_s *queue);
queue_s *	queue_next				(queue_s *queue);
queue_s *	queue_prev				(queue_s *queue);

void fb_queue_init (void);
void fb_queue_exit (void);
fb_queue_s *fb_queue_get_free_unit(void);
void fb_queue_set_free (fb_queue_s *unit);
void fb_queue_set_ready (fb_queue_s *unit);
fb_queue_s *fb_queue_get_ready_unit(void);
fb_queue_s *fb_queue_get_unit_from_base(unsigned int base);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	//_FB_QUEUE_H_