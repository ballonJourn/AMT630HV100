#include "amt630h.h"
#include "board.h"
#include "lcd.h"
#include "update.h"
#include "cp15.h"
#include "pxp.h"

#if LCD_INTERFACE_TYPE == LCD_INTERFACE_MIPI   
#define  UPDATING_WIDTH      200
#define  UPDATING_HEIGHT     20

#define  PROGRESS_WIDTH      200
#define  PROGRESS_HEIGHT     36
#else
#define  UPDATING_WIDTH      200
#define  UPDATING_HEIGHT     20

#define  PROGRESS_WIDTH      200
#define  PROGRESS_HEIGHT     32
#endif



#define  UPDATE_LOGO_WIDTH   PROGRESS_WIDTH
#define  UPDATE_LOGO_HEIGHT  (UPDATING_HEIGHT + PROGRESS_HEIGHT)

#define UPDATELOGO_DISPLAY_ADDR	0x20ff0000

#define PXP_DISPLAY_ADDR	0x21000000

#if LCD_BPP == 32
typedef unsigned int Upcolor;
#define UPDATE_TEXT_COLOR	0xffffffff;		//white
#define UPDATE_RECT_COLOR	0xffffffff		//white
#define UPDATE_BAR_COLOR	0xff00ff00;		//green
#elif LCD_BPP == 16
typedef unsigned short Upcolor;
#define UPDATE_TEXT_COLOR	0xffff;		//white
#define UPDATE_RECT_COLOR	0xffff;		//white
#define UPDATE_BAR_COLOR	0x07e0; 	//green
#endif

#define UPDATE_LAN_CHINESE
#ifndef UPDATE_LAN_CHINESE
#define UPDATE_LAN_ENGLISH
#endif

