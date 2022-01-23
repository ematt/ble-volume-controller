
#include "includes.h"

#define NRF_LOG_MODULE_NAME encoder
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_WARNING
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

#define ENC_PIN_A BSP_QSPI_IO2_PIN
#define ENC_PIN_B BSP_QSPI_IO3_PIN

const uint32_t DEBOUNCE_TIME_MS = 4;

static enc_evt_handler_t enc_evt_handler = NULL;
static const nrfx_timer_t m_timer2 = NRFX_TIMER_INSTANCE(2);

static volatile bool interrupPinAState = false;

// A vald CW or  CCW move returns 1, invalid returns 0.
int8_t read_rotary()
{
    static int8_t rot_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};
    static uint8_t prevNextCode = 0;
    static uint16_t store = 0;

    prevNextCode <<= 2;
    if (nrfx_gpiote_in_is_set(ENC_PIN_A))
        prevNextCode |= 0x02;
    if (nrfx_gpiote_in_is_set(ENC_PIN_B))
        prevNextCode |= 0x01;
    prevNextCode &= 0x0f;

    // If valid then store as 16 bit data.
    if (prevNextCode < sizeof(rot_enc_table) && rot_enc_table[prevNextCode])
    {
        store <<= 4;
        store |= prevNextCode;

        if ((store & 0xff) == 0x2b)
            return -1;
        if ((store & 0xff) == 0x17)
            return 1;
    }
    return 0;
}

void enc_pin_A_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    NRF_LOG_DEBUG("enc_pin_A_handler");
    interrupPinAState = nrfx_gpiote_in_is_set(ENC_PIN_A);

    nrfx_timer_enable(&m_timer2);
    nrf_drv_gpiote_in_event_disable(ENC_PIN_A);
    nrf_drv_gpiote_in_event_disable(ENC_PIN_B);
}

void enc_pin_B_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    NRF_LOG_DEBUG("enc_pin_B_handler");

    nrfx_timer_enable(&m_timer2);
    nrf_drv_gpiote_in_event_disable(ENC_PIN_A);
    nrf_drv_gpiote_in_event_disable(ENC_PIN_B);
}

void Timer_2_Interrupt_Handler(
    nrf_timer_event_t event_type,
    void *p_context)
{
    NRF_LOG_DEBUG("Timer_2_Interrupt_Handler");
    int8_t val;
    val = read_rotary();

    if (val)
    {
        if (val > 0)
        {
            enc_evt_handler(ENCODER_DIR_CCW);
        }
        else
        {
            enc_evt_handler(ENCODER_DIR_CW);
        }
    }

    nrf_drv_gpiote_in_event_enable(ENC_PIN_A, true);
    nrf_drv_gpiote_in_event_enable(ENC_PIN_B, true);
    nrfx_timer_disable(&m_timer2);
    return;

    bool pinAState = nrfx_gpiote_in_is_set(ENC_PIN_A);
    bool pinBState = nrfx_gpiote_in_is_set(ENC_PIN_B);

    if (interrupPinAState == pinAState)
    {
        if (pinAState)
        {
            if (pinBState)
            {
                enc_evt_handler(ENCODER_DIR_CCW);
            }
            else
            {
                enc_evt_handler(ENCODER_DIR_CW);
            }
        }
    }
    else 
        NRF_LOG_DEBUG("Debounce NOK");

    nrf_drv_gpiote_in_event_enable(ENC_PIN_A, true);
    nrfx_timer_disable(&m_timer2);
}

void encoder_init(enc_evt_handler_t evt_handler)
{
    ret_code_t err_code;

    if (!nrfx_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    nrf_drv_gpiote_in_config_t in_A_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_A_config.pull = NRF_GPIO_PIN_NOPULL;
    err_code = nrf_drv_gpiote_in_init(ENC_PIN_A, &in_A_config, enc_pin_A_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(ENC_PIN_A, true);

    nrf_drv_gpiote_in_config_t in_B_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_B_config.pull = NRF_GPIO_PIN_NOPULL;
    err_code = nrf_drv_gpiote_in_init(ENC_PIN_B, &in_B_config, enc_pin_B_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(ENC_PIN_B, true);

    enc_evt_handler = evt_handler;

    nrfx_timer_config_t tmr_config = NRFX_TIMER_DEFAULT_CONFIG;
    tmr_config.frequency = (nrf_timer_frequency_t)NRF_TIMER_FREQ_1MHz;
    tmr_config.mode = (nrf_timer_mode_t)NRF_TIMER_MODE_TIMER;
    tmr_config.bit_width = (nrf_timer_bit_width_t)NRF_TIMER_BIT_WIDTH_32;

    err_code = nrfx_timer_init(&m_timer2, &tmr_config, Timer_2_Interrupt_Handler);
    APP_ERROR_CHECK(err_code);

    uint32_t time_ticks = nrfx_timer_ms_to_ticks(&m_timer2, DEBOUNCE_TIME_MS);
    nrfx_timer_extended_compare(&m_timer2, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
}