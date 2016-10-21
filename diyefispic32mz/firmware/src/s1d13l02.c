#include "../../../ion/bsp.h"

// pointer to the EBI Buffer that is assigned to EBICS0
// is a 2Mx16 SRAM
#define EBICS0_BASE 0x20000000
// size of buffer is 4MB
#define EBICS0_SIZE 0x00200000
// pointer to EBI Buffer that is assigned to EBICS1
// is an S1D13L02 display controller
#define EBICS1_BASE 0x20a00000

#define S1D13L02_INDEX_REG  (EBICS1_BASE + 0)
#define S1D13L02_DATA_REG (EBICS1_BASE + 4)
#define S1D13L02_STATUS_REG (EBICS1_BASE + 0)

// these hold the sizes of the hardware windows
#define ROOT_WINDOW_BASE  0
#define ROOT_WINDOW_SIZE  640

// menu window offset in pixels
#define MENU_WINDOW_BASE  (640 * 480)
#define MENU_WINDOW_SIZE  320

// alert window offset in pixels
#define ALERT_WINDOW_BASE (MENU_WINDOW_BASE + (320 * 480))
#define ALERT_WINDOW_SIZE 128

// pointer to EBI buffer that is assigned to EBICS2
// is a 32m NOR Flas
#define EBICS2_BASE 0x20200000
#define EBICS2_SIZE 0x00800000

#define num_elements(values) (sizeof(values)/sizeof(values[0]))

static uint16_t *framebuffer_base = (uint16_t *)(EBICS0_BASE);
static size_t framebuffer_free = EBICS0_SIZE >> 1;

extern const bitmap_t splash_bitmap;
// These are the definitions for the EPSON S1D13L02 display controller

#define S1D_13L02
#define S1D_DISPLAY_WIDTH               640
#define S1D_DISPLAY_HEIGHT              480
#define S1D_DISPLAY_BPP                 16
#define S1D_DISPLAY_SCANLINE_BYTES      1280
#define S1D_DISPLAY_FRAME_RATE          0
#define S1D_DISPLAY_PCLK                24000000L
#define S1D_PHYSICAL_REG_ADDR           0x00000000L
#define S1D_PHYSICAL_VMEM_ADDR          0x00200000L
#define S1D_PHYSICAL_REG_SIZE           593L
#define S1D_PHYSICAL_VMEM_SIZE          1048576L
#define S1D_PHYSICAL_VMEM_REQUIRED      1280000L
#define S1D_PALETTE_SIZE                256
#define S1D_POWER_DELAY_OFF             0
#define S1D_POWER_DELAY_ON              0
#define S1D_REGRESERVED                 0xFFF0
#define S1D_REGDELAYPLL                 0xFFFC
#define S1D_REGDELAYOFF                 0xFFFD
#define S1D_REGDELAYON                  0xFFFE

