#include "includes.h"

#define NRF_LOG_MODULE_NAME main
#define NRF_LOG_LEVEL NRF_LOG_SEVERITY_INFO
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

#define DEVICE_NAME ABOUT_PROJECT_NAME                 /**< Name of device. Will be included in the advertising data. */

#define APP_BLE_OBSERVER_PRIO 3 /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG 1  /**< A tag identifying the SoftDevice BLE configuration. */

/*lint -emacro(524, MIN_CONN_INTERVAL) // Loss of precision */
#define MIN_CONN_INTERVAL MSEC_TO_UNITS(7.5, UNIT_1_25_MS) /**< Minimum connection interval (7.5 ms). */
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(15, UNIT_1_25_MS)  /**< Maximum connection interval (15 ms). */
#define SLAVE_LATENCY 20                                   /**< Slave latency. */
#define CONN_SUP_TIMEOUT MSEC_TO_UNITS(3000, UNIT_10_MS)   /**< Connection supervisory timeout (3000 ms). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAM_UPDATE_COUNT 3                        /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND 1                               /**< Perform bonding. */
#define SEC_PARAM_MITM 0                               /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC 0                               /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS 0                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES BLE_GAP_IO_CAPS_NONE /**< No I/O capabilities. */

#define SEC_PARAM_OOB 0                                /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE 7                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE 16                      /**< Maximum encryption key size. */

#define SWIFT_PAIR_SUPPORTED 0 /**< Swift Pair feature is supported. */
#if SWIFT_PAIR_SUPPORTED == 1
#define MICROSOFT_VENDOR_ID 0x0006         /**< Microsoft Vendor ID.*/
#define MICROSOFT_BEACON_ID 0x03           /**< Microsoft Beacon ID, used to indicate that Swift Pair feature is supported. */
#define MICROSOFT_BEACON_SUB_SCENARIO 0x00 /**< Microsoft Beacon Sub Scenario, used to indicate how the peripheral will pair using Swift Pair feature. */
#define RESERVED_RSSI_BYTE 0x80            /**< Reserved RSSI byte, used to maintain forwards and backwards compatibility. */
#endif

#define MOVEMENT_SPEED 5             /**< Number of pixels by which the cursor is moved each time a button is pushed. */
#define INPUT_REPORT_COUNT 3         /**< Number of input reports in this application. */
#define INPUT_REP_BUTTONS_LEN 3      /**< Length of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_LEN 3     /**< Length of Mouse Input Report containing movement data. */
#define INPUT_REP_MEDIA_PLAYER_LEN 1 /**< Length of Mouse Input Report containing media player data. */
#define INPUT_REP_BUTTONS_INDEX 0    /**< Index of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_INDEX 1   /**< Index of Mouse Input Report containing movement data. */
#define INPUT_REP_MPLAYER_INDEX 2    /**< Index of Mouse Input Report containing media player data. */
#define INPUT_REP_REF_BUTTONS_ID 1   /**< Id of reference to Mouse Input Report containing button data. */
#define INPUT_REP_REF_MOVEMENT_ID 2  /**< Id of reference to Mouse Input Report containing movement data. */
#define INPUT_REP_REF_MPLAYER_ID 3   /**< Id of reference to Mouse Input Report containing media player data. */

#define BASE_USB_HID_SPEC_VERSION 0x0101 /**< Version number of base USB HID Specification implemented by this application. */

#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#ifdef SVCALL_AS_NORMAL_FUNCTION
#define SCHED_QUEUE_SIZE 20 /**< Maximum number of events in the scheduler queue. More is needed in case of Serialization. */
#else
#define SCHED_QUEUE_SIZE 10 /**< Maximum number of events in the scheduler queue. */
#endif

#define DEAD_BEEF 0xDEADBEEF /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_ADV_FAST_INTERVAL 0x0028 /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.). */
#define APP_ADV_SLOW_INTERVAL 0x00A0 /**< Slow advertising interval (in units of 0.625 ms. This value corresponds to 100 ms.). */

#define APP_ADV_FAST_DURATION 3000  /**< The advertising duration of fast advertising in units of 10 milliseconds. */
#define APP_ADV_SLOW_DURATION 18000 /**< The advertising duration of slow advertising in units of 10 milliseconds. */

BLE_HIDS_DEF(m_hids,               /**< HID service instance. */
             NRF_SDH_BLE_TOTAL_LINK_COUNT,
             INPUT_REP_BUTTONS_LEN,
             INPUT_REP_MOVEMENT_LEN,
             INPUT_REP_MEDIA_PLAYER_LEN);
NRF_BLE_GATT_DEF(m_gatt);           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising); /**< Advertising module instance. */

