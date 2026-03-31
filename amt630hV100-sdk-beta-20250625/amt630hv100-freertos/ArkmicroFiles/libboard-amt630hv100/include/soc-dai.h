#ifndef _SOC_DAI_H
#define _SOC_DAI_H

#ifdef __cplusplus
extern "C" {
#endif

#define SND_SOC_DAIFMT_FORMAT_MASK	0x000f
#define SND_SOC_DAIFMT_CLOCK_MASK	0x00f0
#define SND_SOC_DAIFMT_INV_MASK		0x0f00
#define SND_SOC_DAIFMT_MASTER_MASK	0xf000

#define SND_SOC_DAIFMT_CBM_CFM		(1 << 12) /* codec clk & FRM master */
#define SND_SOC_DAIFMT_CBS_CFM		(2 << 12) /* codec clk slave & FRM master */
#define SND_SOC_DAIFMT_CBM_CFS		(3 << 12) /* codec clk master & frame slave */
#define SND_SOC_DAIFMT_CBS_CFS		(4 << 12) /* codec clk & FRM slave */

#define SND_SOC_DAIFMT_I2S			1 /* I2S mode */
#define SND_SOC_DAIFMT_RIGHT_J		2 /* Right Justified mode */
#define SND_SOC_DAIFMT_LEFT_J		3 /* Left Justified mode */
#define SND_SOC_DAIFMT_DSP_A		4 /* L data MSB after FRM LRC */
#define SND_SOC_DAIFMT_DSP_B		5 /* L data MSB during FRM LRC */
#define SND_SOC_DAIFMT_AC97			6 /* AC97 */
#define SND_SOC_DAIFMT_PDM			7 /* Pulse density modulation */

#define SND_SOC_DAIFMT_NB_NF		(1 << 8) /* normal bit clock + frame */
#define SND_SOC_DAIFMT_NB_IF		(2 << 8) /* normal BCLK + inv FRM */
#define SND_SOC_DAIFMT_IB_NF		(3 << 8) /* invert BCLK + nor FRM */
#define SND_SOC_DAIFMT_IB_IF		(4 << 8) /* invert BCLK + FRM */

enum snd_soc_bias_level {
	SND_SOC_BIAS_OFF = 0,
	SND_SOC_BIAS_STANDBY = 1,
	SND_SOC_BIAS_PREPARE = 2,
	SND_SOC_BIAS_ON = 3,
};

struct snd_soc_hw_params {
	int rates;
	int channels;
	int bits;
};
struct snd_soc_dai_ops {
	int (*init)(int master);
	int (*hw_params)(struct snd_soc_hw_params *params);
	int (*startup)(int start);
	int (*set_mclk)(int freq, int dir);	//reserved.
	int (*set_fmt)(unsigned int fmt);	//reserved.
	int (*set_mute)(int mute);			//reserved.
	int (*set_volume)(int volume);
};

#ifdef __cplusplus
}
#endif

#endif /* _TOUCH_H */