#define REG0004_CFG_PIN_STATUS       0x0004    // Configuration Pin Status Register
#define REG000C_PLL_0                0x000C    // PLL Setting Register 0
#define REG000E_PLL_1                0x000E    // PLL Setting Register 1
#define REG0010_PLL_2                0x0010    // PLL Setting Register 2
#define REG0012_PLL_3                0x0012    // PLL Setting Register 3
#define REG0014_MISC_CONFIG          0x0014    // Miscellaneous Configuration Register
#define REG0016_SOFT_RESET           0x0016    // Software Reset Register [WRITEONLY]
#define REG0018_SYSTEM_CLOCK         0x0018    // System Clock Setting Register
#define REG0030_LCD_CLOCK            0x0030    // LCD Interface Clock Setting Register
#define REG0032_LCD_CONFIG           0x0032    // LCD Interface Configuration Register
#define REG0040_LCD_HT               0x0040    // LCD Horizontal Total (FPLINE Period) Register
#define REG0042_LCD_HDP              0x0042    // LCD Horizontal Display Period Register
#define REG0044_LCD_HDPS             0x0044    // LCD Horizontal Display Period Start Position Register
#define REG0046_LCD_HP               0x0046    // LCD Horizontal Pulse (FPLINE) Register
#define REG0048_LCD_HPS              0x0048    // LCD Horizontal Pulse (FPLINE) Start Position Register
#define REG004A_LCD_VT               0x004A    // LCD Vertical Total (FPFRAME Period) Register
#define REG004C_LCD_VDP              0x004C    // LCD Vertical Display Period Register
#define REG004E_LCD_VDPS             0x004E    // LCD Vertical Display Period Start Position Register
#define REG0050_LCD_VP               0x0050    // LCD Vertical Pulse (FPFRAME) Register
#define REG0052_LCD_VPS              0x0052    // LCD Vertical Pulse (FPFRAME) Start Position Register
#define REG0068_LCD_VSYNC_OUT        0x0068    // LCD VSYNC Output Register
#define REG0180_HOST_CONFIG          0x0180    // Host Interface Configuration Register
#define REG0182_MEM_SADDR_0          0x0182    // Memory Start Address Register 0
#define REG0184_MEM_SADDR_1          0x0184    // Memory Start Address Register 1
#define REG0186_HWC_WR_ADDR_OFF      0x0186    // HWC Memory Rectangular Write Address Offset Register
#define REG0188_HWC_WR_HSIZE         0x0188    // HWC Memory Rectangular Write Horizontal Size Register
#define REG018A_HWC_WR_VSIZE         0x018A    // HWC Memory Rectangular Write Vertical Size Register
#define REG018C_MEM_ACCESS_PORT      0x018C    // Memory Access Port Register
#define REG018E_HWC_RAW_STATUS       0x018E    // HWC Raw Status Register [READONLY]
#define REG0190_HWC_IRQ_CONTROL      0x0190    // HWC Interrupt Control Register
#define REG0192_HWC_STATUS           0x0192    // HWC Status Register
#define REG0194_MEM_WR_ADDR_OFF      0x0194    // Memory Rectangular Write Address Offset Register
#define REG0196_MEM_WR_ADDR_W        0x0196    // Memory Rectangular Write Address Width Register
#define REG0198_VOUT_CONFIG          0x0198    // VOUT Configuration Register
#define REG0200_DISPLAY_MODE_0       0x0200    // Display Mode Setting Register 0
#define REG0202_DISPLAY_MODE_1       0x0202    // Display Mode Setting Register 1
#define REG0204_OVERLAY_ALPHA        0x0204    // Overlay and Alpha Blend Control Register
#define REG0206_BG_COLOR             0x0206    // Background Color Setting Register
#define REG0208_ALPHA_RATIO          0x0208    // Alpha Blend Ratio Setting Register
#define REG020C_PIP1_OVERLAY_KEY     0x020C    // PIP1 Window Overlay Key Color Register
#define REG020E_PIP2_OVERLAY_KEY     0x020E    // PIP2 Window Overlay Key Color Register
#define REG0210_ALPHA_BLEND1_KEY     0x0210    // Alpha Blend 1 Key Color Register
#define REG0212_ALPHA_BLEND2_KEY     0x0212    // Alpha Blend 2 Key Color Register
#define REG0214_ALPHA_BLEND3_KEY     0x0214    // Alpha Blend 3 Key Color Register
#define REG0216_ALPHA_BLEND4_KEY     0x0216    // Alpha Blend 4 Key Color Register
#define REG0218_MAIN1_X_START        0x0218    // Main Window 1 X Start Position Register
#define REG021A_MAIN1_Y_START        0x021A    // Main Window 1 Y Start Position Register
#define REG0220_MAIN2_X_START        0x0220    // Main Window 2 X Start Position Register
#define REG0222_MAIN2_Y_START        0x0222    // Main Window 2 Y Start Position Register
#define REG0228_PIP1_X_START         0x0228    // PIP1 Window X Start Position Register
#define REG022A_PIP1_Y_START         0x022A    // PIP1 Window Y Start Position Register
#define REG022C_PIP1_X_END           0x022C    // PIP1 Window X End Position Register
#define REG022E_PIP1_Y_END           0x022E    // PIP1 Window Y End Position Register
#define REG0230_PIP2_X_START         0x0230    // PIP2 Window X Start Position Register
#define REG0232_PIP2_Y_START         0x0232    // PIP2 Window Y Start Position Register
#define REG0234_PIP2_X_END           0x0234    // PIP2 Window X End Position Register
#define REG0236_PIP2_Y_END           0x0236    // PIP2 Window Y End Position Register
#define REG0238_MAIN_SCROLL_SA_0     0x0238    // Main Window Scroll Start Address Register 0
#define REG023A_MAIN_SCROLL_SA_1     0x023A    // Main Window Scroll Start Address Register 1
#define REG023C_MAIN_SCROLL_EA_0     0x023C    // Main Window Scroll End Address Register 0
#define REG023E_MAIN_SCROLL_EA_1     0x023E    // Main Window Scroll End Address Register 1
#define REG0240_MAIN1_SADDR_0        0x0240    // Main Window 1 Display Start Address Register 0
#define REG0242_MAIN1_SADDR_1        0x0242    // Main Window 1 Display Start Address Register 1
#define REG0244_MAIN1_STRIDE         0x0244    // Main Window 1 Line Address Offset Register
#define REG0246_MAIN1_HORZ_SIZE      0x0246    // Main Window 1 Image Horizontal Size Register
#define REG0248_MAIN1_VERT_SIZE      0x0248    // Main Window 1 Image Vertical Size Register
#define REG024A_MAIN2_SADDR_0        0x024A    // Main Window 2 Display Start Address Register 0
#define REG024C_MAIN2_SADDR_1        0x024C    // Main Window 2 Display Start Address Register 1
#define REG024E_MAIN2_STRIDE         0x024E    // Main Window 2 Line Address Offset Register
#define REG0250_MAIN2_HORZ_SIZE      0x0250    // Main Window 2 Image Horizontal Size Register
#define REG0252_MAIN2_VERT_SIZE      0x0252    // Main Window 2 Image Vertical Size Register
#define REG0260_PIP1_SCALING         0x0260    // PIP1 Scaling Mode Register
#define REG0262_PIP1_HORZ_SCALE      0x0262    // PIP1 Scaler Horizontal Scale Register
#define REG0264_PIP1_VERT_SCALE      0x0264    // PIP1 Scaler Vertical Scale Register
#define REG0266_PIP1_SCALER_PORT     0x0266    // PIP1 Scaler Port Address Counter Control Register
#define REG0268_PIP1_SCALER_COEF     0x0268    // PIP1 Scaler Coefficient Table Access Port Register
#define REG026E_PIP1_SCALER_CTRL     0x026E    // PIP1 Scaler Control Register
#define REG0270_PIP1_SCROLL_SA_0     0x0270    // PIP1 Window Scroll Start Address Register 0
#define REG0272_PIP1_SCROLL_SA_1     0x0272    // PIP1 Window Scroll Start Address Register 1
#define REG0274_PIP1_SCROLL_EA_0     0x0274    // PIP1 Window Scroll End Address Register 0
#define REG0276_PIP1_SCROLL_EA_1     0x0276    // PIP1 Window Scroll End Address Register 1
#define REG0278_PIP1_SADDR_0         0x0278    // PIP1 Window 1 Display Start Address Register 0
#define REG027A_PIP1_SADDR_1         0x027A    // PIP1 Window 1 Display Start Address Register 1
#define REG027C_PIP1_STRIDE          0x027C    // PIP1 Window 1 Line Address Offset Register
#define REG027E_PIP1_HORZ_SIZE       0x027E    // PIP1 Window 1 Image Horizontal Size Register
#define REG0280_PIP1_VERT_SIZE       0x0280    // PIP1 Window 1 Image Vertical Size Register
#define REG0282_PIP1_PSEUDO          0x0282    // PIP1 Pseudo Setting Register
#define REG02A0_PIP2_SCALING         0x02A0    // PIP2 Scaling Mode Register
#define REG02A2_PIP2_HORZ_SCALE      0x02A2    // PIP2 Scaler Horizontal Scale Register
#define REG02A4_PIP2_VERT_SCALE      0x02A4    // PIP2 Scaler Vertical Scale Register
#define REG02A6_PIP2_SCALER_PORT     0x02A6    // PIP2 Scaler Port Address Counter Control Register
#define REG02A8_PIP2_SCALER_COEF     0x02A8    // PIP2 Scaler Coefficient Table Access Port Register
#define REG02AE_PIP2_SCALER_CTRL     0x02AE    // PIP2 Scaler Control Register
#define REG02B0_PIP2_SCROLL_SA_0     0x02B0    // PIP2 Window Scroll Start Address Register 0
#define REG02B2_PIP2_SCROLL_SA_1     0x02B2    // PIP2 Window Scroll Start Address Register 1
#define REG02B4_PIP2_SCROLL_EA_0     0x02B4    // PIP2 Window Scroll End Address Register 0
#define REG02B6_PIP2_SCROLL_EA_1     0x02B6    // PIP2 Window Scroll End Address Register 1
#define REG02B8_PIP2_SADDR_0         0x02B8    // PIP2 Window 1 Display Start Address Register 0
#define REG02BA_PIP2_SADDR_1         0x02BA    // PIP2 Window 1 Display Start Address Register 1
#define REG02BC_PIP2_STRIDE          0x02BC    // PIP2 Window 1 Line Address Offset Register
#define REG02BE_PIP2_HORZ_SIZE       0x02BE    // PIP2 Window 1 Image Horizontal Size Register
#define REG02C0_PIP2_VERT_SIZE       0x02C0    // PIP2 Window 1 Image Vertical Size Register
#define REG0300_GPIO_CONFIG_0        0x0300    // GPIO Configuration Register 0
#define REG0302_GPIO_CONFIG_1        0x0302    // GPIO Configuration Register 1
#define REG0304_GPIO_INPUT_0         0x0304    // GPIO Input Enable Register 0
#define REG0306_GPIO_INPUT_1         0x0306    // GPIO Input Enable Register 1
#define REG0308_GPIO_PULLDOWN_0      0x0308    // GPIO Pull Down Control Register 0
#define REG030A_GPIO_PULLDOWN_1      0x030A    // GPIO Pull Down Control Register 1
#define REG030C_GPIO_STATUS_0        0x030C    // GPIO Status Register 0
#define REG030E_GPIO_STATUS_1        0x030E    // GPIO Status Register 1
#define REG0310_GPIO_POS_EDGE_0      0x0310    // GPIO Positive Edge Interrupt Trigger Register 0
#define REG0312_GPIO_POS_EDGE_1      0x0312    // GPIO Positive Edge Interrupt Trigger Register 1
#define REG0314_GPIO_NEG_EDGE_0      0x0314    // GPIO Negative Edge Interrupt Trigger Register 0
#define REG0316_GPIO_NEG_EDGE_1      0x0316    // GPIO Negative Edge Interrupt Trigger Register 1
#define REG0318_GPIO_IRQ_0           0x0318    // GPIO Interrupt Status Register 0
#define REG031A_GPIO_IRQ_1           0x031A    // GPIO Interrupt Status Register 1
#define REG0400_PIP2LUT_COUNTER      0x0400    // PIP2-LUT Address Counter Register
#define REG0402_PIP2LUT_DATAPORT     0x0402    // PIP2-LUT Data Port Register
#define REG0500_LCDLUT_COUNTER       0x0500    // LCD-LUT Address Counter Register
#define REG0502_LCDLUT_DATAPORT      0x0502    // LCD-LUT Data Port Register
#define REG0A00_IRQ_STATUS           0x0A00    // Interrupt Status Register [READONLY]
#define REG0A02_IRQ_CONTROL_0        0x0A02    // Interrupt Control Register 0
#define REG0A04_IRQ_CONTROL_1        0x0A04    // Interrupt Control Register 1

