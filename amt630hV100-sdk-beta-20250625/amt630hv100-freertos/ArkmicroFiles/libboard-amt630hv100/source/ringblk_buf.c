#include <FreeRTOS.h>
#include "list.h"
#include "ringblk_buf.h"

/**
 * ring block buffer object initialization
 *
 * @param rbb ring block buffer object
 * @param buf buffer
 * @param buf_size buffer size
 * @param block_set block set
 * @param blk_max_num max block number
 *
 * @note When your application need align access, please make the buffer address is aligned.
 */
void rbb_init(rbb_t rbb, uint8_t *buf, size_t buf_size, rbb_blk_t block_set, size_t blk_max_num)
{
    size_t i;

    configASSERT(rbb);
    configASSERT(buf);
    configASSERT(block_set);

    rbb->buf = buf;
    rbb->buf_size = buf_size;
    rbb->blk_set = block_set;
    rbb->blk_max_num = blk_max_num;
    vListInitialise(&rbb->blk_list);
    /* initialize block status */
    for (i = 0; i < blk_max_num; i++)
    {
        block_set[i].status = RBB_BLK_UNUSED;
    }
}

/**
 * ring block buffer object create
 *
 * @param buf_size buffer size
 * @param blk_max_num max block number
 *
 * @return != NULL: ring block buffer object
 *            NULL: create failed
 */
rbb_t rbb_create(size_t buf_size, size_t blk_max_num)
{
    rbb_t rbb = NULL;
    uint8_t *buf;
    rbb_blk_t blk_set;

    rbb = (rbb_t)pvPortMalloc(sizeof(struct rbb));
    if (!rbb)
    {
        return NULL;
    }

    buf = (uint8_t *)pvPortMalloc(buf_size);
    if (!buf)
    {
        vPortFree(rbb);
        return NULL;
    }

    blk_set = (rbb_blk_t)pvPortMalloc(sizeof(struct rbb_blk) * blk_max_num);
    if (!blk_set)
    {
        vPortFree(buf);
        vPortFree(rbb);
        return NULL;
    }

    rbb_init(rbb, buf, buf_size, blk_set, blk_max_num);

    return rbb;
}

/**
 * ring block buffer object destroy
 *
 * @param rbb ring block buffer object
 */
void rbb_destroy(rbb_t rbb)
{
    configASSERT(rbb);

    vPortFree(rbb->buf);
    vPortFree(rbb->blk_set);
    vPortFree(rbb);

}

static rbb_blk_t find_empty_blk_in_set(rbb_t rbb)
{
    size_t i;

    configASSERT(rbb);

    for (i = 0; i < rbb->blk_max_num; i ++)
    {
        if (rbb->blk_set[i].status == RBB_BLK_UNUSED)
        {
            return &rbb->blk_set[i];
        }
    }

    return NULL;
}

/**
 * Allocate a block by given size. The block will add to blk_list when allocate success.
 *
 * @param rbb ring block buffer object
 * @param blk_size block size
 *
 * @note When your application need align access, please make the blk_szie is aligned.
 *
 * @return != NULL: allocated block
 *            NULL: allocate failed
 */
