#ifndef LX_BLESMARTKEY_LX_DRIVER_LX_ADC_LX_ADC_H_
#define LX_BLESMARTKEY_LX_DRIVER_LX_ADC_LX_ADC_H_

#include "em_common.h"

typedef enum lx_adc
{
  LX_ADC_NULL,
  LX_ADC_X,
  LX_ADC_Y,
  LX_ADC_Z,
  LX_ADC_Rx
} lx_adc_t;


void lx_adc_init();
void lx_adc_start(lx_adc_t dev);
void lx_adc_wait(lx_adc_t dev);
uint32_t lx_adc_get(lx_adc_t dev);

#endif /* LX_BLESMARTKEY_LX_DRIVER_LX_ADC_LX_ADC_H_ */