// SPECIAL FLAGS
#define REGFLAG_BASE                 0xFFF0     // Special reserved flags beyond this point
#define REGFLAG_DELAY                0xFFFC     // PLL Register Programming Delay (in us)
#define REGFLAG_OFF_DELAY            0xFFFD     // LCD Panel Power Off Delay (in ms)
#define REGFLAG_ON_DELAY             0xFFFE     // LCD Panel Power On Delay (in ms)
#define REGFLAG_END_OF_TABLE         0xFFFF     // End of Registers Marker

typedef uint16_t S1D_INDEX;
typedef uint16_t S1D_VALUE;
typedef struct _s1d_regs_t
{
    S1D_INDEX Index;
    S1D_VALUE Value;
} s1d_regs_t;

static s1d_regs_t s1d13l02_init_values[] =  
  {  
    { REG0004_CFG_PIN_STATUS, 0x0000 },   /* Configuration Pin Status Register                  */  
    { REG0018_SYSTEM_CLOCK, 0x0000 },   /* System Clock Setting Register                      */  
    { REG000C_PLL_0, 0x1804 },   // CLKI is 10Mhz, so divide by 5 for 25mhz then pll x 25 for 50mhz 
    { REG000E_PLL_1, 0x0002 },   /* PLL Setting Register 1                             */  
    { REG0010_PLL_2, 0x0810 },   /* PLL Setting Register 2                             */  
    { REG0012_PLL_3, 0x0000 },   /* PLL Setting Register 3                             */  
    { S1D_REGDELAYPLL, 0x03E8 },   /* PLL Register Programming Delay (in us)             */  
    { REG0014_MISC_CONFIG, 0x04D0 },   /* Miscellaneous Configuration Register               */  
    { REG0030_LCD_CLOCK, 0x0500 },    // divide system clock (50mhz) by 2  
    { REG0032_LCD_CONFIG, 0x0020 },   // Shift change on rising edge,  16 bit panel
    { REG0040_LCD_HT, (792/8)-1 },    // 792 pixels/line
    { REG0042_LCD_HDP, (640/2)-1 },   // 640 display pixels/line
    { REG0044_LCD_HDPS, 80 },         // horizontal display
    { REG0046_LCD_HP, 0x80 | (112-1) }, // Active high h-pulse, 112 pixel pulse
    { REG0048_LCD_HPS, 40-1 },        // Horizontal start position
    { REG004A_LCD_VT, 526-1 },        // number of lines480
    { REG004C_LCD_VDP, 480-1 },       // Vertical Display Period
    { REG004E_LCD_VDPS, 43 },         // Vertical Display Period Start Position
    { REG0050_LCD_VP, 2 },            // Vertical Pulse (FPFRAME)
    { REG0052_LCD_VPS, 0x000A },      // Vertical Pulse (FPFRAME) Start Position
    { REG0068_LCD_VSYNC_OUT, 0 },     // VSYNC Output Register
    { REG0198_VOUT_CONFIG, 0x1000 },  // VOUT raised every frame
    
    { REG0200_DISPLAY_MODE_0, 0x0030 }, // the display palette is bypassed
    { REG0202_DISPLAY_MODE_1, 0x0461 }, // Display on, Display Main, PIP1, PIP2
    { REG0204_OVERLAY_ALPHA, 0x0002 },  // enable transparency on the PIP1 window
    { REG020C_PIP1_OVERLAY_KEY, 0x5555 }, // random color on menu window causes transparency
    
    // Display window 1, 640x480 
    { REG0218_MAIN1_X_START, 0x0000 },    /* Main Window 1 X Start Position Register            */
    { REG021A_MAIN1_Y_START, 0x0000 },    /* Main Window 1 Y Start Position Register            */
    { REG0238_MAIN_SCROLL_SA_0, 0x0000 }, /* Main Window Scroll Start Address Register 0        */
    { REG023A_MAIN_SCROLL_SA_1, 0x0000 }, /* Main Window Scroll Start Address Register 1        */
    { REG023C_MAIN_SCROLL_EA_0, 0xFFFC }, /* Main Window Scroll End Address Register 0          */
    { REG023E_MAIN_SCROLL_EA_1, 0x000F }, /* Main Window Scroll End Address Register 1          */
    { REG0240_MAIN1_SADDR_0, 0x0000 },    /* Main Window 1 Display Start Address Register 0     */
    { REG0242_MAIN1_SADDR_1, 0x0000 },    /* Main Window 1 Display Start Address Register 1     */
    { REG0244_MAIN1_STRIDE, 1280 },       /* Main Window 1 Line Address Offset Register         */
    { REG0246_MAIN1_HORZ_SIZE, 640-1 },   /* Main Window 1 Image Horizontal Size Register       */
    { REG0248_MAIN1_VERT_SIZE, 480-1 },   /* Main Window 1 Image Vertical Size Register         */
    
     // Display window 2, Menu window, 320 x 480 pixels.
    { REG0228_PIP1_X_START, 320 },      // starts at 320 pixels down
    { REG022A_PIP1_Y_START, 0 },        // starts at top
    { REG022C_PIP1_X_END, 640 },        // ends at bottom
    { REG022E_PIP1_Y_END, 480 },        // and full width
    { REG0260_PIP1_SCALING, 0x0000 },     /* PIP1 Scaling Mode Register                         */
    { REG0262_PIP1_HORZ_SCALE, 0x0000 },  /* PIP1 Scaler Horizontal Scale Register              */
    { REG0264_PIP1_VERT_SCALE, 0x0000 },  /* PIP1 Scaler Vertical Scale Register                */
    { REG0266_PIP1_SCALER_PORT, 0x0000 }, /* PIP1 Scaler Port Address Counter Control Register  */
    { REG026E_PIP1_SCALER_CTRL, 0x0000 }, /* PIP1 Scaler Control Register                       */
    { REG0270_PIP1_SCROLL_SA_0, 0x0000 }, /* PIP1 Window Scroll Start Address Register 0        */
    { REG0272_PIP1_SCROLL_SA_1, 0x0000 }, /* PIP1 Window Scroll Start Address Register 1        */
    { REG0274_PIP1_SCROLL_EA_0, 0xFFFC }, /* PIP1 Window Scroll End Address Register 0          */
    { REG0276_PIP1_SCROLL_EA_1, 0x000F }, /* PIP1 Window Scroll End Address Register 1          */
    { REG0278_PIP1_SADDR_0, 24576 },      /* PIP1 Window 1 Display Start Address Register 0     */
    { REG027A_PIP1_SADDR_1, 9 },          /* PIP1 Window 1 Display Start Address Register 1     */
    { REG027C_PIP1_STRIDE, 640 },         /* PIP1 Window 1 Line Address Offset Register         */
    { REG027E_PIP1_HORZ_SIZE, 640 },      /* PIP1 Window 1 Image Horizontal Size Register       */
    { REG0280_PIP1_VERT_SIZE, 320 },      /* PIP1 Window 1 Image Vertical Size Register         */
    { REG0282_PIP1_PSEUDO, 0x0010 },      /* PIP1 Pseudo Setting Register                       */
    
    // Display window 3, Alert window, 128 x 480 pixels
    { REG0230_PIP2_X_START, 512 },      // alerts at bottom
    { REG0232_PIP2_Y_START, 0 },        // full width
    { REG0234_PIP2_X_END, 640 },        // bottom of screen
    { REG0236_PIP2_Y_END, 480 },        // full width
    { REG02A0_PIP2_SCALING, 0x0000 },     /* PIP2 Scaling Mode Register                         */
    { REG02A2_PIP2_HORZ_SCALE, 0x0000 },  /* PIP2 Scaler Horizontal Scale Register              */
    { REG02A4_PIP2_VERT_SCALE, 0x0000 },  /* PIP2 Scaler Vertical Scale Register                */
    { REG02A6_PIP2_SCALER_PORT, 0x0000 }, /* PIP2 Scaler Port Address Counter Control Register  */
    { REG02AE_PIP2_SCALER_CTRL, 0x0000 }, /* PIP2 Scaler Control Register                       */
    { REG02B0_PIP2_SCROLL_SA_0, 0x0000 }, /* PIP2 Window Scroll Start Address Register 0        */
    { REG02B2_PIP2_SCROLL_SA_1, 0x0000 }, /* PIP2 Window Scroll Start Address Register 1        */
    { REG02B4_PIP2_SCROLL_EA_0, 0xFFFC }, /* PIP2 Window Scroll End Address Register 0          */
    { REG02B6_PIP2_SCROLL_EA_1, 0x000F }, /* PIP2 Window Scroll End Address Register 1          */
    { REG02B8_PIP2_SADDR_0, 4096 },       /* PIP2 Window 1 Display Start Address Register 0     */
    { REG02BA_PIP2_SADDR_1, 14 },         /* PIP2 Window 1 Display Start Address Register 1     */
    { REG02BC_PIP2_STRIDE, 640 },         /* PIP2 Window 1 Line Address Offset Register         */
    { REG02BE_PIP2_HORZ_SIZE, 640 },      /* PIP2 Window 1 Image Horizontal Size Register       */
    { REG02C0_PIP2_VERT_SIZE, 128 },      /* PIP2 Window 1 Image Vertical Size Register         */
    
    { REG024A_MAIN2_SADDR_0, 0x0000 },    /* Main Window 2 Display Start Address Register 0     */
    { REG024C_MAIN2_SADDR_1, 0x0000 },    /* Main Window 2 Display Start Address Register 1     */
    { REG024E_MAIN2_STRIDE, 0x0002 },     /* Main Window 2 Line Address Offset Register         */
    { REG0250_MAIN2_HORZ_SIZE, 0x0000 },   /* Main Window 2 Image Horizontal Size Register       */
    { REG0252_MAIN2_VERT_SIZE, 0x0000 },   /* Main Window 2 Image Vertical Size Register         */
  };

