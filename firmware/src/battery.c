#include "includes.h"

#define NRF_LOG_MODULE_NAME battery
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_INFO
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

#define BATTERY_LEVEL_MEAS_INTERVAL APP_TIMER_TICKS(60000) /**< Battery level measurement interval (ticks). */

APP_TIMER_DEF(m_battery_timer_id); /**< Battery timer. */

static battery_evt_handler_t _evt_handler = NULL;

static battery_status_t _battery_state = {0};
uint32_t m_adc_evt_counter = 1;
bool m_saadc_calibrate = false;
#define SAADC_CALIBRATION_INTERVAL 5
static nrf_saadc_value_t       m_buffer_pool[2][1];

static void __INLINE saadc_init(void);
static uint8_t __INLINE battery_mv_to_percent(const uint16_t mvolts);
static uint16_t __INLINE battery_get_ADC_mV_value(void);
static __INLINE enum BATTERY_STATUS_STATE battery_to_state(const battery_status_t *battery_state);

static void battery_level_meas_timeout_handler(void *p_context);
void calibrate_saadc(void);

void battery_init(battery_evt_handler_t evt_handler)
{
    ret_code_t err_code;
    _evt_handler = evt_handler;

    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    saadc_init();
    NRF_LOG_INFO("SAADC HAL simple example started.");

    ble_battery_init();

    battery_run();
}

void battery_run(void)
{
    battery_status_t current_battery_state;

    current_battery_state.mv = battery_get_ADC_mV_value();
    current_battery_state.proc = battery_mv_to_percent(current_battery_state.mv);
    current_battery_state.state = battery_to_state(&current_battery_state);
    NRF_LOG_INFO("mV: %d, proc: %d%%, state: %d", current_battery_state.mv, current_battery_state.proc, current_battery_state.state);

    if(current_battery_state.proc != _battery_state.proc || current_battery_state.state != _battery_state.state)
    {
        _battery_state = current_battery_state;
        ble_battery_bas_update(_battery_state);
        if(_evt_handler)
        {
            _evt_handler(_battery_state);
        }
    }
}

static void battery_level_meas_timeout_handler(void *p_context)
{
    battery_run();
}

void saadc_run_calibration(void *p_event_data, uint16_t event_size)
{
    //calibrate_saadc();

    nrf_drv_saadc_abort();                                  // Abort all ongoing conversions. Calibration cannot be run if SAADC is busy
    NRF_LOG_INFO("SAADC calibration starting...");    //Print on UART

    while(nrf_drv_saadc_calibrate_offset() != NRF_SUCCESS); //Trigger calibration task
}

void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    ret_code_t err_code;
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)                                //Capture offset calibration complete event
    {
        NRF_LOG_INFO("ADC event number: %d\r\n",(int)m_adc_evt_counter);        //Print the event number on UART

        for (int i = 0; i < p_event->data.done.size; i++)
        {
            NRF_LOG_INFO("%d\r\n", p_event->data.done.p_buffer[i]);             //Print the SAADC result on UART
        }
        
        if((m_adc_evt_counter % SAADC_CALIBRATION_INTERVAL) == 0)               //Evaluate if offset calibration should be performed. Configure the SAADC_CALIBRATION_INTERVAL constant to change the calibration frequency
        {
            app_sched_event_put(NULL, 0, saadc_run_calibration);                // Set flag to trigger calibration in main context when SAADC is stopped
        }
        else
        {
            err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, ARRAY_SIZE(m_buffer_pool[0]));             //Set buffer so the SAADC can write to it again. 
            APP_ERROR_CHECK(err_code);
        }
        
        m_adc_evt_counter++;
  
    }
    else if (p_event->type == NRF_DRV_SAADC_EVT_CALIBRATEDONE)
    {
        err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], ARRAY_SIZE(m_buffer_pool[0]));             //Set buffer so the SAADC can write to it again. 
        APP_ERROR_CHECK(err_code);
        //err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], ARRAY_SIZE(m_buffer_pool[1]));             //Need to setup both buffers, as they were both removed with the call to nrf_drv_saadc_abort before calibration.
        //APP_ERROR_CHECK(err_code);
        
        NRF_LOG_INFO("SAADC calibration complete ! \r\n");                                              //Print on UART
    }
}

