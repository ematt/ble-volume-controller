#include <stdint.h>
#include <string.h>
#include <string.h>
#include <limits.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "app_util.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_hids.h"
#include "ble_bas.h"
#include "ble_conn_params.h"
#include "sensorsim.h"
#include "bsp.h"
#include "app_scheduler.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_drv_gpiote.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "ble_advertising.h"
#include "fds.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_pwr_mgmt.h"
#include "peer_manager_handler.h"
#include "boards.h"
#include "app_util_platform.h"

#include "nrfx_timer.h"
#include "nrf_timer.h"

#include "error_codes.h"
#include "encoder.h"
#include "about.h"
#include "ble_device_info.h"
