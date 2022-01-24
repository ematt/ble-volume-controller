#ifndef ABOUT_H_
#define ABOUT_H_

#define ABOUT_PROJECT_NAME "Test"

#define ABOUT_FIRMWARE_REV_MAJOR    0           ///< Major firmware version.
#define ABOUT_FIRMWARE_REV_MINOR    0           ///< Minor firmware version.
#define ABOUT_FIRMWARE_REV_PATCH    1           ///< Patch firmware version.
#define ABOUT_FIRMWARE_REV_META		"alpha"     ///< Meta firmware version.
#define ABOUT_FIRMWARE_REV_STRING   (STRINGIFY(ABOUT_FIRMWARE_REV_MAJOR) "." STRINGIFY(ABOUT_FIRMWARE_REV_MINOR) "." STRINGIFY(ABOUT_FIRMWARE_REV_PATCH) ABOUT_FIRMWARE_REV_META) ///< Full version string.

#define ABOUT_HARDWARE_REV_MAJOR    0           ///< Major firmware version.
#define ABOUT_HARDWARE_REV_MINOR    0           ///< Minor firmware version.
#define ABOUT_HARDWARE_REV_PATCH    1           ///< Patch firmware version.
#define ABOUT_HARDWARE_REV_META		"proto"     ///< Meta firmware version.
#define ABOUT_HARDWARE_REV_STRING   (STRINGIFY(ABOUT_HARDWARE_REV_MAJOR) "." STRINGIFY(ABOUT_HARDWARE_REV_MINOR) "." STRINGIFY(ABOUT_HARDWARE_REV_PATCH) ABOUT_HARDWARE_REV_META) ///< Full version string.

#define ABOUT_SERIAL_NO "DEMO"
#define ABOUT_MODEL_NAME (ABOUT_PROJECT_NAME "-Model")
#define ABOUT_MANUFACTURER_NAME "Vlad"

#endif // ABOUT_H