#ifdef UPDATE_LAN_CHINESE
//chinese
#define  UPDATING_POINT_COUNT  397
static unsigned short updating[UPDATING_POINT_COUNT] =
{
    810, 821, 828, 840, 866, 878, 883, 894, 900, 1008, 1009, 1010, 1021, 1024, 1025, 1026, 
    1027, 1028, 1040, 1060, 1066, 1076, 1077, 1078, 1079, 1080, 1083, 1086, 1093, 1094, 1096, 1097, 
    1098, 1099, 1100, 1206, 1207, 1210, 1220, 1223, 1224, 1225, 1227, 1228, 1240, 1260, 1261, 1263, 
    1264, 1265, 1266, 1267, 1268, 1269, 1275, 1276, 1277, 1280, 1281, 1283, 1285, 1293, 1299, 1404, 
    1405, 1406, 1407, 1410, 1420, 1422, 1425, 1427, 1436, 1440, 1441, 1442, 1443, 1444, 1445, 1461, 
    1466, 1467, 1468, 1477, 1481, 1483, 1484, 1492, 1494, 1498, 1499, 1500, 1501, 1502, 1603, 1606, 
    1610, 1619, 1621, 1622, 1624, 1627, 1628, 1629, 1636, 1637, 1638, 1639, 1640, 1641, 1644, 1645, 
    1664, 1665, 1666, 1667, 1669, 1677, 1678, 1679, 1680, 1683, 1684, 1685, 1686, 1692, 1694, 1695, 
    1696, 1697, 1698, 1702, 1806, 1810, 1812, 1813, 1814, 1818, 1819, 1820, 1821, 1824, 1825, 1826, 
    1827, 1828, 1829, 1836, 1840, 1844, 1859, 1860, 1861, 1862, 1863, 1864, 1865, 1866, 1867, 1868, 
    1869, 1870, 1874, 1875, 1876, 1877, 1880, 1881, 1882, 1883, 1884, 1885, 1891, 1892, 1894, 1896, 
    1897, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2020, 2024, 2025, 2028, 
    2036, 2040, 2044, 2058, 2059, 2060, 2063, 2068, 2076, 2077, 2078, 2080, 2085, 2090, 2092, 2094, 
    2096, 2097, 2098, 2099, 2100, 2101, 2206, 2210, 2219, 2220, 2221, 2222, 2223, 2224, 2226, 2227, 
    2228, 2236, 2237, 2238, 2239, 2240, 2241, 2242, 2243, 2244, 2245, 2260, 2264, 2265, 2266, 2267, 
    2268, 2276, 2277, 2279, 2280, 2281, 2282, 2283, 2284, 2285, 2292, 2294, 2295, 2296, 2298, 2406, 
    2410, 2419, 2420, 2423, 2426, 2427, 2436, 2440, 2460, 2464, 2465, 2466, 2467, 2468, 2475, 2477, 
    2480, 2485, 2492, 2494, 2498, 2499, 2500, 2501, 2502, 2606, 2610, 2621, 2622, 2623, 2626, 2627, 
    2640, 2660, 2662, 2663, 2664, 2668, 2674, 2675, 2677, 2680, 2681, 2682, 2683, 2684, 2685, 2692, 
    2694, 2695, 2696, 2697, 2698, 2805, 2810, 2820, 2821, 2822, 2825, 2826, 2827, 2840, 2851, 2852, 
    2860, 2861, 2862, 2864, 2865, 2866, 2867, 2868, 2874, 2877, 2880, 2885, 2892, 2897, 2899, 3004, 
    3005, 3010, 3018, 3019, 3021, 3022, 3024, 3025, 3028, 3040, 3051, 3052, 3060, 3061, 3064, 3068, 
    3077, 3080, 3083, 3085, 3092, 3096, 3100, 3106, 3107, 3114, 3115, 3122, 3123, 3203, 3204, 3210, 
    3221, 3223, 3224, 3228, 3229, 3230, 3240, 3252, 3264, 3266, 3267, 3268, 3277, 3280, 3284, 3285, 
    3292, 3295, 3296, 3300, 3301, 3403, 3422, 3423, 3451, 3464, 3494, 3495, 3502, 
};
#else
//english
#define  UPDATING_POINT_COUNT  237
static unsigned short updating[UPDATING_POINT_COUNT] =
{
    1021, 1022, 1023, 1044, 1045, 1222, 1223, 1235, 1236, 1422, 1423, 1435, 1436, 1601, 1602, 1603, 
    1605, 1606, 1607, 1609, 1610, 1611, 1612, 1613, 1614, 1619, 1620, 1621, 1622, 1623, 1627, 1628, 
    1629, 1630, 1633, 1634, 1635, 1636, 1637, 1638, 1642, 1643, 1644, 1645, 1649, 1650, 1652, 1653, 
    1654, 1659, 1660, 1661, 1662, 1663, 1802, 1803, 1806, 1807, 1810, 1811, 1814, 1815, 1818, 1819, 
    1822, 1823, 1826, 1827, 1830, 1831, 1835, 1836, 1844, 1845, 1850, 1851, 1852, 1854, 1855, 1858, 
    1859, 1861, 1862, 2002, 2003, 2006, 2007, 2010, 2011, 2014, 2015, 2018, 2019, 2022, 2023, 2028, 
    2029, 2030, 2031, 2035, 2036, 2044, 2045, 2050, 2054, 2055, 2058, 2059, 2061, 2062, 2202, 2203, 
    2206, 2207, 2210, 2211, 2214, 2215, 2218, 2219, 2222, 2223, 2227, 2228, 2230, 2231, 2235, 2236, 
    2244, 2245, 2250, 2254, 2255, 2259, 2260, 2261, 2402, 2403, 2406, 2407, 2410, 2411, 2414, 2415, 
    2418, 2419, 2422, 2423, 2426, 2427, 2430, 2431, 2435, 2436, 2444, 2445, 2450, 2454, 2455, 2458, 
    2459, 2466, 2467, 2468, 2474, 2475, 2476, 2482, 2483, 2484, 2603, 2604, 2605, 2606, 2607, 2608, 
    2610, 2611, 2612, 2613, 2614, 2619, 2620, 2621, 2622, 2623, 2624, 2627, 2628, 2629, 2630, 2631, 
    2632, 2636, 2637, 2638, 2642, 2643, 2644, 2645, 2646, 2647, 2649, 2650, 2651, 2652, 2654, 2655, 
    2656, 2658, 2659, 2660, 2661, 2662, 2666, 2667, 2668, 2674, 2675, 2676, 2682, 2683, 2684, 2810, 
    2811, 2858, 2859, 2862, 2863, 3009, 3010, 3011, 3012, 3059, 3060, 3061, 3062,
};
#endif

static int update_start = 0;
static int update_percent = -1;

static void updating_init(void)
{
	int i;
	Upcolor *p = (Upcolor *)(UPDATELOGO_DISPLAY_ADDR);
	Upcolor color = UPDATE_TEXT_COLOR;
	int pos = 0;

	for(i = 0; i < UPDATING_WIDTH * UPDATING_HEIGHT; i++) {
        if(updating[pos] == i) {
            *p = color;
            pos++;
        }
        p++;
	}
}

