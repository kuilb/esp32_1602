#ifndef WEB_SETTING_H
#define WEB_SETTING_H

#include "./hardware/lcd_driver.h"
#include "./connectivity/jwt_auth.h"
#include "./menu/menu.h"
#include "./utils/logger.h"
#include "./services/ota_manager.h"
#include "./utils/memory_utils.h"
#include "./utils/logger.h"
#include "./services/web_pages.h"

#include <esp_ota_ops.h>

void webSettingSetupWebServer();

#endif // WEB_SETTING_H
