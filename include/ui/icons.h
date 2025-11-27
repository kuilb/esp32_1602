#ifndef ICONS_H
#define ICONS_H

#include <Arduino.h>

//  系统图标
namespace SystemIcons {
    extern uint8_t wifiIcon[];
    extern uint8_t wifiOffIcon[];
    extern uint8_t bluetoothIcon[];
    extern uint8_t batteryFullIcon[];
    extern uint8_t batteryHalfIcon[];
    extern uint8_t batteryLowIcon[];
    extern uint8_t batteryChargingIcon[];
    
    extern uint8_t tempIcon[];
    extern uint8_t celsius[];

    uint8_t* getIcon(const String& iconName);
}

// 天气风向图标
namespace WindIcons {
    extern uint8_t northWindIcon[];
    extern uint8_t southWindIcon[];
    extern uint8_t eastWindIcon[];
    extern uint8_t westWindIcon[];
    extern uint8_t northEastWindIcon[];
    extern uint8_t southEastWindIcon[];
    extern uint8_t southWestWindIcon[];
    extern uint8_t northWestWindIcon[];

    uint8_t* getIcon(const String& direction);
}

// 天气图标
namespace WeatherIcons {
        extern uint8_t sunnyLeft[8];
    extern uint8_t sunnyRight[8];
    extern uint8_t cloudyLeft[8];
    extern uint8_t cloudyRight[8];
    extern uint8_t overcastLeft[8];
    extern uint8_t overcastRight[8];
    extern uint8_t lightRainLeft[8];
    extern uint8_t lightRainRight[8];
    extern uint8_t moderateRainLeft[8];
    extern uint8_t moderateRainRight[8];
    extern uint8_t heavyRainLeft[8];
    extern uint8_t heavyRainRight[8];
    extern uint8_t stormLeft[8];
    extern uint8_t stormRight[8];
    extern uint8_t fogLeft[8];
    extern uint8_t fogRight[8];
    extern uint8_t snowLeft[8];
    extern uint8_t snowRight[8];
    extern uint8_t thunderLeft[8];
    extern uint8_t thunderRight[8];
    extern uint8_t dustStormLeft[8];
    extern uint8_t dustStormRight[8];
    
    uint8_t* getLeftIcon(const String& weather);
    uint8_t* getRightIcon(const String& weather);
}

#endif  // ICONS_H