static const s1d_regs_t display_mode_0[] =
  {
    { REG0188_HWC_WR_HSIZE, 960 },
    { REG018A_HWC_WR_VSIZE, 480 },
    { REG0186_HWC_WR_ADDR_OFF, 960 },
    { REG0184_MEM_SADDR_1, 0 },
    { REG0182_MEM_SADDR_0, 0 },
    { REG0180_HOST_CONFIG, 0x0000 },
    { REG018C_MEM_ACCESS_PORT, 0x0000 },
  };

static const s1d_regs_t display_mode_1[] =
  {
    { REG0188_HWC_WR_HSIZE, 960 },
    { REG018A_HWC_WR_VSIZE, 640 },
    { REG0186_HWC_WR_ADDR_OFF, 0x0000 },
    { REG0184_MEM_SADDR_1, 0 },
    { REG0182_MEM_SADDR_0, 2558 },
    { REG0180_HOST_CONFIG, 0x0002 },
    { REG018C_MEM_ACCESS_PORT, 0x0000 },
  };

static const s1d_regs_t display_mode_2[] =
  {
    { REG0188_HWC_WR_HSIZE, 960 },
    { REG018A_HWC_WR_VSIZE, 640 },
    { REG0186_HWC_WR_ADDR_OFF, 960 },
    { REG0184_MEM_SADDR_1, 9 },
    { REG0182_MEM_SADDR_0, 24574 },
    { REG0180_HOST_CONFIG, 0x0004 },
    { REG018C_MEM_ACCESS_PORT, 0x0000 },
  };

