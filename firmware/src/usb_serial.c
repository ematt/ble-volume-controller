



#define NRF_LOG_MODULE_NAME usb_serial
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_INFO
#include <log/nrf_log.h>
#include <log/nrf_log_ctrl.h>
#include <log/nrf_log_default_backends.h>
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

#define USBD_POWER_DETECTION 0

APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);


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
            //bsp_board_led_on(LED_CDC_ACM_OPEN);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            //bsp_board_led_off(LED_CDC_ACM_OPEN);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            //bsp_board_led_invert(LED_CDC_ACM_TX);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            // ret_code_t ret;
            NRF_LOG_INFO("Bytes waiting: %d", app_usbd_cdc_acm_bytes_stored(p_cdc_acm));
            // do
            // {
            //     /*Get amount of data transfered*/
            //     size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
            //     NRF_LOG_INFO("RX: size: %lu char: %c", size, m_rx_buffer[0]);

            //     /* Fetch data until internal buffer is empty */
            //     ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
            //                                 m_rx_buffer,
            //                                 READ_SIZE);
            // } while (ret == NRF_SUCCESS);

            // bsp_board_led_invert(LED_CDC_ACM_RX);
            break;
        }
        default:
            break;
    }
}

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            // bsp_board_led_off(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_DRV_RESUME:
            // bsp_board_led_on(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            // bsp_board_leds_off();
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

    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }

    app_usbd_serial_num_generate();

    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("USBD CDC ACM example started.");

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }
    return ESUCCESS;

}

int usb_serial_loop()
{
    ret_code_t ret;
    static int  frame_counter = 0;

    ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, "Hello USB CDC FA demo: \r\n", sizeof("Hello USB CDC FA demo: \r\n"));
    if (ret == NRF_SUCCESS)
    {
        ++frame_counter;
    }
    return ESUCCESS;
}