static bool m_in_boot_mode = false;                      /**< Current protocol mode. */
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */
static pm_peer_id_t m_peer_id;                           /**< Device reference handle to the current bonded central. */
static ble_uuid_t m_adv_uuids[] =                        /**< Universally unique service identifiers. */
    {
        {BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE, BLE_UUID_TYPE_BLE}};

#if SWIFT_PAIR_SUPPORTED == 1
static uint8_t m_sp_payload[] = /**< Payload of advertising data structure for Microsoft Swift Pair feature. */
    {
        MICROSOFT_BEACON_ID,
        MICROSOFT_BEACON_SUB_SCENARIO,
        RESERVED_RSSI_BYTE};
static ble_advdata_manuf_data_t m_sp_manuf_advdata = /**< Advertising data structure for Microsoft Swift Pair feature. */
    {
        .company_identifier = MICROSOFT_VENDOR_ID,
        .data =
            {
                .size = sizeof(m_sp_payload),
                .p_data = &m_sp_payload[0]}};
static ble_advdata_t m_sp_advdata;
#endif


typedef struct __attribute__((__packed__))
{
    //uint8_t  reportId;                                 // Report ID = 0x01 (1)
    // Collection: CA:Mouse CP:Pointer
    uint8_t BTN_MousePointerButton1 : 1; // Usage 0x00090001: Button 1 Primary/trigger, Value = 0 to 1
    uint8_t BTN_MousePointerButton2 : 1; // Usage 0x00090002: Button 2 Secondary, Value = 0 to 1
    uint8_t BTN_MousePointerButton3 : 1; // Usage 0x00090003: Button 3 Tertiary, Value = 0 to 1
    uint8_t BTN_MousePointerButton4 : 1; // Usage 0x00090004: Button 4, Value = 0 to 1
    uint8_t BTN_MousePointerButton5 : 1; // Usage 0x00090005: Button 5, Value = 0 to 1
    uint8_t : 3;                         // Pad
    int8_t GD_MousePointerWheel;         // Usage 0x00010038: Wheel, Value = -127 to 127
    int8_t CD_MousePointerAcPan;         // Usage 0x000C0238: AC Pan, Value = -127 to 127
} inputReport01_t;

typedef struct __attribute__((__packed__))
{
    //uint8_t  reportId;                                 // Report ID = 0x02 (2)
    // Collection: CA:Mouse CP:ConsumerControl
    int16_t GD_MouseConsumerControlX : 12; // Usage 0x00010030: X, Value = -2047 to 2047
    int16_t GD_MouseConsumerControlY : 12; // Usage 0x00010031: Y, Value = -2047 to 2047
} inputReport02_t;

typedef struct __attribute__((__packed__))
{
    //uint8_t  reportId;                                 // Report ID = 0x03 (3)
    // Collection: CA:ConsumerControl
    uint8_t CD_ConsumerControlPlayPause : 1;                      // Usage 0x000C00CD: Play/Pause, Value = 0 to 1
    uint8_t CD_ConsumerControlAlConsumerControlConfiguration : 1; // Usage 0x000C0183: AL Consumer Control Configuration, Value = 0 to 1
    uint8_t CD_ConsumerControlScanNextTrack : 1;                  // Usage 0x000C00B5: Scan Next Track, Value = 0 to 1
    uint8_t CD_ConsumerControlScanPreviousTrack : 1;              // Usage 0x000C00B6: Scan Previous Track, Value = 0 to 1
    uint8_t CD_ConsumerControlVolumeDecrement : 1;                // Usage 0x000C00EA: Volume Decrement, Value = 0 to 1
    uint8_t CD_ConsumerControlVolumeIncrement : 1;                // Usage 0x000C00E9: Volume Increment, Value = 0 to 1
    uint8_t CD_ConsumerControlAcForward : 1;                      // Usage 0x000C0225: AC Forward, Value = 0 to 1
    uint8_t CD_ConsumerControlAcBack : 1;                         // Usage 0x000C0224: AC Back, Value = 0 to 1
} inputReport03_t;

enum CONSUMER_BUTTON
{
    CONSUMER_BUTTON_ReleaseAll = 0,
    CONSUMER_BUTTON_PlayPause = (1 << 0),
    CONSUMER_BUTTON_AlConsumerControlConfiguration = (1 << 1),
    CONSUMER_BUTTON_ScanNextTrack = (1 << 2),
    CONSUMER_BUTTON_ScanPreviousTrack = (1 << 3),
    CONSUMER_BUTTON_VolumeDecrement = (1 << 4),
    CONSUMER_BUTTON_VolumeIncrement = (1 << 5),
    CONSUMER_BUTTON_AcForward = (1 << 6),
    CONSUMER_BUTTON_AcBack = (1 << 7),
};

