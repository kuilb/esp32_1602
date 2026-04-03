#ifndef SETTING_H
#define SETTING_H

/** @brief 进入手动亮度调节界面 */
void _enterBrightnessScreen();

/** @brief 切换自动亮度状态 */
void _toggleAutoBrightness();

/** @brief 切换按键音效状态 */
void _toggleSoundEffects();

/** @brief 清除 WiFi 配置并重启 */
void _resetWifi();

/** @brief 重置燃料计参数 */
void _resetFuelGauge();

/** @brief 启动 Web 设置服务 */
void _setupWebSetting();

/** @brief 显示连接信息界面 */
void _connectInfo();

/** @brief 进入电池信息界面 */
void _enterBatteryInfoScreen();

/** @brief 重启系统 */
void _rebootSystem();

#endif