rbb_blk_t rbb_blk_alloc(rbb_t rbb, size_t blk_size)
{
	UBaseType_t uxSavedInterruptStatus;
    size_t empty1 = 0, empty2 = 0;
    rbb_blk_t head, tail, new_rbb = NULL;

    configASSERT(rbb);
    configASSERT(blk_size < (1L << 24));

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    new_rbb = find_empty_blk_in_set(rbb);

    if (listCURRENT_LIST_LENGTH(&rbb->blk_list) < rbb->blk_max_num && new_rbb)
    {
        if (listCURRENT_LIST_LENGTH(&rbb->blk_list) > 0)
        {
            head = listGET_OWNER_OF_HEAD_ENTRY(&rbb->blk_list);
            tail = listGET_LIST_ITEM_OWNER(rbb->blk_list.xListEnd.pxPrevious);
            if (head->buf <= tail->buf)
            {
                /**
                 *                      head                     tail
                 * +--------------------------------------+-----------------+------------------+
                 * |      empty2     | block1 |   block2  |      block3     |       empty1     |
                 * +--------------------------------------+-----------------+------------------+
                 *                            rbb->buf
                 */
                empty1 = (rbb->buf + rbb->buf_size) - (tail->buf + tail->size);
                empty2 = head->buf - rbb->buf;

                if (empty1 >= blk_size)
                {
                	listSET_LIST_ITEM_OWNER(&new_rbb->list, new_rbb);
                    vListInsertEnd(&rbb->blk_list, &new_rbb->list);
                    new_rbb->status = RBB_BLK_INITED;
                    new_rbb->buf = tail->buf + tail->size;
                    new_rbb->size = blk_size;
                }
                else if (empty2 >= blk_size)
                {
					listSET_LIST_ITEM_OWNER(&new_rbb->list, new_rbb);
                    vListInsertEnd(&rbb->blk_list, &new_rbb->list);
                    new_rbb->status = RBB_BLK_INITED;
                    new_rbb->buf = rbb->buf;
                    new_rbb->size = blk_size;
                }
                else
                {
                    /* no space */
                    new_rbb = NULL;
                }
            }
            else
            {
                /**
                 *        tail                                              head
                 * +----------------+-------------------------------------+--------+-----------+
                 * |     block3     |                empty1               | block1 |  block2   |
                 * +----------------+-------------------------------------+--------+-----------+
                 *                            rbb->buf
                 */
                empty1 = head->buf - (tail->buf + tail->size);

                if (empty1 >= blk_size)
                {
                	listSET_LIST_ITEM_OWNER(&new_rbb->list, new_rbb);
                    vListInsertEnd(&rbb->blk_list, &new_rbb->list);					
                    new_rbb->status = RBB_BLK_INITED;
                    new_rbb->buf = tail->buf + tail->size;
                    new_rbb->size = blk_size;
                }
                else
                {
                    /* no space */
                    new_rbb = NULL;
                }
            }
        }
        else
        {
            /* the list is empty */
            listSET_LIST_ITEM_OWNER(&new_rbb->list, new_rbb);
            vListInsertEnd(&rbb->blk_list, &new_rbb->list);			
            new_rbb->status = RBB_BLK_INITED;
            new_rbb->buf = rbb->buf;
            new_rbb->size = blk_size;
        }
    }
    else
    {
        new_rbb = NULL;
    }

    portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

    return new_rbb;
}

/**
 * put a block to ring block buffer object
 *
 * @param block the block
 */
void rbb_blk_put(rbb_blk_t block)
{
    configASSERT(block);
    configASSERT(block->status == RBB_BLK_INITED);

    block->status = RBB_BLK_PUT;
}

/**
 * get a block from the ring block buffer object
 *
 * @param rbb ring block buffer object
 *
 * @return != NULL: block
 *            NULL: get failed
 */
rbb_blk_t rbb_blk_get(rbb_t rbb)
{
	UBaseType_t uxSavedInterruptStatus;
    rbb_blk_t block = NULL;
    ListItem_t *node;

    configASSERT(rbb);

    if (listLIST_IS_EMPTY(&rbb->blk_list))
        return 0;

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    for (node = listGET_HEAD_ENTRY(&rbb->blk_list); node != listGET_END_MARKER(&rbb->blk_list);
			node = listGET_NEXT(node))
    {
        block = listGET_LIST_ITEM_OWNER(node);
        if (block->status == RBB_BLK_PUT)
        {
            block->status = RBB_BLK_GET;
            goto __exit;
        }
    }
    /* not found */
    block = NULL;

__exit:

    portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

    return block;
}

/**
 * return the block size
 *
 * @param block the block
 *
 * @return block size
 */
size_t rbb_blk_size(rbb_blk_t block)
{
    configASSERT(block);

    return block->size;
}

/**
 * return the block buffer
 *
 * @param block the block
 *
 * @return block buffer
 */
uint8_t *rbb_blk_buf(rbb_blk_t block)
{
    configASSERT(block);

    return block->buf;
}

/**
 * free the block
 *
 * @param rbb ring block buffer object
 * @param block the block
 */
void rbb_blk_free(rbb_t rbb, rbb_blk_t block)
{
	UBaseType_t uxSavedInterruptStatus;

    configASSERT(rbb);
    configASSERT(block);
    configASSERT(block->status != RBB_BLK_UNUSED);

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    /* remove it on rbb block list */
    uxListRemove(&block->list);

    block->status = RBB_BLK_UNUSED;

    portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);
}

/**
 * get a continuous block to queue by given size
 *
 *          tail                         head
 * +------------------+---------------+--------+----------+--------+
 * |      block3      |  empty1       | block1 |  block2  |fragment|
 * +------------------+------------------------+----------+--------+
 *                                    |<-- return_size -->|    |
 *                                    |<--- queue_data_len --->|
 *
 *         tail                          head
 * +------------------+---------------+--------+----------+--------+
 * |      block3      |  empty1       | block1 |  block2  |fragment|
 * +------------------+------------------------+----------+--------+
 *                                    |<-- return_size -->|              out of len(b1+b2+b3)    |
 *                                    |<-------------------- queue_data_len -------------------->|
 *
 * @param rbb ring block buffer object
 * @param queue_data_len The max queue data size, and the return size must less then it.
 * @param queue continuous block queue
 *
 * @return the block queue data total size
 */