enum APP_BSP_EVENT
{
    APP_BSP_EVENT_BTN_1_PRESSED = BSP_EVENT_KEY_LAST,
    APP_BSP_EVENT_BTN_1_RELEASED,
    APP_BSP_EVENT_BTN_1_LONG_PRESS,
    APP_BSP_EVENT_BTN_ENC_PRESSED,
    APP_BSP_EVENT_BTN_ENC_RELEASED,
    APP_BSP_EVENT_BTN_ENC_LONG_PRESS,
};

static bool panelToggle = true;

static void on_hids_evt(ble_hids_t *p_hids, ble_hids_evt_t *p_evt);

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    NRF_LOG_DEBUG("assert_nrf_callback %d %s", line_num, p_file_name);
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for setting filtered whitelist.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void whitelist_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    uint32_t peer_id_count = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
                 peer_id_count + 1,
                 BLE_GAP_WHITELIST_ADDR_MAX_COUNT);

    err_code = pm_whitelist_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for setting filtered device identities.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void identities_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT];
    uint32_t peer_id_count = BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    err_code = pm_device_identities_list_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}

/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        whitelist_set(PM_PEER_ID_LIST_SKIP_NO_ID_ADDR);

        ret_code_t ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(ret);
    }
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const *p_evt)
{
    ret_code_t err_code;

    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
    case PM_EVT_BONDED_PEER_CONNECTED:
    {
        NRF_LOG_INFO("Connected to a previously bonded device.");
    } 
    break;
    case PM_EVT_PEERS_DELETE_SUCCEEDED:
        advertising_start(false);
        break;

    case PM_EVT_CONN_SEC_CONFIG_REQ:
    {
        // Allow pairing request from an already bonded peer.
        pm_conn_sec_config_t conn_sec_config = { .allow_repairing = true };
        pm_conn_sec_config_reply(m_conn_handle, &conn_sec_config);
    } 
    break;

    case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        if (p_evt->params.peer_data_update_succeeded.flash_changed && (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_BONDING))
        {
            NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible");
            // Note: You should check on what kind of white list policy your application should use.

            whitelist_set(PM_PEER_ID_LIST_SKIP_NO_ID_ADDR);
        }
        break;

    case PM_EVT_CONN_SEC_SUCCEEDED:
    {
        /* Restore default Peer Manager configuration */
        ble_gap_sec_params_t sec_params;
        memset(&sec_params, 0, sizeof(sec_params));

        // Security parameters to be used for all security procedures.
        sec_params.bond = SEC_PARAM_BOND;
        sec_params.mitm = SEC_PARAM_MITM;
        sec_params.lesc = SEC_PARAM_LESC;
        sec_params.keypress = SEC_PARAM_KEYPRESS;
        sec_params.io_caps = SEC_PARAM_IO_CAPABILITIES;
        sec_params.oob = SEC_PARAM_OOB;
        sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
        sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
        sec_params.kdist_own.enc = 1;
        sec_params.kdist_own.id = 1;
        sec_params.kdist_peer.enc = 1;
        sec_params.kdist_peer.id = 1;

        err_code = pm_sec_params_set(&sec_params);
        APP_ERROR_CHECK(err_code);
    }
    break;

    case PM_EVT_CONN_SEC_FAILED:
        {
            if (p_evt->params.conn_sec_failed.error == PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING)
            {
                NRF_LOG_INFO("PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING");
                // Rebond if one party has lost its keys.
                err_code = pm_conn_secure(p_evt->conn_handle, true);

                if (err_code != NRF_ERROR_BUSY)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
        }
        break;

        /** @snippet [NFC Pairing Lib usage_1] */
        case PM_EVT_CONN_SEC_PARAMS_REQ:
        {
            // Send event to the NFC BLE pairing library as it may dynamically alternate
            // security parameters to achieve the highest possible security level.
        }
        break;

    default:
        break;
    }
}

/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    NRF_LOG_DEBUG("service_error_handler %d", nrf_error);
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
    NRF_LOG_DEBUG("ble_advertising_error_handler %d", nrf_error);
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code;

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HID_MOUSE);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    NRF_LOG_DEBUG("nrf_qwr_error_handler %d", nrf_error);
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Queued Write Module.
 */
