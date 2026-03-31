/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H
#include <stdio.h>
#include <stdlib.h>

/* 
    定义系统的字节顺序。 需要将网络数据转换为主机字节顺序。 
    允许的值：LITTLE_ENDIAN和BIG_ENDIAN */
#define 	BYTE_ORDER   LITTLE_ENDIAN
 
/* 定义系统的随机数生成器功能 */ 
#define 	LWIP_RAND()   ((u32_t)rand())
 
/* 平台特定的诊断输出。 */ 
#define 	LWIP_PLATFORM_DIAG(x)   do {printf x;} while(0)
 
/* 特定于平台的断言处理。 */  
#define 	LWIP_PLATFORM_ASSERT(x)
 
/* 标准C库 是否包含 stddef.h ，默认使用标准库 */ 
#define 	LWIP_NO_STDDEF_H   0
 
/* 标准C库 是否包含 stdint.h ，默认使用标准库 */  
#define 	LWIP_NO_STDINT_H   0
 
/* 标准C库 是否包含 inttypes.h ，默认使用标准库 */  
#define 	LWIP_NO_INTTYPES_H   0
 
/* 标准C库 是否包含 limits.h ，默认使用标准库 */  
#define 	LWIP_NO_LIMITS_H   0
 
/* 标准C库 是否包含 ctype.h ，默认使用标准库 */  
#define 	LWIP_NO_CTYPE_H   0
 
#define 	LWIP_CONST_CAST(target_type, val)   ((target_type)((ptrdiff_t)val))
 
#define 	LWIP_ALIGNMENT_CAST(target_type, val)   LWIP_CONST_CAST(target_type, val)
 
#define 	LWIP_PTR_NUMERIC_CAST(target_type, val)   LWIP_CONST_CAST(target_type, val)
 
#define 	LWIP_PACKED_CAST(target_type, val)   LWIP_CONST_CAST(target_type, val)
 
/* 分配指定大小的内存缓冲区，其大小足以使用LWIP_MEM_ALIGN对齐其起始地址。 */ 
#define 	LWIP_DECLARE_MEMORY_ALIGNED(variable_name, size)   u8_t variable_name[LWIP_MEM_ALIGN_BUFFER(size)]
 
/*     
    计算对齐缓冲区的内存大小 - 返回MEM_ALIGNMENT的下一个最高倍数
   （例如，LWIP_MEM_ALIGN_SIZE（3）和LWIP_MEM_ALIGN_SIZE（4）将为MEM_ALIGNMENT == 4产生4）。
 */ 
#define 	LWIP_MEM_ALIGN_SIZE(size)   (((size) + MEM_ALIGNMENT - 1U) & ~(MEM_ALIGNMENT-1U))
 
/* 使用未对齐类型作为存储时，计算对齐缓冲区的安全内存大小。 这包括开始时的（MEM_ALIGNMENT  -  1）安全边际 */ 
#define 	LWIP_MEM_ALIGN_BUFFER(size)   (((size) + MEM_ALIGNMENT - 1U))
 
/* 将内存指针以MEM_ALIGNMENT定义的对齐方式对齐，以使ADDR％MEM_ALIGNMENT == 0 */ 
#define 	LWIP_MEM_ALIGN(addr)   ((void *)(((mem_ptr_t)(addr) + MEM_ALIGNMENT - 1) & ~(mem_ptr_t)(MEM_ALIGNMENT-1)))
 
 
/* 打包结构支持。 在声明打包结构之前放置BEFORE。 */ 
#define 	PACK_STRUCT_BEGIN
 
/* 打包结构支持。 在声明打包结构之前放置AFTER 。 */ 
#define 	PACK_STRUCT_END
 
/* 打包结构支持。 放置在打包结构的声明结束和尾随分号之间。 */ 
#define 	PACK_STRUCT_STRUCT
 
/* 打包结构支持。 封装u32_t和u16_t成员。 */ 
#define 	PACK_STRUCT_FIELD(x)   x
 
/* 打包结构支持。 包含u8_t成员，其中一些编译器警告说不需要打包。 */ 
#define 	PACK_STRUCT_FLD_8(x)   PACK_STRUCT_FIELD(x)
 
/* 打包结构支持。 包装结构本身的成员，一些编译器警告不必包装。 */ 
#define 	PACK_STRUCT_FLD_S(x)   PACK_STRUCT_FIELD(x)
 
/*
    PACK_STRUCT_USE_INCLUDES == 1：在打包struct之前和之后使用#include文件支持打包结构。
    该文件包含在结构为“arch / bpstruct.h”之前。
    该结构为“arch / epstruct.h”后包含的文件。
*/ 
//#define 	PACK_STRUCT_USE_INCLUDES
 
/* 消除有关未使用参数的编译器警告 */ 
#define 	LWIP_UNUSED_ARG(x)   (void)x
 
/* 
    LWIP_PROVIDE_ERRNO == 1：让lwIP提供ERRNO值和'errno'变量。 
    如果禁用此选项，cc.h必须定义'errno'，include <errno.h>，定义LWIP_ERRNO_STDINCLUDE以包含    <errno.h>或将LWIP_ERRNO_INCLUDE定义为<errno.h>或等效。
*/ 
#define 	LWIP_PROVIDE_ERRNO
typedef unsigned int sys_prot_t;

//#define LWIP_DEBUG


#endif /* LWIP_ARCH_CC_H */
