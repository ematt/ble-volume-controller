#ifndef BLE_DEVICE_INFO_H_
#define BLE_DEVICE_INFO_H_

#define PNP_ID_VENDOR_ID_SOURCE 0x02                    /**< Vendor ID Source. */
#define PNP_ID_VENDOR_ID        0x1915                  /**< Vendor ID. */
#define PNP_ID_PRODUCT_ID       0xEEEE                  /**< Product ID. */
#define PNP_ID_PRODUCT_VERSION  0x0001                  /**< Product Version. */

void ble_device_info_dis_init(void);

#endif // BLE_DEVICE_INFO_H_