static void qwr_init(void)
{
    ret_code_t err_code;
    nrf_ble_qwr_init_t qwr_init_obj = {0};

    qwr_init_obj.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing HID Service.
 */
static void hids_init(void)
{
    ret_code_t err_code;
    ble_hids_init_t hids_init_obj;
    ble_hids_inp_rep_init_t *p_input_report;
    uint8_t hid_info_flags;

    static ble_hids_inp_rep_init_t inp_rep_array[INPUT_REPORT_COUNT];
    static uint8_t rep_map_data[] =
        {
            0x05, 0x01, // Usage Page (Generic Desktop)
            0x09, 0x02, // Usage (Mouse)

            0xA1, 0x01, // Collection (Application)

            // Report ID 1: Mouse buttons + scroll/pan
            0x85, 0x01,       // Report Id 1
            0x09, 0x01,       // Usage (Pointer)
            0xA1, 0x00,       // Collection (Physical)
            0x95, 0x05,       // Report Count (3)
            0x75, 0x01,       // Report Size (1)
            0x05, 0x09,       // Usage Page (Buttons)
            0x19, 0x01,       // Usage Minimum (01)
            0x29, 0x05,       // Usage Maximum (05)
            0x15, 0x00,       // Logical Minimum (0)
            0x25, 0x01,       // Logical Maximum (1)
            0x81, 0x02,       // Input (Data, Variable, Absolute)
            0x95, 0x01,       // Report Count (1)
            0x75, 0x03,       // Report Size (3)
            0x81, 0x01,       // Input (Constant) for padding
            0x75, 0x08,       // Report Size (8)
            0x95, 0x01,       // Report Count (1)
            0x05, 0x01,       // Usage Page (Generic Desktop)
            0x09, 0x38,       // Usage (Wheel)
            0x15, 0x81,       // Logical Minimum (-127)
            0x25, 0x7F,       // Logical Maximum (127)
            0x81, 0x06,       // Input (Data, Variable, Relative)
            0x05, 0x0C,       // Usage Page (Consumer)
            0x0A, 0x38, 0x02, // Usage (AC Pan)
            0x95, 0x01,       // Report Count (1)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0xC0,             // End Collection (Physical)

            // Report ID 2: Mouse motion
            0x85, 0x02,       // Report Id 2
            0x09, 0x01,       // Usage (Pointer)
            0xA1, 0x00,       // Collection (Physical)
            0x75, 0x0C,       // Report Size (12)
            0x95, 0x02,       // Report Count (2)
            0x05, 0x01,       // Usage Page (Generic Desktop)
            0x09, 0x30,       // Usage (X)
            0x09, 0x31,       // Usage (Y)
            0x16, 0x01, 0xF8, // Logical maximum (2047)
            0x26, 0xFF, 0x07, // Logical minimum (-2047)
            0x81, 0x06,       // Input (Data, Variable, Relative)
            0xC0,             // End Collection (Physical)
            0xC0,             // End Collection (Application)

            // Report ID 3: Advanced buttons
            0x05, 0x0C, // Usage Page (Consumer)
            0x09, 0x01, // Usage (Consumer Control)
            0xA1, 0x01, // Collection (Application)
            0x85, 0x03, // Report Id (3)
            0x15, 0x00, // Logical minimum (0)
            0x25, 0x01, // Logical maximum (1)
            0x75, 0x01, // Report Size (1)
            0x95, 0x01, // Report Count (1)

            0x09, 0xCD,       // Usage (Play/Pause)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0x0A, 0x83, 0x01, // Usage (AL Consumer Control Configuration)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0x09, 0xB5,       // Usage (Scan Next Track)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0x09, 0xB6,       // Usage (Scan Previous Track)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)

            0x09, 0xEA,       // Usage (Volume Down)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0x09, 0xE9,       // Usage (Volume Up)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0x0A, 0x25, 0x02, // Usage (AC Forward)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0x0A, 0x24, 0x02, // Usage (AC Back)
            0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
            0xC0              // End Collection
        };

    memset(inp_rep_array, 0, sizeof(inp_rep_array));
    // Initialize HID Service.
    p_input_report = &inp_rep_array[INPUT_REP_BUTTONS_INDEX];
    p_input_report->max_len = INPUT_REP_BUTTONS_LEN;
    p_input_report->rep_ref.report_id = INPUT_REP_REF_BUTTONS_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
    p_input_report->sec.wr = SEC_JUST_WORKS;
    p_input_report->sec.rd = SEC_JUST_WORKS;

    p_input_report = &inp_rep_array[INPUT_REP_MOVEMENT_INDEX];
    p_input_report->max_len = INPUT_REP_MOVEMENT_LEN;
    p_input_report->rep_ref.report_id = INPUT_REP_REF_MOVEMENT_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
    p_input_report->sec.wr = SEC_JUST_WORKS;
    p_input_report->sec.rd = SEC_JUST_WORKS;

    p_input_report = &inp_rep_array[INPUT_REP_MPLAYER_INDEX];
    p_input_report->max_len = INPUT_REP_MEDIA_PLAYER_LEN;
    p_input_report->rep_ref.report_id = INPUT_REP_REF_MPLAYER_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
    p_input_report->sec.wr = SEC_JUST_WORKS;
    p_input_report->sec.rd = SEC_JUST_WORKS;

    hid_info_flags = HID_INFO_FLAG_REMOTE_WAKE_MSK | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;

    memset(&hids_init_obj, 0, sizeof(hids_init_obj));

    hids_init_obj.evt_handler = on_hids_evt;
    hids_init_obj.error_handler = service_error_handler;
    hids_init_obj.is_kb = false;
    hids_init_obj.is_mouse = true;
    hids_init_obj.inp_rep_count = INPUT_REPORT_COUNT;
    hids_init_obj.p_inp_rep_array = inp_rep_array;
    hids_init_obj.outp_rep_count = 0;
    hids_init_obj.p_outp_rep_array = NULL;
    hids_init_obj.feature_rep_count = 0;
    hids_init_obj.p_feature_rep_array = NULL;
    hids_init_obj.rep_map.data_len = sizeof(rep_map_data);
    hids_init_obj.rep_map.p_data = rep_map_data;
    hids_init_obj.hid_information.bcd_hid = BASE_USB_HID_SPEC_VERSION;
    hids_init_obj.hid_information.b_country_code = 0;
    hids_init_obj.hid_information.flags = hid_info_flags;
    hids_init_obj.included_services_count = 0;
    hids_init_obj.p_included_services_array = NULL;

    hids_init_obj.rep_map.rd_sec = SEC_JUST_WORKS;
    hids_init_obj.hid_information.rd_sec = SEC_JUST_WORKS;

    hids_init_obj.boot_mouse_inp_rep_sec.cccd_wr = SEC_JUST_WORKS;
    hids_init_obj.boot_mouse_inp_rep_sec.wr = SEC_JUST_WORKS;
    hids_init_obj.boot_mouse_inp_rep_sec.rd = SEC_JUST_WORKS;

    hids_init_obj.protocol_mode_rd_sec = SEC_JUST_WORKS;
    hids_init_obj.protocol_mode_wr_sec = SEC_JUST_WORKS;
    hids_init_obj.ctrl_point_wr_sec = SEC_JUST_WORKS;

    err_code = ble_hids_init(&m_hids, &hids_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    qwr_init();
    ble_device_info_dis_init();
    hids_init();
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    NRF_LOG_DEBUG("conn_params_error_handler %d", nrf_error);
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count = MAX_CONN_PARAM_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail = false;
    cp_init.evt_handler = NULL;
    cp_init.error_handler = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting timers.
 */
static void timers_start(void)
{
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling HID events.
 *
 * @details This function will be called for all HID events which are passed to the application.
 *
 * @param[in]   p_hids  HID service structure.
 * @param[in]   p_evt   Event received from the HID service.
 */
static void on_hids_evt(ble_hids_t *p_hids, ble_hids_evt_t *p_evt)
{
    switch (p_evt->evt_type)
    {
    case BLE_HIDS_EVT_BOOT_MODE_ENTERED:
        m_in_boot_mode = true;
        break;

    case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
        m_in_boot_mode = false;
        break;

    case BLE_HIDS_EVT_NOTIF_ENABLED:
        break;

    default:
        // No implementation needed.
        break;
    }
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
    case BLE_ADV_EVT_DIRECTED_HIGH_DUTY:
        NRF_LOG_INFO("Directed advertising.");
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_DIRECTED);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_FAST:
        NRF_LOG_INFO("Fast advertising.");
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_SLOW:
        NRF_LOG_INFO("Slow advertising.");
#if SWIFT_PAIR_SUPPORTED == 1
        m_sp_advdata.p_manuf_specific_data = NULL;
        err_code = ble_advertising_advdata_update(&m_advertising, &m_sp_advdata, NULL);
        APP_ERROR_CHECK(err_code);
#endif
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_SLOW);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_FAST_WHITELIST:
        NRF_LOG_INFO("Fast advertising with whitelist.");
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_SLOW_WHITELIST:
        NRF_LOG_INFO("Slow advertising with whitelist.");
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
        APP_ERROR_CHECK(err_code);
        err_code = ble_advertising_restart_without_whitelist(&m_advertising);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_IDLE:
        err_code = bsp_indication_set(BSP_INDICATE_IDLE);
        APP_ERROR_CHECK(err_code);
        sleep_mode_enter();
        break;

    case BLE_ADV_EVT_WHITELIST_REQUEST:
    {
        ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
        ble_gap_irk_t whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
        uint32_t addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
        uint32_t irk_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

        err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                    whitelist_irks, &irk_cnt);
        APP_ERROR_CHECK(err_code);
        NRF_LOG_DEBUG("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist",
                      addr_cnt,
                      irk_cnt);

        // Set the correct identities list (no excluding peers with no Central Address Resolution).
        identities_set(PM_PEER_ID_LIST_SKIP_NO_IRK);

        // Apply the whitelist.
        err_code = ble_advertising_whitelist_reply(&m_advertising,
                                                   whitelist_addrs,
                                                   addr_cnt,
                                                   whitelist_irks,
                                                   irk_cnt);
        APP_ERROR_CHECK(err_code);
    }
    break;

    case BLE_ADV_EVT_PEER_ADDR_REQUEST:
    {
        pm_peer_data_bonding_t peer_bonding_data;

        // Only Give peer address if we have a handle to the bonded peer.
        if (m_peer_id != PM_PEER_ID_INVALID)
        {

            err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
            if (err_code != NRF_ERROR_NOT_FOUND)
            {
                APP_ERROR_CHECK(err_code);

                // Manipulate identities to exclude peers with no Central Address Resolution.
                identities_set(PM_PEER_ID_LIST_SKIP_ALL);

                ble_gap_addr_t *p_peer_addr = &(peer_bonding_data.peer_ble_id.id_addr_info);
                err_code = ble_advertising_peer_addr_reply(&m_advertising, p_peer_addr);
                APP_ERROR_CHECK(err_code);
            }
        }
        break;
    }

    default:
        break;
    }
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
	char       passkey[BLE_GAP_PASSKEY_LEN + 1];
    ret_code_t err_code;
    static pm_evt_id_t lastId = 0;
    if(lastId != p_ble_evt->header.evt_id)
    {
        NRF_LOG_INFO("%d", p_ble_evt->header.evt_id);
        lastId = p_ble_evt->header.evt_id;
    }

    
	pm_handler_secure_on_connection(p_ble_evt);
    
    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_CONNECTED:
        NRF_LOG_INFO("Connected");
        err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
        APP_ERROR_CHECK(err_code);

        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

        err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("Disconnected");
        // LED indication will be changed when advertising starts.

        m_conn_handle = BLE_CONN_HANDLE_INVALID;
        break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
        NRF_LOG_DEBUG("PHY update request.");
        ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
        err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
        APP_ERROR_CHECK(err_code);
    }
    break;

    case BLE_GATTC_EVT_TIMEOUT:
        // Disconnect on GATT Client timeout event.
        NRF_LOG_DEBUG("GATT Client Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_EVT_USER_MEM_REQUEST:
        err_code = sd_ble_user_mem_reply(p_ble_evt->evt.gap_evt.conn_handle, NULL);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        // No system attributes have been stored.
        err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_TIMEOUT:
        // Disconnect on GATT Server timeout event.
        NRF_LOG_DEBUG("GATT Server Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GAP_EVT_AUTH_STATUS:
		{
			NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x",
				p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
				p_ble_evt->evt.gap_evt.params.auth_status.bonded,
				p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
				*((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
				*((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
			    
		}
			break;
    
    case BLE_GAP_EVT_PASSKEY_DISPLAY:
        memcpy(passkey, p_ble_evt->evt.gap_evt.params.passkey_display.passkey, BLE_GAP_PASSKEY_LEN);
        passkey[BLE_GAP_PASSKEY_LEN] = 0x00;
        NRF_LOG_INFO("BLE_GAP_EVT_PASSKEY_DISPLAY: passkey=%s match_req=%d",
            nrf_log_push(passkey),
            p_ble_evt->evt.gap_evt.params.passkey_display.match_request);

        if (p_ble_evt->evt.gap_evt.params.passkey_display.match_request)
        {				
            err_code = sd_ble_gap_auth_key_reply(m_conn_handle,
                BLE_GAP_AUTH_KEY_TYPE_PASSKEY,
                NULL);
            APP_ERROR_CHECK(err_code);
        }
        break;

    case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
    {
        ble_gatts_evt_rw_authorize_request_t  req;
        ble_gatts_rw_authorize_reply_params_t auth_reply;

        req = p_ble_evt->evt.gatts_evt.params.authorize_request;

        if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
        {
            if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ) ||
                (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
            {
                if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                {
                    auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                }
                else
                {
                    auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                }

                auth_reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2;
                err_code                            = sd_ble_gatts_rw_authorize_reply(
                    p_ble_evt->evt.gatts_evt.conn_handle,
                    &auth_reply);
                APP_ERROR_CHECK(err_code);
            }
        }
    }
    break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

    default:
        // No implementation needed.
        break;
    }
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond = SEC_PARAM_BOND;
    sec_param.mitm = SEC_PARAM_MITM;
    sec_param.lesc = SEC_PARAM_LESC;
    sec_param.keypress = SEC_PARAM_KEYPRESS;
    sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob = SEC_PARAM_OOB;
    sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc = 1;
    sec_param.kdist_own.id = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t err_code;
    uint8_t adv_flags;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    //adv_flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    adv_flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;
    init.advdata.flags = adv_flags;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids = m_adv_uuids;
#if SWIFT_PAIR_SUPPORTED == 1
    init.advdata.p_manuf_specific_data = &m_sp_manuf_advdata;
    memcpy(&m_sp_advdata, &init.advdata, sizeof(m_sp_advdata));
#endif

    init.config.ble_adv_whitelist_enabled = true;
    init.config.ble_adv_directed_high_duty_enabled = true;
    init.config.ble_adv_directed_enabled = false;
    init.config.ble_adv_directed_interval = 0;
    init.config.ble_adv_directed_timeout = 0;
    init.config.ble_adv_fast_enabled = true;
    init.config.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    init.config.ble_adv_fast_timeout = APP_ADV_FAST_DURATION;
//    init.config.ble_adv_fast_timeout = 0;
    init.config.ble_adv_slow_enabled = true;
    init.config.ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
    init.config.ble_adv_slow_timeout = APP_ADV_SLOW_DURATION;

    init.evt_handler = on_adv_evt;
    init.error_handler = ble_advertising_error_handler;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */
static void mouse_movement_send(int16_t x_delta, int16_t y_delta)
{
    ret_code_t err_code;

    if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        NRF_LOG_INFO("mouse_movement_send with no connection");
        return;
    }

    if (m_in_boot_mode)
    {
        x_delta = MIN(x_delta, 0x00ff);
        y_delta = MIN(y_delta, 0x00ff);

        err_code = ble_hids_boot_mouse_inp_rep_send(&m_hids,
                                                    0x00,
                                                    (int8_t)x_delta,
                                                    (int8_t)y_delta,
                                                    0,
                                                    NULL,
                                                    m_conn_handle);
    }
    else
    {
        APP_ERROR_CHECK_BOOL(sizeof(inputReport02_t) == INPUT_REP_MOVEMENT_LEN);

        inputReport02_t data = {0};
        data.GD_MouseConsumerControlX = MIN(x_delta, 0x0fff);
        data.GD_MouseConsumerControlY = MIN(y_delta, 0x0fff);

        err_code = ble_hids_inp_rep_send(&m_hids,
                                         INPUT_REP_MOVEMENT_INDEX,
                                         INPUT_REP_MOVEMENT_LEN,
                                         (uint8_t *)&data,
                                         m_conn_handle);
    }

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */
static void mouse_ConsumerControl_send(uint8_t button)
{
    ret_code_t err_code;

    if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        NRF_LOG_INFO("mouse_ConsumerControl_send with no connection");
        return;
    }

    APP_ERROR_CHECK_BOOL(sizeof(inputReport03_t) == INPUT_REP_MEDIA_PLAYER_LEN);

    inputReport03_t data = {0};

    if (button & CONSUMER_BUTTON_PlayPause)
        data.CD_ConsumerControlPlayPause = 1;
    if (button & CONSUMER_BUTTON_AlConsumerControlConfiguration)
        data.CD_ConsumerControlAlConsumerControlConfiguration = 1;
    if (button & CONSUMER_BUTTON_ScanNextTrack)
        data.CD_ConsumerControlScanNextTrack = 1;
    if (button & CONSUMER_BUTTON_ScanPreviousTrack)
        data.CD_ConsumerControlScanPreviousTrack = 1;
    if (button & CONSUMER_BUTTON_VolumeDecrement)
        data.CD_ConsumerControlVolumeDecrement = 1;
    if (button & CONSUMER_BUTTON_VolumeIncrement)
        data.CD_ConsumerControlVolumeIncrement = 1;
    if (button & CONSUMER_BUTTON_AcForward)
        data.CD_ConsumerControlAcForward = 1;
    if (button & CONSUMER_BUTTON_AcBack)
        data.CD_ConsumerControlAcBack = 1;

    err_code = ble_hids_inp_rep_send(&m_hids,
                                     INPUT_REP_MPLAYER_INDEX,
                                     INPUT_REP_MEDIA_PLAYER_LEN,
                                     (uint8_t *)&data,
                                     m_conn_handle);

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */
static void mouse_wheel_send(int16_t delta)
{
    ret_code_t err_code;

    if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        NRF_LOG_INFO("mouse_wheel_send with no connection");
        return;
    }

    APP_ERROR_CHECK_BOOL(INPUT_REP_BUTTONS_LEN == sizeof(inputReport01_t));

    inputReport01_t data = {0};
    data.GD_MousePointerWheel = MIN(delta, CHAR_MAX);

    err_code = ble_hids_inp_rep_send(&m_hids,
                                     INPUT_REP_BUTTONS_INDEX,
                                     INPUT_REP_BUTTONS_LEN,
                                     (uint8_t *)&data,
                                     m_conn_handle);

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
static void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;
    static bool wasLongPress = false;
    

    NRF_LOG_INFO("bsp_event_handler %d", event);

    switch (event)
    {
    case BSP_EVENT_SLEEP:
        sleep_mode_enter();
        break;

    case BSP_EVENT_DISCONNECT:
        err_code = sd_ble_gap_disconnect(m_conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
        break;

    case BSP_EVENT_WHITELIST_OFF:
        if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
        {
            err_code = ble_advertising_restart_without_whitelist(&m_advertising);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
        }
        break;
    default:
        switch((enum APP_BSP_EVENT)event)
        {
        case APP_BSP_EVENT_BTN_1_PRESSED:
            bsp_board_led_on(BSP_BOARD_LED_0);
            break;
        case APP_BSP_EVENT_BTN_1_RELEASED:
            bsp_board_led_off(BSP_BOARD_LED_0);
            break;
        case APP_BSP_EVENT_BTN_1_LONG_PRESS:

            break;
        case APP_BSP_EVENT_BTN_ENC_PRESSED:
            wasLongPress = false;
            break;
        case APP_BSP_EVENT_BTN_ENC_RELEASED:
            if(!wasLongPress)
            {
                //mouse_ConsumerControl_send(CONSUMER_BUTTON_PlayPause);
            }
            if(wasLongPress)
            {
                panelToggle = !panelToggle;
                if(panelToggle)
                {
                    bsp_board_led_on(BSP_BOARD_LED_0);
                }
                else
                {
                    bsp_board_led_off(BSP_BOARD_LED_0);
                }
            }
            else
            {
                mouse_ConsumerControl_send(CONSUMER_BUTTON_PlayPause);
                mouse_ConsumerControl_send(CONSUMER_BUTTON_ReleaseAll);
            }
            //mouse_ConsumerControl_send(CONSUMER_BUTTON_ReleaseAll);
            break;
        case APP_BSP_EVENT_BTN_ENC_LONG_PRESS:
            wasLongPress = true;
            //mouse_ConsumerControl_send(CONSUMER_BUTTON_VolumeIncrement);
            break;

        default:
            break;
        }
    }
}

/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool *p_erase_bonds)
{
    ret_code_t err_code;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Assign button events");
    const struct 
    {
        uint32_t button;
        bsp_button_action_t action;
        enum APP_BSP_EVENT event;
    } map[] = 
    {
        {BSP_BUTTON_BOARD, BSP_BUTTON_ACTION_PUSH,      APP_BSP_EVENT_BTN_1_PRESSED},
        {BSP_BUTTON_BOARD, BSP_BUTTON_ACTION_RELEASE,   APP_BSP_EVENT_BTN_1_RELEASED},
        {BSP_BUTTON_BOARD, BSP_BUTTON_ACTION_LONG_PUSH, APP_BSP_EVENT_BTN_1_LONG_PRESS},
        {BSP_BUTTON_ENC,   BSP_BUTTON_ACTION_PUSH,      APP_BSP_EVENT_BTN_ENC_PRESSED},
        {BSP_BUTTON_ENC,   BSP_BUTTON_ACTION_RELEASE,   APP_BSP_EVENT_BTN_ENC_RELEASED},
        {BSP_BUTTON_ENC,   BSP_BUTTON_ACTION_LONG_PUSH, APP_BSP_EVENT_BTN_ENC_LONG_PRESS},
    };

    for(int i = 0; i < ARRAY_SIZE(map); i++)
    {
        err_code = bsp_event_to_button_action_assign(map[i].button, map[i].action, map[i].event);
        APP_ERROR_CHECK(err_code);
        NRF_LOG_INFO("bsp_event_to_button_action_assign %d", i);
    }

    *p_erase_bonds = false;
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
    
    NRF_LOG_INFO("\033[2J\033[;H");
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    app_sched_execute();
    if (NRF_LOG_PROCESS() == false)
    {
        //nrf_pwr_mgmt_run();
    }
}

void encoder_handler(encoder_event_t evt)
{
    NRF_LOG_INFO("encoder_handler");
    switch (evt.direction)
    {
    case ENCODER_DIR_CW:
    {
        if(panelToggle)
        {
            mouse_ConsumerControl_send(CONSUMER_BUTTON_VolumeIncrement);
            mouse_ConsumerControl_send(CONSUMER_BUTTON_ReleaseAll);
        }
        else
        {
            mouse_wheel_send(evt.steps);
        }
    }
    break;

    case ENCODER_DIR_CCW:
    {
        if(panelToggle)
        {
            mouse_ConsumerControl_send(CONSUMER_BUTTON_VolumeDecrement);
            mouse_ConsumerControl_send(CONSUMER_BUTTON_ReleaseAll);
        }
        else
        {
            mouse_wheel_send(-1 * evt.steps);
        }
    }
    break;

    default:
        break;
    }
}

/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds;

    // Initialize.
    log_init();
    NRF_LOG_INFO(ABOUT_PROJECT_NAME " started.");

    timers_init();
    buttons_leds_init(&erase_bonds);
    power_management_init();
    ble_stack_init();
    scheduler_init();
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    conn_params_init();
    peer_manager_init();
    encoder_init(encoder_handler);

    // Start execution.
    timers_start();
    advertising_start(erase_bonds);

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}

/**
 * @}
 */
