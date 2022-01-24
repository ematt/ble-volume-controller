
#include "includes.h"

#define NRF_LOG_MODULE_NAME ble_device_info
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_INFO
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Function for initializing Device Information Service.
 */
void ble_device_info_dis_init(void)
{
    ret_code_t err_code;
    ble_dis_init_t dis_init_obj;
    ble_dis_pnp_id_t pnp_id;

    pnp_id.vendor_id_source = PNP_ID_VENDOR_ID_SOURCE;
    pnp_id.vendor_id = PNP_ID_VENDOR_ID;
    pnp_id.product_id = PNP_ID_PRODUCT_ID;
    pnp_id.product_version = PNP_ID_PRODUCT_VERSION;

    memset(&dis_init_obj, 0, sizeof(dis_init_obj));

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