static const s1d_regs_t display_mode_3[] =
  {
    { REG0188_HWC_WR_HSIZE, 960 },
    { REG018A_HWC_WR_VSIZE, 640 },
    { REG0186_HWC_WR_ADDR_OFF, 960 },
    { REG0184_MEM_SADDR_1, 9 },
    { REG0182_MEM_SADDR_0, 23614 },
    { REG0180_HOST_CONFIG, 0x0004 },
    { REG018C_MEM_ACCESS_PORT, 0x0000 },
  };

static const int red_offset = 11;
static const int red_mask = 0x001f;
static const int red_shift = 3;
static const int green_offset = 5;
static const int green_mask = 0x003f;
static const int green_shift = 2;
static const int blue_offset = 0;
static const int blue_mask = 0x001f;
static const int blue_shift = 3;

inline color_t make_gdi_pixel(uint16_t pixel)
  {
  color_t red = ((pixel >> red_offset) & red_mask) << red_shift;
  color_t blue = ((pixel >> blue_offset) & blue_mask) << blue_shift;
  color_t green = ((pixel >> green_offset) & green_mask) << green_shift;
  return rgb(red, green, blue);
  }

inline color_t get_framebuffer_pixel(const uint16_t *src)
  {
  return make_gdi_pixel(*src);
  }

