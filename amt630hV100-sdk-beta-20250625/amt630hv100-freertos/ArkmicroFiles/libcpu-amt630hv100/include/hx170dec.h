#ifndef _HX170DEC_H_
#define _HX170DEC_H_

struct core_desc
{
	u32 id;    /* id of the core */
	u32 *regs; /* pointer to user registers */
	u32 size;  /* size of register space */
};


#define HX170DEC_IOCGHWOFFSET		 3
#define HX170DEC_IOCGHWIOSIZE		 4

#define HX170DEC_IOC_MC_OFFSETS		 7
#define HX170DEC_IOC_MC_CORES		 8
#define HX170DEC_IOCS_DEC_PUSH_REG	 9
#define HX170DEC_IOCS_PP_PUSH_REG	10
#define HX170DEC_IOCH_DEC_RESERVE	11
#define HX170DEC_IOCT_DEC_RELEASE	12
#define HX170DEC_IOCQ_PP_RESERVE	13
#define HX170DEC_IOCT_PP_RELEASE	14
#define HX170DEC_IOCX_DEC_WAIT		15
#define HX170DEC_IOCX_PP_WAIT		16
#define HX170DEC_IOCS_DEC_PULL_REG	17
#define HX170DEC_IOCS_PP_PULL_REG	18

#define HX170DEC_IOX_ASIC_ID		20

#define HX170DEC_IOC_MAXNR 29

long vdec_ioctl(unsigned int cmd, void *arg);

#endif /* !_HX170DEC_H_ */
