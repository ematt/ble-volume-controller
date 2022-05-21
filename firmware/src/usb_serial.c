



#define NRF_LOG_MODULE_NAME usb_serial
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_INFO
#include <log/nrf_log.h>
#include <log/nrf_log_ctrl.h>
#include <log/nrf_log_default_backends.h>
#include <log/src/nrf_log_backend_serial.h>
#include <log/nrf_log_backend_interface.h>
NRF_LOG_MODULE_REGISTER();

#include <usbd/class/cdc/acm/app_usbd_cdc_acm.h>
#include <usbd/app_usbd_serial_num.h>
#include <nrf_drv_clock.h>

#include "error_codes.h"

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

static bool m_async_mode;
static volatile bool m_xfer_done;

static void serial_tx(void const * p_context, char const * p_buffer, size_t len)
{
    m_xfer_done = false;
    ret_code_t err_code = app_usbd_cdc_acm_write(&m_app_cdc_acm, p_buffer, len);
    APP_ERROR_CHECK(err_code);
    /* wait for completion since buffer is reused*/
    while (m_async_mode && (m_xfer_done == false));
}

void usb_serial_log_backend_api_put(nrf_log_backend_t const * p_backend, nrf_log_entry_t * p_entry)
{
    static uint8_t m_string_buff[128];
    nrf_log_backend_serial_put(p_backend, p_entry, m_string_buff,
                               sizeof(m_string_buff), serial_tx);
}

void usb_serial_log_backend_api_panic_set(nrf_log_backend_t const * p_backend)
{
    m_async_mode = true;
}

void usb_serial_log_backend_api_flush(nrf_log_backend_t const * p_backend)
{

}

const nrf_log_backend_api_t usb_serial_log_backend_api = {
        .put       = usb_serial_log_backend_api_put,
        .flush     = usb_serial_log_backend_api_flush,
        .panic_set = usb_serial_log_backend_api_panic_set,
};

NRF_LOG_BACKEND_DEF(usb_serial_log_backend, usb_serial_log_backend_api, NULL);

/**
 * @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t (headphones)
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            NRF_LOG_INFO("VCP port open");
            nrf_log_backend_enable(&usb_serial_log_backend);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            NRF_LOG_INFO("VCP port close");
            nrf_log_backend_disable(&usb_serial_log_backend);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            m_xfer_done = true;
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            ret_code_t ret;
            NRF_LOG_INFO("Bytes waiting: %d", app_usbd_cdc_acm_bytes_stored(p_cdc_acm));
            break;
        }
        default:
            break;
    }
}

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    NRF_LOG_INFO("usbd_user_ev_handler %d", event);
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            break;
        case APP_USBD_EVT_DRV_RESUME:
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}

int usb_serial_init()
{
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);

    nrf_drv_clock_lfclk_request(NULL);

    while(!nrf_drv_clock_lfclk_is_running());
    
    app_usbd_serial_num_generate();

    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);

    int32_t backend_id = -1;
    backend_id = nrf_log_backend_add(&usb_serial_log_backend, NRF_LOG_SEVERITY_DEBUG);
    ASSERT(backend_id >= 0);

    return ESUCCESS;
}

int usb_serial_loop()
{
    while (app_usbd_event_queue_process());

    return ESUCCESS;
}
