
#include "includes.h"

#define NRF_LOG_MODULE_NAME encoder
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_DEBUG
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_drv_qdec.h"
NRF_LOG_MODULE_REGISTER();

#define ENC_PIN_A BSP_QSPI_IO2_PIN
#define ENC_PIN_B BSP_QSPI_IO3_PIN

const uint32_t DEBOUNCE_TIME_MS = 4;

static enc_evt_handler_t enc_evt_handler = NULL;

static volatile bool interrupPinAState = false;


static void qdec_event_handler(nrf_drv_qdec_event_t event)
{
    if (event.type == NRF_QDEC_EVENT_REPORTRDY)
    {
        NRF_LOG_INFO("%d %d", event.data.report.accdbl, event.data.report.acc);
        if(event.data.report.acc > 0)
        {
            enc_evt_handler(ENCODER_DIR_CCW);
        }
        else
        {
            enc_evt_handler(ENCODER_DIR_CW);
        }
    }
}

void encoder_init(enc_evt_handler_t evt_handler)
{
    ret_code_t err_code;
    nrf_drv_qdec_config_t const default_config = NRFX_QDEC_DEFAULT_CONFIG;
    err_code = nrf_drv_qdec_init(&default_config, qdec_event_handler);
    APP_ERROR_CHECK(err_code);
    nrf_gpio_cfg_input(default_config.psela, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(default_config.pselb, NRF_GPIO_PIN_PULLUP);

    enc_evt_handler = evt_handler;

    nrf_drv_qdec_enable();

    NRF_LOG_INFO("encoder_init");
}