size_t rbb_blk_queue_get(rbb_t rbb, size_t queue_data_len, rbb_blk_queue_t blk_queue)
{
	UBaseType_t uxSavedInterruptStatus;
    size_t data_total_size = 0;
    ListItem_t *node;
    rbb_blk_t last_block = NULL, block;

    configASSERT(rbb);
    configASSERT(blk_queue);

    if (listLIST_IS_EMPTY(&rbb->blk_list))
        return 0;

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    for (node = listGET_HEAD_ENTRY(&rbb->blk_list); node != listGET_END_MARKER(&rbb->blk_list);
			node = listGET_NEXT(node))		
    {
        if (!last_block)
        {
            last_block = listGET_LIST_ITEM_OWNER(node);
            if (last_block->status == RBB_BLK_PUT)
            {
                /* save the first put status block to queue */
                blk_queue->blocks = last_block;
                blk_queue->blk_num = 0;
            }
            else
            {
                /* the first block must be put status */
                last_block = NULL;
                continue;
            }
        }
        else
        {
            block = listGET_LIST_ITEM_OWNER(node);
            /*
             * these following conditions will break the loop:
             * 1. the current block is not put status
             * 2. the last block and current block is not continuous
             * 3. the data_total_size will out of range
             */
            if (block->status != RBB_BLK_PUT ||
                last_block->buf > block->buf ||
                data_total_size + block->size > queue_data_len)
            {
                break;
            }
            /* backup last block */
            last_block = block;
        }
        /* remove current block */
        uxListRemove(&last_block->list);
        data_total_size += last_block->size;
        last_block->status = RBB_BLK_GET;
        blk_queue->blk_num++;
    }

    portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

    return data_total_size;
}

/**
 * get all block length on block queue
 *
 * @param blk_queue the block queue
 *
 * @return total length
 */
size_t rbb_blk_queue_len(rbb_blk_queue_t blk_queue)
{
    size_t i, data_total_size = 0;

    configASSERT(blk_queue);

    for (i = 0; i < blk_queue->blk_num; i++)
    {
        data_total_size += blk_queue->blocks[i].size;
    }

    return data_total_size;
}

/**
 * return the block queue buffer
 *
 * @param blk_queue the block queue
 *
 * @return block queue buffer
 */
uint8_t *rbb_blk_queue_buf(rbb_blk_queue_t blk_queue)
{
    configASSERT(blk_queue);

    return blk_queue->blocks[0].buf;
}

/**
 * free the block queue
 *
 * @param rbb ring block buffer object
 * @param blk_queue the block queue
 */
void rbb_blk_queue_free(rbb_t rbb, rbb_blk_queue_t blk_queue)
{
    size_t i;

    configASSERT(rbb);
    configASSERT(blk_queue);

    for (i = 0; i < blk_queue->blk_num; i++)
    {
        rbb_blk_free(rbb, &blk_queue->blocks[i]);
    }
}

/**
 * The put status and buffer continuous blocks can be make a block queue.
 * This function will return the length which from next can be make block queue.
 *
 * @param rbb ring block buffer object
 *
 * @return the next can be make block queue's length
 */
size_t rbb_next_blk_queue_len(rbb_t rbb)
{
	UBaseType_t uxSavedInterruptStatus;
    size_t data_len = 0;
    ListItem_t *node;
    rbb_blk_t last_block = NULL, block;

    configASSERT(rbb);

    if (listLIST_IS_EMPTY(&rbb->blk_list))
        return 0;

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    for (node = listGET_HEAD_ENTRY(&rbb->blk_list); node != listGET_END_MARKER(&rbb->blk_list);
			node = listGET_NEXT(node))				
    {
        if (!last_block)
        {
            last_block = listGET_LIST_ITEM_OWNER(node);
            if (last_block->status != RBB_BLK_PUT)
            {
                /* the first block must be put status */
                last_block = NULL;
                continue;
            }
        }
        else
        {
            block = listGET_LIST_ITEM_OWNER(node);
            /*
             * these following conditions will break the loop:
             * 1. the current block is not put status
             * 2. the last block and current block is not continuous
             */
            if (block->status != RBB_BLK_PUT || last_block->buf > block->buf)
            {
                break;
            }
            /* backup last block */
            last_block = block;
        }
        data_len += last_block->size;
    }

    portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

    return data_len;
}

/**
 * get the ring block buffer object buffer size
 *
 * @param rbb ring block buffer object
 *
 * @return buffer size
 */
size_t rbb_get_buf_size(rbb_t rbb)
{
    configASSERT(rbb);

    return rbb->buf_size;
}