void saadc_init(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t channel_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);
    channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;    // 0.6V internal
    channel_config.gain = NRF_SAADC_GAIN1_6;                    // 0.6/(1/6) is 3.6 max voltage. We have 3.2V for Vbat 4.2
    channel_config.acq_time = SAADC_CH_CONFIG_TACQ_10us; 
    channel_config.burst = NRF_SAADC_BURST_ENABLED;

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], ARRAY_SIZE(m_buffer_pool[0]));             //Set buffer so the SAADC can write to it again. 
    APP_ERROR_CHECK(err_code);
    //err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], ARRAY_SIZE(m_buffer_pool[1]));             //Need to setup both buffers, as they were both removed with the call to nrf_drv_saadc_abort before calibration.
    //APP_ERROR_CHECK(err_code);
}


void calibrate_saadc(void)
{
    nrfx_err_t nrfx_err_code = NRFX_SUCCESS;

    NRF_LOG_INFO("Calibration started");

    // Stop ADC
    nrf_saadc_int_disable(NRF_SAADC_INT_ALL);
    NRFX_IRQ_DISABLE(SAADC_IRQn);
    nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);

    // Wait for ADC being stopped.
    bool result;
    NRFX_WAIT_FOR(nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED), 10000, 0, result);
    NRFX_ASSERT(result);

    // Start calibration
    NRFX_IRQ_ENABLE(SAADC_IRQn);
    nrfx_err_code = nrfx_saadc_calibrate_offset();
    APP_ERROR_CHECK(nrfx_err_code);
    while(nrfx_saadc_is_busy()){};
    
    // Stop ADC
    nrf_saadc_int_disable(NRF_SAADC_INT_ALL);
    NRFX_IRQ_DISABLE(SAADC_IRQn);
    nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);

    // Wait for ADC being stopped. 
    NRFX_WAIT_FOR(nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED), 10000, 0, result);
    NRFX_ASSERT(result);
    
    // Enable IRQ
    NRFX_IRQ_ENABLE(SAADC_IRQn);
    NRF_LOG_INFO("Calibration ended");

}

#define FLOAT_TO_INT(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))
uint16_t battery_get_ADC_mV_value(void)
{
    // nrf_saadc_value_t ADCvalue = 0;
    // nrfx_err_t err_code;
    // err_code = nrf_drv_saadc_sample_convert(0, &ADCvalue);
    // APP_ERROR_CHECK(err_code);


    // float adc_voltage = (ADCvalue * 3600.0f) / 16383.0f;
    // float battery_voltage = adc_voltage * 4200 / 3600;
    // NRF_LOG_INFO("ADC: %d", ADCvalue);

    // uint16_t batteryVoltage = FLOAT_TO_INT(battery_voltage); //ADCvalue * 1000 / ((1/5.0f)/(0.6f)*4095);

    // return batteryVoltage;
    nrf_drv_saadc_sample();
    return 4000;
}

float map(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// based on https://lygte-info.dk/info/BatteryChargePercent%20UK.html
static __INLINE uint8_t battery_mv_to_percent(const uint16_t mvolts)
{
    uint8_t battery_level;

    if (mvolts >= 4200)
    {
        battery_level = 100;
    }
    else if (mvolts > 4100)
    {
        battery_level = map(mvolts, 4100, 4200, 90, 100);
    }
    else if (mvolts > 4000)
    {
        battery_level = map(mvolts, 4000, 4100, 79, 90);
    }
    else if (mvolts > 3900)
    {
        battery_level = map(mvolts, 3900, 4000, 62, 79);
    }
    else if (mvolts > 3800)
    {
        battery_level = map(mvolts, 3800, 3900, 42, 62);
    }
    else if (mvolts > 3700)
    {
        battery_level = map(mvolts, 3700, 3800, 12, 42);
    }
    else if (mvolts > 3600)
    {
        battery_level = map(mvolts, 3600, 3700, 2, 12);
    }
    else if (mvolts > 3500)
    {
        battery_level = map(mvolts, 3500, 3600, 0, 2);
    }
    else
    {
        battery_level = 0;
    }

    return battery_level;
}

static __INLINE enum BATTERY_STATUS_STATE battery_to_state(const battery_status_t *battery_state)
{
    enum BATTERY_STATUS_STATE state = BATTERY_STATUS_STATE_UNKNOWN;

    switch(battery_state->proc)
    {
        case 26 ... 100:
            state = BATTERY_STATUS_STATE_DISCHARGING;
            break;
        case 11 ... 25:
            state = BATTERY_STATUS_STATE_WARNING;
            break;
        case 0 ... 10:
            state = BATTERY_STATUS_STATE_DEPLETED;
            break;
    }

    return state;

}