inline uint16_t make_framebuffer_pixel(color_t color)
  {
  uint16_t pixel = 0;
  pixel = (red(color) >> red_shift) << red_offset;
  pixel |= (blue(color) >> blue_shift) << blue_offset;
  pixel |= (green(color) >> green_shift) << green_offset;
  
  return pixel;
  }

inline void set_framebuffer_pixel(uint16_t *dest, color_t color)
  {
  *dest = make_framebuffer_pixel(color);
  }

static void send_s1d13l02_value(uint16_t addr, uint16_t value)
  {
  *(uint16_t *)(S1D13L02_INDEX_REG) = addr;
  *(uint16_t *)(S1D13L02_DATA_REG) = value;
  }

static void send_s1d13l02_values(const s1d_regs_t *values, size_t num_values)
  {
	size_t i;
	S1D_INDEX reg;
	S1D_VALUE value;

	for (i = 0; i < num_values; i++)
  	{
		reg	 = s1d13l02_init_values[i].Index;
		value = s1d13l02_init_values[i].Value;
		
		if (reg == S1D_REGDELAYOFF || reg == S1D_REGDELAYON || reg == S1D_REGDELAYPLL)
      event_t::lock_t butler(dma7_irq, value);
		else
      send_s1d13l02_value(reg, value);
    }
  }

static int orientation;
typedef struct _s1d13l02_display_t
  {
  canvas_t canvas;
  // true if can be freed 
  bool can_free;
  uint16_t *buffer;
  } s1d13l02_canvas_t;
  
static s1d13l02_canvas_t displays[NUM_DISPLAYS];

/**
 * Allocate a pixel buffer
 * @param size  Number of pixels
 * @return 
 */
uint16_t *framebuffer_malloc(size_t size)
  {
  
  }

result_t framebuffer_free(canvas_t *canvas)
  {
  
  }

static color_t bsp_get_pixel(canvas_t *canvas, const point_t *src);
static void bsp_set_pixel(canvas_t *canvas, const point_t *dest, color_t color);
static void bsp_fast_fill(canvas_t *canvas, const rect_t *dest, color_t fill_color, raster_operation rop);
static void bsp_fast_line(canvas_t *canvas, const point_t *p1, const point_t *p2, color_t fill_color, raster_operation rop);
static void bsp_fast_copy(canvas_t *canvas, const point_t *dest, const canvas_t *src_canvas, const rect_t *src, raster_operation rop);
static color_t bsp_execute_rop(color_t src, color_t dst, raster_operation rop);

static result_t init_canvas(s1d13l02_canvas_t *canvas,
                              gdi_dim_t dx,
                              gdi_dim_t dy,
                              bool can_free)
  {
  uint16_t *buffer = framebuffer_malloc(dx * dy);
  
  if(buffer == 0)
    return e_not_enough_memory;
  
  canvas->canvas.version = sizeof(s1d13l02_canvas_t);
  canvas->buffer = buffer;
  canvas->can_free = can_free;
  canvas->canvas.metrics.bits_per_pixel = 16;
  canvas->canvas.metrics.palette = 0;
  canvas->canvas.metrics.row_size = dx;
  canvas->canvas.metrics.screen_x = dx;
  canvas->canvas.metrics.screen_y = dy;
  
  canvas->canvas.get_pixel = bsp_get_pixel;
  canvas->canvas.set_pixel = bsp_set_pixel;
  canvas->canvas.fast_fill = bsp_fast_fill;
  canvas->canvas.fast_line = bsp_fast_line;
  canvas->canvas.fast_copy = bsp_fast_copy;
  canvas->canvas.execute_rop = bsp_execute_rop;
  
  return s_ok;
  }


result_t display_init(int orientation)
  {
  size_t i;
  memset(displays, 0, sizeof(s1d13l02_canvas_t) * NUM_DISPLAYS);
  
  init_canvas(&displays[0].canvas, 640, 480, false);
  init_canvas(&displays[1].canvas, 320, 480, false);
  init_canvas(&displays[2].canvas, 120, 480, false);
  
  set_display_orientation(orientation);
  
  // Start the display controller
  // we send the hardware init to the display and then copy a splash screen
  // as quickly as we can.
  
	// Initialize controller registers
  send_s1d13l02_values(s1d13l02_init_values, num_elements(s1d13l02_init_values));
  send_s1d13l02_value(REG0180_HOST_CONFIG, 0x0001);   // set write mode
  
  // we send the splash screen next.  Sent directly to the registers
  send_s1d13l02_values(display_mode_0, num_elements(display_mode_0));
  
  size_t num_pixels = splash_bitmap.bitmap_height * splash_bitmap.bitmap_width;
  for(i = 0; i < num_pixels; i++)
    set_framebuffer_pixel((uint16_t *)(S1D13L02_DATA_REG),
                          make_framebuffer_pixel(splash_bitmap.pixels[i]))

  // Wait for the HWC to become not busy any more.
  while(*(uint16_t *)(S1D13L02_STATUS_REG) & 0x0100);
  }

int get_display_orientation()
  {
  return orientation;
  }
  
