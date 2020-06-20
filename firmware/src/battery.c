#include "includes.h"

static battery_evt_handler_t _evt_handler = NULL;

static battery_status_t _battery_state = {0};

static void __INLINE saadc_init(void);
static uint8_t __INLINE battery_mv_to_percent(const uint16_t mvolts);
static uint16_t __INLINE battery_get_ADC_mV_value(void);
static __INLINE enum BATTERY_STATUS_STATE battery_to_state(const battery_status_t *battery_state);


void battery_init(battery_evt_handler_t evt_handler)
{
    _evt_handler = evt_handler;

    saadc_init();
    NRF_LOG_INFO("SAADC HAL simple example started.");

    battery_run();
}

void battery_run(void)
{
    battery_status_t current_battery_state;

    current_battery_state.mv = battery_get_ADC_mV_value();
    current_battery_state.proc = battery_mv_to_percent(current_battery_state.mv);
    current_battery_state.state = battery_to_state(&current_battery_state);
    NRF_LOG_INFO("%d, %d, %d", current_battery_state.mv, current_battery_state.proc, current_battery_state.state);

    if(current_battery_state.proc != _battery_state.proc || current_battery_state.state != _battery_state.state)
    {
        _battery_state = current_battery_state;
        if(_evt_handler)
        {
            _evt_handler(_battery_state);
        }
    }
}



void saadc_init(void)
{
    ret_code_t err_code;

    nrf_saadc_channel_config_t channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);

    channel_config.gain = NRF_SAADC_GAIN1_5;
    channel_config.acq_time = NRF_SAADC_ACQTIME_3US;

    err_code = nrf_drv_saadc_init(NULL, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);
}

uint16_t battery_get_ADC_mV_value(void)
{
    nrf_saadc_value_t ADCvalue = 0;
    nrfx_err_t err_code;
    err_code = nrf_drv_saadc_sample_convert(0, &ADCvalue);
    APP_ERROR_CHECK(err_code);

    uint16_t batteryVoltage = ADCvalue * 1000 / ((1/5.0f)/(0.6f)*4095);

    return batteryVoltage;
}

//2.2M and 805K

/** @brief Function for converting the input voltage (in milli volts) into percentage of 3.0 Volts.
 *
 *  @details The calculation is based on a linearized version of the battery's discharge
 *           curve. 3.0V returns 100% battery level. The limit for power failure is 2.1V and
 *           is considered to be the lower boundary.
 *
 *           The discharge curve for CR2032 is non-linear. In this model it is split into
 *           4 linear sections:
 *           - Section 1: 3.0V - 2.9V = 100% - 42% (58% drop on 100 mV)
 *           - Section 2: 2.9V - 2.74V = 42% - 18% (24% drop on 160 mV)
 *           - Section 3: 2.74V - 2.44V = 18% - 6% (12% drop on 300 mV)
 *           - Section 4: 2.44V - 2.1V = 6% - 0% (6% drop on 340 mV)
 *
 *           These numbers are by no means accurate. Temperature and
 *           load in the actual application is not accounted for!
 *
 *  @param[in] mvolts The voltage in mV
 *
 *  @return    Battery level in percent.
*/
static __INLINE uint8_t battery_mv_to_percent(const uint16_t mvolts)
{
    uint8_t battery_level;

    if (mvolts >= 3000)
    {
        battery_level = 100;
    }
    else if (mvolts > 2900)
    {
        battery_level = 100 - ((3000 - mvolts) * 58) / 100;
    }
    else if (mvolts > 2740)
    {
        battery_level = 42 - ((2900 - mvolts) * 24) / 160;
    }
    else if (mvolts > 2440)
    {
        battery_level = 18 - ((2740 - mvolts) * 12) / 300;
    }
    else if (mvolts > 2100)
    {
        battery_level = 6 - ((2440 - mvolts) * 6) / 340;
    }
    else
    {
        battery_level = 0;
    }

    return battery_level;
}

static __INLINE enum BATTERY_STATUS_STATE battery_to_state(const battery_status_t *battery_state)
{
    enum BATTERY_STATUS_STATE state;

    switch(battery_state->proc)
    {
        case 11 ... 100:
            state = BATTERY_STATUS_STATE_DISCHARGING;
            break;
        case 5 ... 10:
            state = BATTERY_STATUS_STATE_WARNING;
            break;
        case 0 ... 4:
            state = BATTERY_STATUS_STATE_DEPLETED;
            break;
    }

    return state;

}
