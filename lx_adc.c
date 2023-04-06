#include "lx_adc.h"
#include "em_device.h"
#include "em_chip.h"
#include "em_core.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_iadc.h"
#include "app_log.h"
//#include "LXDrive/debugLog/debugLog.h"
//#include "cmsis_os2.h"
//#include "sl_cmsis_os2_common.h"

// 定义ADC最大时钟
#define ADCFREQ 16000000
// 设置CLK_ADC为1 MHz(为最短的IADC转换/通电时间设置为max)
#define SRCCLKFREQ 10000000
#define CLKFREQ 10000000

// Set IADC timer cycles
#define TIMER_CYCLES 50000 // 50000 => 100   samples/second
                           // 5000  => 1000  samples/second
                           // 1000  => 5000  samples/second
                           // 500   => 10000 samples/second
                           // 200   => 25000 samples/second
// 当更改IADC输入端口/引脚时，请确保相应地更改xBUSALLOC宏
//  #define IADC_INPUT_0_BUS          CDBUSALLOC
//  #define IADC_INPUT_0_BUSALLOC     GPIO_CDBUSALLOC_CDEVEN0_ADC0
//  #define IADC_INPUT_1_BUS          CDBUSALLOC
//  #define IADC_INPUT_1_BUSALLOC     GPIO_CDBUSALLOC_CDODD0_ADC0
// #define IADC_INPUT_0_BUS          CDBUSALLOC
// #define IADC_INPUT_0_BUSALLOC     GPIO_CDBUSALLOC_CDEVEN0_ADC0
#define IADC_X_PORT gpioPortC
#define IADC_X_PIN 0
#define IADC_X_BUS CDBUSALLOC
#define IADC_X_BUSALLOC GPIO_CDBUSALLOC_CDODD0_ADC0

#define IADC_Y_PORT gpioPortC
#define IADC_Y_PIN 1
#define IADC_Y_BUS CDBUSALLOC
#define IADC_Y_BUSALLOC GPIO_CDBUSALLOC_CDODD0_ADC0

#define IADC_Z_PORT gpioPortC
#define IADC_Z_PIN 2
#define IADC_Z_BUS CDBUSALLOC
#define IADC_Z_BUSALLOC GPIO_CDBUSALLOC_CDODD0_ADC0

#define IADC_Rx_PORT gpioPortC
#define IADC_Rx_PIN 3
#define IADC_Rx_BUS CDBUSALLOC
#define IADC_Rx_BUSALLOC GPIO_CDBUSALLOC_CDODD0_ADC0

static lx_adc_t g_adc;

