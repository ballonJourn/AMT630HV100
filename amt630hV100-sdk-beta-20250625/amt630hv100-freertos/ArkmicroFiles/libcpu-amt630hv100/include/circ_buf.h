#ifndef _CIRC_BUF_H
#define _CIRC_BUF_H

#ifdef __cplusplus
extern "C" {
#endif

struct circ_buf {
	char *buf;
	int head;
	int tail;
};

/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
static __INLINE int CIRC_CNT_TO_END(uint32_t head, uint32_t tail, uint32_t size)
{
	int end = (size) - (tail);
	int n = ((head) + end) & ((size)-1);
	
	return n < end ? n : end;
}

/* Return space available up to the end of the buffer.  */
static __INLINE int CIRC_SPACE_TO_END(uint32_t head, uint32_t tail, uint32_t size)
{
	int end = (size) - 1 - (head);
	int n = (end + (tail)) & ((size) - 1);

	return n <= end ? n : end + 1;
}
	  
#ifdef __cplusplus
}
#endif	  

#endif /* _LINUX_CIRC_BUF_H  */