result_t set_display_orientation(int orientation)
  {
  int i;
 
  switch(orientation)
    {
    case 0 :
      displays[0].canvas.metrics.bits_per_pixel = 16;
      displays[0].canvas.metrics.row_size = 640;
      displays[0].canvas.metrics.screen_x = 640;
      displays[0].canvas.metrics.screen_y = 480;
      
      displays[1].canvas.metrics.bits_per_pixel = 16;
      displays[1].canvas.metrics.row_size = 640;
      displays[1].canvas.metrics.screen_x = 640;
      displays[1].canvas.metrics.screen_y = 240;
      
      displays[2].canvas.metrics.bits_per_pixel = 16;
      displays[2].canvas.metrics.row_size = 480;
      displays[2].canvas.metrics.screen_x = 480;
      displays[2].canvas.metrics.screen_y = 120;
      break;
    case 90 :
      displays[0].canvas.metrics.bits_per_pixel = 16;
      displays[0].canvas.metrics.row_size = 480;
      displays[0].canvas.metrics.screen_x = 480;
      displays[0].canvas.metrics.screen_y = 640;
      
      displays[1].canvas.metrics.bits_per_pixel = 16;
      displays[1].canvas.metrics.row_size = 480;
      displays[1].canvas.metrics.screen_x = 480;
      displays[1].canvas.metrics.screen_y = 320;
      
      displays[2].canvas.metrics.bits_per_pixel = 16;
      displays[2].canvas.metrics.row_size = 480;
      displays[2].canvas.metrics.screen_x = 480;
      displays[2].canvas.metrics.screen_y = 120;
      break;
    case 180 :
      return e_not_implemented;
    case 270 :
      return e_not_implemented;
    default :
      return e_bad_parameter;
    }
  
  return s_ok;
  }

size_t display_count()
  {
  return NUM_DISPLAYS;
  }

static const char *names[] = {
  "main",
  "menu",
  "alert"
  };

result_t display_metrics(size_t num, metrics_t *metrics, const char **name)
  {
  if(num > NUM_DISPLAYS)
    return e_bad_parameter;
  
  memcpy(metrics, &displays[num].canvas.metrics, sizeof(metrics_t));
  if(name != 0)
     *name = names[num];
  
  return s_ok;
  }

result_t display_open(size_t num, handle_t *hndl)
  {
  if(num > NUM_DISPLAYS)
    return e_bad_parameter;
  
  hndl = &displays[num].canvas;
  
  return s_ok;
  }

result_t bsp_canvas_create_rect(const extent_t *size, handle_t *hndl)
  {
  size_t pixels = size->dx * size->dy;
  s1d13l02_canvas_t *canvas;
  
  canvas = malloc(sizeof(s1d13l02_canvas_t));
  
  if(canvas == 0)
    return e_not_enough_memory;
  
  if(init_canvas(canvas, size->dx, size->dy, true)!= s_ok)
    {
    free(canvas);
    return e_not_enough_memory;
    }
 
  *hndl = canvas;
  
  return s_ok;
  }

result_t bsp_canvas_create_bitmap(const bitmap_t *bitmap, handle_t *hndl)
  {
  size_t pixels = bitmap->bitmap_width * bitmap->bitmap_height;
  canvas_t *canvas;
  rect_t rect;
  
  canvas = malloc(sizeof(s1d13l02_canvas_t));
  
  if(canvas == 0)
    return e_not_enough_memory;
  
  if(init_canvas(canvas, bitmap->bitmap_width, bitmap->bitmap_height, true) != s_ok)
    {
    free(canvas);
    return e_not_enough_memory;
    }
  
  *hndl = canvas;
  rect.left = 0;
  rect.top = 0;
  rect.bottom = bitmap->bitmap_height;
  rect.right = bitmap->bitmap_width;
  
 
  return canvas_pattern_blt(canvas, &rect, bitmap, &rect, rop_srccopy);
  }

result_t bsp_canvas_close(handle_t hndl)
  {
  s1d13l02_canvas_t *canvas = (s1d13l02_canvas_t *)hndl;
  if(canvas->canvas.version != sizeof(s1d13l02_canvas_t))
    return e_invalid_handle;
  
  if(!canvas->can_free)
    return e_invalid_handle;
  
  if(framebuffer_free(canvas->buffer)!= 0)
    return e_invalid_handle;
  
  free(hndl);
  
  return s_ok;
  }

color_t bsp_execute_rop(canvas_t *canvas,
                        color_t src,
                        color_t dst,
                        raster_operation rop)
  {
  switch(rop)
    {
  case color_hollow:
    return dst;
  case rop_r2_masknotpen:			// dpna
    return dst & ~src;
  case rop_r2_maskpennot:			// pdna
    return ~(dst & src);
  case rop_r2_notxorpen:				// dpxn
    return ~(dst ^ src);
  case rop_r2_nop:							// d
    return dst;
  case rop_r2_mergepennot:			// pdno
    return ~(dst | src);
  case rop_patcopy:						// dest = pattern
  case rop_srccopy:						// dest = source
  case rop_r2_copypen:					// p
  case rop_r2_last:
    return src;
  case rop_patpaint:						// dest = dpsnoo
  case rop_srcpaint:						// dest = source or dest
  case rop_r2_notmergepen:			// dpon
  case rop_r2_mergepen:				// dpo
    return src | dst;
  case rop_srcand:							// dest = source and dest
  case rop_r2_maskpen:					// dpa
    return src & dst;
  case rop_patinvert:					// dest = pattern xor dest
  case rop_srcinvert:					// dest = source xor dest
  case rop_r2_xorpen:					// dpx
    return src ^ dst;
  case rop_r2_notmaskpen:			// dpan
  case rop_srcerase:						// dest = source and (not dest )
    return src & ~dst;
  case rop_notsrccopy:					// dest = (not source)
  case rop_r2_notcopypen:			// pn
    return ~src;
  case rop_notsrcerase:				// dest = (not src) and (not dest)
    return ~src & ~dst;
  case rop_mergecopy:					// dest = (source and pattern)
    return src & dst;
  case rop_r2_mergenotpen:			// dpno
  case rop_mergepaint:					// dest = (not source) or dest
    return ~src | dst;
  case rop_dstinvert:					// dest = (not dest)
  case rop_r2_not:							// dn
    return ~dst;
  case rop_r2_black:						//  0
  case rop_blackness:					// dest = black
    return color_black;
  case rop_whiteness:					// dest = white
    return color_white;
    }
  return 0;
  }

