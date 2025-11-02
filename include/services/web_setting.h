#ifndef WEB_SETTING_H
#define WEB_SETTING_H

#include "myheader.h"
#include "lcd_driver.h"
#include "jwt_auth.h"
#include "menu.h"
#include "logger.h"
#include "ota_manager.h"
#include "memory_utils.h"
#include "logger.h"

#include <esp_ota_ops.h>

void web_setting_setupWebServer();

#endif // WEB_SETTING_H