void update_progress_init(void)
{
    int i, j;
    Upcolor *p = (Upcolor *)(UPDATELOGO_DISPLAY_ADDR +
								UPDATING_HEIGHT * PROGRESS_WIDTH * LCD_BPP / 8);
    int height = UPDATE_LOGO_HEIGHT;
    Upcolor color = UPDATE_RECT_COLOR;

    for(j = UPDATING_HEIGHT; j < height; j++) {
        for(i = 0; i < PROGRESS_WIDTH; i++) {
            if(j < (UPDATING_HEIGHT + 2) || j > (height -3) || \
                i < 2 || i > (PROGRESS_WIDTH -3)) {
                *p = color;
            }      
            p++;
        }
    }
}

void update_progress_set(int percent)
{
    int i, j, end;
	unsigned int memstart = UPDATELOGO_DISPLAY_ADDR +
								UPDATING_WIDTH * UPDATING_HEIGHT * LCD_BPP / 8;
    Upcolor *p = (Upcolor *)memstart;
    int height = UPDATE_LOGO_HEIGHT;
    Upcolor color = UPDATE_BAR_COLOR;

	if(percent < 0 || percent > 100) {
        return;
    }

	if (!update_start || percent == update_percent)
		return;

	update_percent = percent;

    end = percent * (PROGRESS_WIDTH - 3) / 100;

    for(j = UPDATING_HEIGHT; j < height; j++) {
        for(i = 0; i < PROGRESS_WIDTH; i++) {
            if(j > (UPDATING_HEIGHT + 2) && j < (height -3) \
                && i > 2 && i < end) {
                *p = color;
            }
            p++;
        }
    }

	CP15_clean_dcache_for_dma((unsigned int)memstart, 
		(unsigned int)memstart + PROGRESS_WIDTH *PROGRESS_HEIGHT * LCD_BPP / 8);
    
#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0    
            pxp_scaler_rotate(UPDATELOGO_DISPLAY_ADDR, 0, 0,
                PXP_SRC_FMT_RGB565, UPDATE_LOGO_WIDTH, UPDATE_LOGO_HEIGHT,
                PXP_DISPLAY_ADDR, 0, PXP_OUT_FMT_RGB565, UPDATE_LOGO_HEIGHT, UPDATE_LOGO_WIDTH, LCD_ROTATE_ANGLE);
#endif            
  

}

void update_logo_init(void)
{
	int x, y;
	unsigned int width = UPDATE_LOGO_WIDTH;
    unsigned int height = UPDATE_LOGO_HEIGHT;
#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
    unsigned int display_addr = PXP_DISPLAY_ADDR;
	unsigned int out_width;
	unsigned int out_height;
#endif

	if (update_start)
		return;

    memset((void *)UPDATELOGO_DISPLAY_ADDR, 0 , width *height * LCD_BPP / 8);
    updating_init();
    update_progress_init();
	CP15_clean_dcache_for_dma(UPDATELOGO_DISPLAY_ADDR,
		UPDATELOGO_DISPLAY_ADDR + width *height * LCD_BPP / 8);

#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
#if LCD_ROTATE_ANGLE == LCD_ROTATE_ANGLE_180
	out_width = width;
	out_height = height;
#else
	out_width = height;
	out_height = width;
#endif
	ark_lcd_set_osd_size(LCD_OSD1, out_width, out_height);
	pxp_scaler_rotate(UPDATELOGO_DISPLAY_ADDR, 0, 0, PXP_SRC_FMT_RGB565, width, height,
			display_addr, 0, PXP_OUT_FMT_RGB565, out_width, out_height, LCD_ROTATE_ANGLE);
	ark_lcd_set_osd_yaddr(LCD_OSD1, display_addr);
	y = (LCD_HEIGHT - out_height) / 2;
	x = (LCD_WIDTH - out_width) / 2;
#else
	ark_lcd_set_osd_yaddr(LCD_OSD1, UPDATELOGO_DISPLAY_ADDR);
	ark_lcd_set_osd_size(LCD_OSD1, width, height);
	x = (LCD_WIDTH - width) / 2;
	y = (LCD_HEIGHT - height) / 2;
#endif
	ark_lcd_set_osd_possition(LCD_OSD1, x, y);
	ark_lcd_osd_enable(LCD_OSD1, 1);
	ark_lcd_set_osd_sync(LCD_OSD1);

	update_start = 1;
}