static color_t bsp_get_pixel(canvas_t *canvas,
                             const point_t *src)
  {
  s1d13l02_canvas_t *canvas = (s1d13l02_canvas_t *)hndl;
  if(canvas->canvas.version != sizeof(s1d13l02_canvas_t))
    return color_hollow;
  
  }

static void bsp_set_pixel(canvas_t *canvas,
                          const point_t *dest,
                          color_t color)
  {
  s1d13l02_canvas_t *canvas = (s1d13l02_canvas_t *)hndl;
  if(canvas->canvas.version != sizeof(s1d13l02_canvas_t))
    return;
  
  
  }

static void bsp_fast_fill(canvas_t *canvas,
                          const rect_t *dest,
                          color_t fill_color,
                          raster_operation rop)
  {
  s1d13l02_canvas_t *canvas = (s1d13l02_canvas_t *)hndl;
  if(canvas->canvas.version != sizeof(s1d13l02_canvas_t))
    return;
  }

static void bsp_fast_line(canvas_t *canvas,
                          const point_t *p1,
                          const point_t *p2,
                          color_t fill_color,
                          raster_operation rop)
  {
  s1d13l02_canvas_t *canvas = (s1d13l02_canvas_t *)hndl;
  if(canvas->canvas.version != sizeof(s1d13l02_canvas_t))
    return;
  }

static void bsp_fast_copy(canvas_t *canvas,
                          const point_t *dest,
                          const canvas_t *src_canvas,
                          const rect_t *src,
                          raster_operation rop)
  {
  s1d13l02_canvas_t *canvas = (s1d13l02_canvas_t *)hndl;
  if(canvas->canvas.version != sizeof(s1d13l02_canvas_t))
    return;
  }

#if 0 
class s1d13l02_worker_t : public thread_t
  {
  public:
    s1d13l02_worker_t();
    
    void set_screen(s1d13l02_screen_t *s);
    
    void vsync();
  private:
    enum framebuffer_draw_state
      {
      fbds_checkredraw,
      fbds_drawsync,
      fbds_draw,
      };
    framebuffer_draw_state _state = fbds_checkredraw;
    
    s1d13l02_worker_t *_main_screen;
    s1d13l02_worker_t *_pip1_screen;
    s1d13l02_worker_t *_pip2_screen;

    static uint32_t do_run(void *);
    uint32_t run();
  event_t _vsync_irq;
  };
  

static event_t dma7_irq;        // channel 7 is the framebuffer->peripheral

/**
 * Allocate a buffer in the 16bit external buffer.  
 * @param length  Number of pixels to allocate
 * @return 
 */
static uint16_t *alloc_framebuffer(size_t length)
  {
  if(framebuffer_free < length)
    return 0;
  
  uint16_t *buffer = framebuffer_base + framebuffer_free;
  
  framebuffer_free -= length;
  
  return buffer;
  }


static size_t transfer_size;
static int display_mode;        // set to the display mode of the screen
static critical_section_t dma_cs;

static void dma7_complete()
  {
  // the transfer is completed see if any more to do
  if(transfer_size == 0)
    dma7_irq.set();
  else
    {
    // next block....
    DCH7SSA += 65536;
    
    size_t next_transfer = transfer_size > 65536 ? 65536 : transfer_size;
    
    // set the CHCSIZ register
    DCH7CSIZ = next_transfer;
    
    transfer_size -= next_transfer;
    
    // and start the channel again
    DCH7ECON=0x00000080; // set CFORCE to 1
    }
  }

inline unsigned int VirtToPhys(const void* p)
{
return (int)p<0?((int)p&0x1fffffffL):(unsigned int)((unsigned char*)p+0x40000000L);
}


/**
 * Copy a block of rows from the src_ptr in video buffer to the offset
 * dst_ptr in the framebuffer of the S1D13L02
 * @param src_ptr address in memory
 * @param dst_ptr offset into S1D13L02
 * @param num_rows  number of full rows to copy
 */
static void bit_blt_image(void *src_ptr, uint32_t dst_ptr, size_t num_rows)
  {
  critical_section_t::lock_t lock(dma_cs);
  
  s1d_regs_t regs[7];
  
  // if the dma from foreground buffer to display is complete then
  // we bitblt from background to foreground buffer
  switch(display_mode)
    {
    default :
    case 0 :
      memcpy(regs, display_mode_0, sizeof(display_mode_0)); 
      break;
    case 90:
      memcpy(regs, display_mode_1, sizeof(display_mode_1)); 
      break;
    case 180 :
      memcpy(regs, display_mode_2, sizeof(display_mode_2)); 
      break;
    case 270 :            
      memcpy(regs, display_mode_3, sizeof(display_mode_3)); 
      break;
    }

  transfer_size = 480 * num_rows * 2;

  // set up the destination offset into the buffer.

  send_s1d13l02_values(regs, 7);

  // this will take approx 32 msec

  // now the display controller uses the dma controller channel 7
  // to transfer the framebuffer to the S1D13L02
  DCH7SSA=VirtToPhys(src_ptr); // transfer source physical address
  DCH7DSA=VirtToPhys((void *)S1D13L02_DATA_REG); // transfer destination physical address
  DCH7SSIZ = 0;   // source size 65536 bytes
  DCH7DSIZ=2;     // destination size 2 bytes
  DCH7CSIZ=0;     // transfer size, 65536
  DCH7INT = 0x00800000; // clear interrupts and enable source done
  DCH7CONSET = 0x80; // turn channel on
  DCH7ECONSET=0x00000080; // set CFORCE to 1

        // wait till done.
  event_t::lock_t wait_lock(dma7_irq);
  }

#endif