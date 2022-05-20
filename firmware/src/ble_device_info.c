
//#include "includes.h"

#include <ble/ble_services/ble_dis/ble_dis.h>

#include "about.h"
#include "ble_device_info.h"

#define NRF_LOG_MODULE_NAME ble_device_info
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_INFO
#include <log/nrf_log.h>
#include <log/nrf_log_ctrl.h>
#include <log/nrf_log_default_backends.h>
NRF_LOG_MODULE_REGISTER();

/**@brief Function for initializing Device Information Service.
 */
void ble_device_info_dis_init(void)
{
    ret_code_t err_code;
    ble_dis_init_t dis_init_obj = {0};
    ble_dis_pnp_id_t pnp_id = {
        .vendor_id_source = PNP_ID_VENDOR_ID_SOURCE,
        .vendor_id = PNP_ID_VENDOR_ID,
        .product_id = PNP_ID_PRODUCT_ID,
        .product_version = PNP_ID_PRODUCT_VERSION,
    };

    ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, ABOUT_MANUFACTURER_NAME);
    ble_srv_ascii_to_utf8(&dis_init_obj.model_num_str, ABOUT_MODEL_NAME);
    ble_srv_ascii_to_utf8(&dis_init_obj.serial_num_str, ABOUT_SERIAL_NO);
    ble_srv_ascii_to_utf8(&dis_init_obj.hw_rev_str, ABOUT_HARDWARE_REV_STRING);
    ble_srv_ascii_to_utf8(&dis_init_obj.fw_rev_str, ABOUT_FIRMWARE_REV_STRING);
    dis_init_obj.p_pnp_id = &pnp_id;

    dis_init_obj.dis_char_rd_sec = SEC_JUST_WORKS;

    err_code = ble_dis_init(&dis_init_obj);
    APP_ERROR_CHECK(err_code);
}