void lx_adc_init()
{
  IADC_Init_t iadcinit = IADC_INIT_DEFAULT;
  IADC_InitSingle_t iadcinitsingle = IADC_INITSINGLE_DEFAULT;
  IADC_AllConfigs_t iadcallconfig = IADC_ALLCONFIGS_DEFAULT;
  IADC_SingleInput_t singleinput = IADC_SINGLEINPUT_DEFAULT;

  CMU_ClockEnable(cmuClock_IADC0, true); // 使能IADC时钟

  IADC_reset(IADC0); // 重启IADC，重置IADC重置配置，如果它已经被修改

  // 配置IADC时钟源为EM2
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO); // 20MHz FSRCO：快速启动RC振荡器

  iadcinit.warmup = iadcWarmupNormal;
  // 在这里设置HFSCLK预缩放值
  iadcinit.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, SRCCLKFREQ, 0);

  // 默认情况下，扫描和单次转换都使用配置0
  // 使用未缓冲的AVDD作为引用
  // iadcallconfig.configs[0].reference = iadcCfgReferenceVddx;//内部3.3V参考电压

  // 划分CLK_SRC_ADC来设置CLK_ADC频率
  // 默认的OSR是2x，转换时间= ((4 * OSR + 2) / fCLK_ADC
  // iadcallconfig.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0, CLKFREQ, 0, iadcCfgModeNormal, iadcinit.srcClkPrescale);

  iadcallconfig.configs[0].reference = iadcCfgReferenceVddx;
  iadcallconfig.configs[0].vRef = 3300;
  iadcallconfig.configs[0].analogGain = iadcCfgAnalogGain1x;

  // Divides CLK_SRC_ADC to set the CLK_ADC frequency
  iadcallconfig.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                    CLKFREQ,
                                                                    0,
                                                                    iadcCfgModeNormal,
                                                                    iadcinit.srcClkPrescale);

  singleinput.posInput = IADC_portPinToPosInput(IADC_X_PORT, IADC_X_PIN); // 选择相应的GPIO口作为IADC口
  singleinput.negInput = iadcNegInputGnd;

  // 为ADC0输入分配模拟总线
  GPIO_PinModeSet(IADC_X_PORT, IADC_X_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(IADC_Y_PORT, IADC_Y_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(IADC_Z_PORT, IADC_Z_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(IADC_Rx_PORT, IADC_Rx_PIN, gpioModeDisabled, 0);
  GPIO->IADC_X_BUS |= IADC_X_BUSALLOC;

  // IADC初始化
  IADC_init(IADC0, &iadcinit, &iadcallconfig);
  IADC_initSingle(IADC0, &iadcinitsingle, &singleinput);

  g_adc = LX_ADC_NULL;
}

void lx_adc_start(lx_adc_t dev)
{
  IADC_SingleInput_t singleinput = IADC_SINGLEINPUT_DEFAULT;
  g_adc = dev;
  if (dev == LX_ADC_X)
  {
    GPIO->IADC_X_BUS |= IADC_X_BUSALLOC;
    singleinput.posInput = IADC_portPinToPosInput(IADC_X_PORT, IADC_X_PIN); // 选择相应的GPIO口作为IADC口
    singleinput.negInput = iadcNegInputGnd;
    IADC_updateSingleInput(IADC0, &singleinput);
    IADC_command(IADC0, iadcCmdStartSingle);
  }
  else if (dev == LX_ADC_Y)
  {
    GPIO->IADC_Y_BUS |= IADC_Y_BUSALLOC;
    singleinput.posInput = IADC_portPinToPosInput(IADC_Y_PORT, IADC_Y_PIN);
    singleinput.negInput = iadcNegInputGnd;
    IADC_updateSingleInput(IADC0, &singleinput);
    IADC_command(IADC0, iadcCmdStartSingle);
  }
  else if (dev == LX_ADC_Z)
  {
    GPIO->IADC_Z_BUS |= IADC_Z_BUSALLOC;
    singleinput.posInput = IADC_portPinToPosInput(IADC_Z_PORT, IADC_Z_PIN);
    singleinput.negInput = iadcNegInputGnd;
    IADC_updateSingleInput(IADC0, &singleinput);
    IADC_command(IADC0, iadcCmdStartSingle);
  }
  else if (dev == LX_ADC_Rx)
  {
    GPIO->IADC_Rx_BUS |= IADC_Rx_BUSALLOC;
    singleinput.posInput = IADC_portPinToPosInput(IADC_Rx_PORT, IADC_Rx_PIN);
    singleinput.negInput = iadcNegInputGnd;
    IADC_updateSingleInput(IADC0, &singleinput);
    IADC_command(IADC0, iadcCmdStartSingle);
  }
  else
  {
    return;
  }
}

void lx_adc_wait(lx_adc_t dev)
{
  if (dev == g_adc)
  {
    while ((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK | _IADC_STATUS_SINGLEFIFODV_MASK)) != IADC_STATUS_SINGLEFIFODV)
      {
        //osThreadYield();
      }
  }
  else
  {
      app_log("Error ADC dev type");
  }
}
uint32_t lx_adc_get(lx_adc_t dev)
{
  IADC_Result_t res;
  if (dev == g_adc)
  {
    res = IADC_pullSingleFifoResult(IADC0);
    return res.data;
  }
  else
  {
      app_log("Error ADC dev type");
    return 0;
  }
}
