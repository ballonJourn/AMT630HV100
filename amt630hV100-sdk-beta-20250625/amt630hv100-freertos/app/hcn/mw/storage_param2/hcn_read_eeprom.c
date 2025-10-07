/**
*
* @file hcn_read_eeprom.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/02 09:42
* @author och
*
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "log/hcn_log.h"
#include "hal_gpio/hal_gpio.h"
#include "storage_param2/hcn_read_eeprom.h"
#include "os_adapt.h"
#include "i2c.h"

#define DEBUG_E2PROM_ENABLE

static void e2prom_write_protect(int value) {
    hal_gpio_set_output(E2PROM_WP_GPIO, value);
}

int e2prom_write_data(uint16_t addr, uint8_t *buf, int size) {
    struct i2c_msg msg;
    uint8_t add_low = 0;
    uint16_t page_cnt = 0;
    uint16_t msg_buff_size = 0;
    uint16_t msg_addr = 0;
    uint8_t page_first_size = 0;
    uint8_t page_last_size = 0;
    uint8_t retries = 0;
    int ret = 0;

    if ((size < 1) || (addr > E2PROM_CAPACITY_BYTE) 
        || ((addr + size) > E2PROM_CAPACITY_BYTE)) {
        hcn_log_error("e2prom write data out of size!\n");    
        return -1;
    }

    ///< 地址是按照页划分，16个字节为一页
    if (addr % E2PROM_PAGE_SIZE) {
        hcn_log_error("e2prom write data add error!\n");    
        return -2;
    } else {
        page_cnt = (size -1)/E2PROM_PAGE_SIZE + 1;
        if (size < E2PROM_PAGE_SIZE) {
            page_first_size = size;
        } else {
            page_first_size = E2PROM_PAGE_SIZE;
        }

        page_last_size = size & 0xf;
        if (page_last_size == 0) {
            page_last_size = 16;
        }
    }

    uint8_t *write_buff = (uint8_t *)pvPortMalloc(17);
    if (!write_buff) {
        hcn_log_error("e2prom write data pvPortMalloc fail!\n");    
        return -3;
    }

    struct i2c_adapter *adap = i2c_open("i2c1");
    if (adap == NULL) {
        hcn_log_error("open i2c1 fail!\n");
        vPortFree(write_buff);
        return -1;
    }

    e2prom_write_protect(0);

    msg_addr = addr;

    uint8_t *src = buf;
    for (uint16_t i = 0; i < page_cnt; i++) {
        add_low = msg_addr & 0xff;
        write_buff[0] = add_low;
        if (i == 0) {
            msg_buff_size = page_first_size;
        } else if (i == page_cnt - 1) {
            msg_buff_size = page_last_size;
        } else {
            msg_buff_size = E2PROM_PAGE_SIZE;
        }

        memcpy(write_buff + 1, src, msg_buff_size);
        msg.flags = 0;
        msg.addr = T24C08A_DEV_ADDR | ((msg_addr >> 8) & 0x03);
        msg.len = msg_buff_size + 1;
        msg.buf = write_buff;

        retries = 0;
        while (retries < 3) {
            ret = i2c_transfer(adap, &msg, 1);
            if (ret == 1) {
                break;
            } else {
                hcn_log_error("i2c_transfer error:%d\n", ret);
            }
            retries++;
        }

        if (retries < 3) {
            msg_addr += msg_buff_size;
            src += msg_buff_size;
            vTaskDelay(pdMS_TO_TICKS(5));
        } else {
            break;
        }
    }

    i2c_close(adap);
    vPortFree(write_buff);
    e2prom_write_protect(0);

    return (msg_addr - addr);
}

int e2prom_read_data(uint16_t addr, uint8_t *buf, int size) {
    struct i2c_msg msg[2];
    uint8_t addr_low;
    int ret;
    uint8_t retries = 0;
    uint16_t msg_cnt = 0;
    uint16_t msg_buf_size;
    uint16_t msg_addr;
    uint8_t page_off;
    uint8_t page_last_size;
    uint8_t page_first_size;

    if((size < 1) || addr > E2PROM_CAPACITY_BYTE 
        || (addr+size) > E2PROM_CAPACITY_BYTE) {
        hcn_log_error("e2prom read data out of size!\n");    
        return -1;
    }

    page_off = addr % 16;
    if(page_off) {
        hcn_log_error("e2prom read data add error!\n");    
        return -2;
    } else {
        msg_cnt = (size - 1) / 16 + 1;
        if (size < 16) {
            page_first_size = size;
        } else {
            page_first_size = 16;
        }

        page_last_size = size & 0xf;
        if(page_last_size == 0) {
            page_last_size = 16;
        }
    }

    struct i2c_adapter *adap = i2c_open("i2c1");

    if (adap == NULL) {
        hcn_log_error("Open i2c1 fail!\r\n");
        return -1;
    }

    msg_addr = addr;
    uint8_t *dst = buf;

    for (uint16_t i = 0; i < msg_cnt; i++) {
        addr_low = msg_addr & 0xff;
        if (i == 0) {
            msg_buf_size = page_first_size;
        } else if(i == msg_cnt-1) {
            msg_buf_size = page_last_size;
        } else {
            msg_buf_size = 16;
        }

        msg[0].flags = 0;
        msg[0].addr = T24C08A_DEV_ADDR | ((msg_addr >> 8) & 0x03);
        msg[0].len = 1;
        msg[0].buf = &addr_low;

        msg[1].flags = I2C_M_RD;
        msg[1].addr = T24C08A_DEV_ADDR | ((msg_addr >> 8) & 0x03);
        msg[1].len = msg_buf_size;
        msg[1].buf = dst;

        retries = 0;
        while (retries < 3) {
            ret = i2c_transfer(adap, msg, 2);
            if (ret == 2) {
                break;
            } else {
                hcn_log_error("i2c_transfer error %d.\n", ret);
            }
            retries++;
        }

        if (retries < 3) {
            msg_addr += msg_buf_size;
            dst += msg_buf_size;
        } else {
            break;
        }
    }

    i2c_close(adap);

    return (msg_addr - addr);
}

int e2prom_byte_write (uint16_t addr,uint8_t *buf, uint8_t length) {
    uint8_t retires = 0;

    if ((length < 1) || (addr >= E2PROM_CAPACITY_BYTE) 
        || ((addr + length) > E2PROM_CAPACITY_BYTE)) {
        hcn_log_error("e2prom byte write out of size!\n");    
        return -1;
    }

    struct i2c_msg msg[2];
    struct i2c_adapter *adap = i2c_open("i2c1");
    if (adap == NULL) {
        hcn_log_error("Open i2c1 fail!\r\n");
        return -1;
    }

    e2prom_write_protect(0);

    int ret = 0;
    for (int i = 0; i < length; i++) {
        uint8_t addr_low = (addr + i) & 0xFF;
        uint8_t addr_high = T24C08A_DEV_ADDR | (((addr + i) >> 8) & 0x03);

        msg[0].flags = 0; ///< 写操作
        msg[0].len = 1;
        msg[0].buf = &addr_low;
        msg[0].addr = addr_high;

        msg[1].flags = 0; ///< 写操作
        msg[1].len = 1;
        msg[1].buf = &buf[i];
        msg[1].addr = addr_high;

        retires = 0;
        while (retires < 3) {
            ret = i2c_transfer(adap, msg, 2);
            if (ret == 2) {
               ret = 0;
               break;
            } else {
                hcn_log_error("i2c_transfer error %d at address 0x%04X.\n", ret, addr + i);
                ret = -1;
            }
        }

	    vTaskDelay(pdMS_TO_TICKS(5)); ///< 延迟5毫秒，等待EEPROM写入完成
    }
 
    i2c_close(adap);
    e2prom_write_protect(1);

    return ret;
}

int e2prom_byte_read (uint16_t addr, uint8_t *buf, uint8_t length) {
    uint8_t retires;
    if ((length < 1) || (addr >= E2PROM_CAPACITY_BYTE) 
        || ((addr + length) > E2PROM_CAPACITY_BYTE)) {
        hcn_log_error("e2prom byte read out of size!\n");    
        return -1;
    }

    struct i2c_msg msg[2];
    struct i2c_adapter *adap = i2c_open("i2c1");
    if (adap == NULL) {
        hcn_log_error("Open i2c1 fail!\n");
        return -1;
    }

    int ret = 0;

    for (int i = 0; i < length; i++) {
        uint8_t addr_low = (addr + i) & 0xFF;
        uint8_t addr_high = T24C08A_DEV_ADDR | (((addr + i) >> 8) & 0x03);

        ///< 设置写消息，发送地址
        msg[0].flags = 0; ///< 写操作
        msg[0].len = 1;
        msg[0].buf = &addr_low;
        msg[0].addr = addr_high;

        ///< 设置读消息，读取数据
        msg[1].flags = I2C_M_RD; ///< 读操作
        msg[1].len = 1;
        msg[1].buf = &buf[i];
        msg[1].addr = addr_high;

        retires = 0;

        while (retires < 3) {
            ret = i2c_transfer(adap, msg, 2);
            if (ret == 2) {
                ret = 0;
                break;
            } else {
                hcn_log_error("i2c_transfer error %d at address 0x%04X.\n", ret, addr + i);
                retires++;
                ret = -1;
            }
        }
    }

    i2c_close(adap);

    return ret;
}

int e2prom_test(uint16_t addr,int size,uint16_t start_data) {
    uint8_t* w_buf = (uint8_t*)pvPortMalloc(size);
    uint8_t* r_buf = (uint8_t*)pvPortMalloc(size);

    int rtn = 0;
    uint16_t *p = (uint16_t *)w_buf;

    hcn_log_info("%s(0x%08X, %d)\r\n",__FUNCTION__,addr, size);
    if (w_buf == NULL || r_buf==NULL) {
        hcn_log_error("%s pvPortMalloc fail!\r\n",__FUNCTION__);
        rtn = -1;
        goto _error1;
    }

    memset(w_buf, 0, size);
    memset(r_buf, 0, size);

    for (uint16_t i = 0; i < size / 2; i++) {
        p[i] = start_data+i;
    }

    rtn = e2prom_write_data(addr, w_buf, size);
    if (rtn <0) {
        hcn_log_error("err: eeprom_write return %d\r\n",rtn);
        rtn = -2;
        goto _error1;
    } else {
        hcn_log_error("eeprom_write %d bytes\r\n",rtn);
    }

    rtn = e2prom_read_data(addr, r_buf, size);
    if (rtn < 0) {
        hcn_log_error("err: eeprom_read return %d\r\n",rtn);
        rtn = -3;
        goto _error1;
    } else {
        hcn_log_error("eeprom_read %d bytes\r\n",rtn);
    }

#ifdef DEBUG_E2PROM_ENABLE
        hcn_log_info("w_buf:");
        for (uint16_t i = 0; i < size; i++) {
            if (i % 16 == 0) {
                hcn_log_info("\r\n0x%04X: ",i);
            }
            hcn_log_info("%02X ",w_buf[i]);
        }
        hcn_log_info("\r\n");

        hcn_log_info("r_buf:");
        for (uint16_t i = 0; i < size; i++) {
            if (i % 16 == 0) {
                hcn_log_info("\r\n0x%04X: ",i);
            }
            hcn_log_info("%02X ",r_buf[i]);
        }

        hcn_log_info("\r\n");
#endif

    if (memcmp(w_buf, r_buf, size) == 0) {
        hcn_log_info("w_buf == r_buf, eeprom test ok!\r\n");
    } else {
        hcn_log_info("w_buf != r_buf, eeprom test fail!\r\n");
    }

    return 0;
_error1:
    if (w_buf)
        vPortFree(w_buf);

    if (r_buf)
        vPortFree(r_buf);
    
    return rtn;
}

