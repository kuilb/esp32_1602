#include "./menu/menu.h"
#include "./menu/menu_navigator.h"
#include "./menu/status_bar_renderer.h"
#include "./services/auto_brightness.h"
#include "./hardware/buzzer.h"

// 前置声明
void _displayMenu(const Menu* menu, int menuIndex, int scrollOffset);
bool _checkStateChanges();
const Menu* _getMenuByState(MenuState state);

void _menuTask(void* parameter);

// =====================
// 内部辅助函数前置声明
// 目的：把“渲染/输入/滚动”拆开，降低嵌套与分支复杂度。
// =====================

static inline bool _isMainMenu(const Menu* menu);
static int _readMenuButtonOnce();
static void _renderMenuItemLine(const Menu* menu, int menuItemIndex, int visibleIndex, int lcdLine);
static bool _shouldHideMenuItem(const Menu* menu, int menuItemIndex);
static bool _isMainMenuCallback(const Menu* menu);
static bool _shouldHideMenuItemCallback(const Menu* menu, int menuItemIndex);
static const Menu* _getMenuByStateCallback(MenuState state);
static void _markDisplayNeedsUpdate();
static void _onMenuPostActivate();
static void _handleMenuInterface();
static void _handleMenuState();
static void _dispatchCurrentInterfaceState();
static MenuState _getMenuStateFromPtr(const Menu* menu);
static MenuState _getParentMenuState(MenuState childState);

enum SettingsMenuIndex {
    SETTINGS_ITEM_WEB = 0,
    SETTINGS_ITEM_WIFI_CONFIG,
    SETTINGS_ITEM_AUTO_BRIGHTNESS,
    SETTINGS_ITEM_SOUND_EFFECTS,
    SETTINGS_ITEM_BRIGHTNESS,
    SETTINGS_ITEM_BATTERY_INFO,
    SETTINGS_ITEM_RESET_FUEL_IC,
    SETTINGS_ITEM_REBOOT,
    SETTINGS_ITEM_RETURN
};

// 菜单状态变量
volatile bool inMenuMode = true;
volatile bool isReadyToDisplay = false;

static MenuContext s_menuContext = {
    &allMenus[MENU_MAIN],
    0,
    -1,
    true
};

// 当前子界面处理函数：Null 表示处于菜单模式
static InterfaceHandler s_currentHandler = nullptr;

// 菜单历史栈：进入子菜单时压栈，返回时弹栈恢复光标位置
struct MenuHistoryEntry {
    const Menu* menu;
    int menuCursor;
    int scrollOffset;
};

static const int kMenuHistoryMaxDepth = 4;
static MenuHistoryEntry s_menuHistory[kMenuHistoryMaxDepth];
static int s_menuHistoryDepth = 0;

// 子界面 API 实现
void setCurrentInterface(InterfaceHandler handler) {
    // 进入子界面时将当前菜单上下文压栈，使返回时能回到正确菜单（而非跳过当前层）
    if (handler != nullptr && s_menuHistoryDepth < kMenuHistoryMaxDepth) {
        s_menuHistory[s_menuHistoryDepth++] = {
            s_menuContext.currentMenu,
            s_menuContext.menuCursor,
            s_menuContext.scrollOffset
        };
    }
    s_currentHandler = handler;
    s_menuContext.isDisplayNeedsUpdate = true;
}

void clearCurrentInterface() {
    s_currentHandler = nullptr;
    s_menuContext.isDisplayNeedsUpdate = true;
}

bool isInSubInterface() {
    return s_currentHandler != nullptr;
}

extern WifiConfigManager wifiConfigManager;

void _enterWirelessScreen(){
    // 进入无线界面：未联网则启动配网，已联网则展示连接信息。
    LOG_MENU_INFO("Entering Wireless Screen");
    inMenuMode = false;

    // WiFi 未连接时，启动配网
    if (WiFi.status() != WL_CONNECTED) {
        LOG_MENU_INFO("Starting AP config mode from menu");
        enterConfigMode();
    }
    // WiFi 已连接时，显示连接信息
    else{
        server.begin();
        LOG_MENU_INFO("Wireless screen ensured TCP server on port %d", CONNECT_PORT);
        lcdText("SSID:" + wifiConfigManager.getSSID(),1);
        lcdText("IP:" + WiFi.localIP().toString(),2);
    }
}

void _setClockInterface(){
    // 切换到时钟应用状态机。
    enterClockInterface();
}

void _setWeatherInterface(){
    // 切换到天气应用状态机。
    enterWeatherInterface();
}

void _playBadAppleWrapper() {
    // 播放 Bad Apple 动画文件。
    playBadAppleFromFileRaw("/badapple.bin");
}

void _startPomodoroWrapper() {
    // 启动番茄钟应用（25分钟工作 + 5分钟休息循环）。
    runPomodoroApp();
}

static const MenuNavigationCallbacks kMenuNavigationCallbacks = {
    _isMainMenuCallback,
    _shouldHideMenuItemCallback,
    _getMenuByStateCallback,
    _markDisplayNeedsUpdate,
    _onMenuPostActivate
};

// =====================
// 内部辅助函数实现
// =====================

static inline bool _isMainMenu(const Menu* menu) {
    // 判断当前菜单是否为主菜单。
    return menu == &allMenus[MENU_MAIN];
}

static bool _isMainMenuCallback(const Menu* menu) {
    return _isMainMenu(menu);
}

static bool _shouldHideMenuItemCallback(const Menu* menu, int menuItemIndex) {
    return _shouldHideMenuItem(menu, menuItemIndex);
}

static const Menu* _getMenuByStateCallback(MenuState state) {
    return _getMenuByState(state);
}

static void _markDisplayNeedsUpdate() {
    s_menuContext.isDisplayNeedsUpdate = true;
}

static void _onMenuPostActivate() {
    globalButtonDelay(FIRST_TIME_DELAY);
}

// 读取一次“有效按键”（带去抖），未检测到返回 -1
static int _readMenuButtonOnce() {
    for (int i = 0; i < 5; i++) {
        if (isButtonReadyToRespond(i, BUTTON_DEBOUNCE_DELAY)) {
            return i;
        }
    }
    return -1;
}

// 渲染普通菜单项一行（带 > 光标）
static void _renderMenuItemLine(const Menu* menu, int menuItemIndex, int visibleIndex, int lcdLine) {
    // 渲染一行菜单项，并按可见索引决定光标位置。
    if (!menu || menuItemIndex < 0 || menuItemIndex >= menu->itemCount) {
        lcdText(" ", lcdLine);
        return;
    }

    String itemName = menu->items[menuItemIndex].name;

    // 主菜单第0项：根据 WiFi 状态动态显示名称
    if (_isMainMenu(menu) && menuItemIndex == 0) {
        itemName = (wifiConnectionState == WIFI_CONNECTED) ? "Wireless Screen" : "Config WiFi";
    }

    if (s_menuContext.menuCursor == visibleIndex) {
        lcdText(">" + itemName, lcdLine);
    } else {
        lcdText(" " + itemName, lcdLine);
    }
}

static bool _shouldHideMenuItem(const Menu* menu, int menuItemIndex) {
    // 根据当前运行状态决定菜单项是否隐藏。
    if (!menu || menuItemIndex < 0 || menuItemIndex >= menu->itemCount) {
        return false;
    }

    // 设置菜单中：启用自动亮度时隐藏手动亮度调节项
    if (menu == &allMenus[MENU_SETTINGS]
        && menuItemIndex == SETTINGS_ITEM_BRIGHTNESS
        && isAutoBrightnessActive()) {
        return true;
    }

    return false;
}

// 初始化
void initMenu() {
    // 初始化菜单任务与状态栏动画资源。
    if(inConfigMode) return; // 配置模式不启动菜单任务

    statusBarRendererInit();

    xTaskCreatePinnedToCore(_menuTask, "_menuTask", 16384, NULL, 1, &_menuTaskHandle, 0);
}

void _displayMenu(const Menu* menu, int menuIndex, int scrollOffset) {
    // 根据菜单类型渲染可见窗口内容。
    // menuIndex / scrollOffset 由调用者传入（由 MenuContext 持有），这里保持参数以便调试
    lcdResetCursor();
    
    // 检查是否为主菜单，如果是则显示状态栏
    if (menu == &allMenus[MENU_MAIN]) {
        // 主菜单：状态栏作为第 -1 项（虚拟项），可滚动但不可选中
        for (int i = 0; i < VISIBLE_LINES; i++) {
            int displayIndex = scrollOffset + i;
            
            if (displayIndex == -1) {
                // 状态栏固定渲染在第 1 行
                renderStatusBar();
            } else {
                // 普通菜单项
                _renderMenuItemLine(menu, displayIndex, displayIndex, i + 1);
            }
        }
    } else {
        // 其他菜单：纯列表显示，不包含状态栏
        for (int i = 0; i < VISIBLE_LINES; i++) {
            int visibleIndex = scrollOffset + i;
            int menuItemIndex = menuNavigatorGetActualMenuIndexFromVisible(menu, visibleIndex, &kMenuNavigationCallbacks);

            if (menuItemIndex < 0) {
                lcdText(" ", i + 1);
                continue;
            }

            _renderMenuItemLine(menu, menuItemIndex, visibleIndex, i + 1);
        }
    }
}

// 根据菜单枚举获取对应菜单结构体指针
const Menu* _getMenuByState(MenuState state) {
    // 将菜单状态枚举转换为菜单结构体指针。
    // MenuState 与 allMenus 的顺序一致，直接按索引取更简单
    if (state >= MENU_MAIN && state <= MENU_ABOUT) {
        return &allMenus[(int)state];
    }
    return NULL;
}

static MenuState _getMenuStateFromPtr(const Menu* menu) {
    if (menu == nullptr) {
        return MENU_NONE;
    }

    for (int i = MENU_MAIN; i <= MENU_ABOUT; ++i) {
        if (menu == &allMenus[i]) {
            return static_cast<MenuState>(i);
        }
    }

    return MENU_NONE;
}

static MenuState _getParentMenuState(MenuState childState) {
    switch (childState) {
        case MENU_WIFI_CONFIG:
            return MENU_SETTINGS;
        case MENU_SETTINGS:
        case MENU_ABOUT:
            return MENU_MAIN;
        case MENU_MAIN:
            return MENU_MAIN;
        default:
            return MENU_MAIN;
    }
}

void menuHandleBackAction() {
    // 非菜单页面（含无线显示/应用页面）统一回主菜单。
    if (!inMenuMode) {
        buzzerPlayBackSound();
        inMenuMode = true;
        clearCurrentInterface();
        s_menuContext.currentMenu = &allMenus[MENU_MAIN];
        s_menuContext.menuCursor = 0;
        s_menuContext.scrollOffset = -1;
        s_menuContext.isDisplayNeedsUpdate = true;
        s_menuHistoryDepth = 0;  // 清空历史栈
        globalButtonDelay(FIRST_TIME_DELAY);
        LOG_MENU_INFO("Power short press: return to main menu");
        return;
    }

    // 菜单内返回上一级：优先从历史栈恢复光标位置。
    buzzerPlayBackSound();
    if (s_menuHistoryDepth > 0) {
        const MenuHistoryEntry& e = s_menuHistory[--s_menuHistoryDepth];
        s_menuContext.currentMenu = e.menu;
        s_menuContext.menuCursor = e.menuCursor;
        s_menuContext.scrollOffset = e.scrollOffset;
    } else {
        // 历史栈为空（已在主菜单），保持不动
        const MenuState currentMenuState = _getMenuStateFromPtr(s_menuContext.currentMenu);
        const MenuState targetMenuState = _getParentMenuState(currentMenuState);
        s_menuContext.currentMenu = &allMenus[targetMenuState];
        s_menuContext.menuCursor = 0;
        s_menuContext.scrollOffset = (targetMenuState == MENU_MAIN) ? -1 : 0;
    }
    s_menuContext.isDisplayNeedsUpdate = true;
    clearCurrentInterface();
    globalButtonDelay(FIRST_TIME_DELAY);
    LOG_MENU_INFO("Power short press: back to menu, history depth=%d", s_menuHistoryDepth);
}

void menuReturnToCurrentSubMenu() {
    // 弹栈恢复进入子界面前的菜单上下文（与 menuHandleBackAction 同路径）
    buzzerPlayBackSound();
    if (s_menuHistoryDepth > 0) {
        const MenuHistoryEntry& e = s_menuHistory[--s_menuHistoryDepth];
        s_menuContext.currentMenu = e.menu;
        s_menuContext.menuCursor = e.menuCursor;
        s_menuContext.scrollOffset = e.scrollOffset;
        s_menuContext.isDisplayNeedsUpdate = true;
    }
    clearCurrentInterface();
    globalButtonDelay(FIRST_TIME_DELAY);
}

static void _syncMenuNavigationState() {
    MenuNavigationContext context = {
        s_menuContext.currentMenu,
        s_menuContext.menuCursor,
        s_menuContext.scrollOffset
    };
    menuNavigatorSyncState(&context, &kMenuNavigationCallbacks, VISIBLE_LINES);
    s_menuContext.currentMenu = context.currentMenu;
    s_menuContext.menuCursor = context.menuCursor;
    s_menuContext.scrollOffset = context.scrollOffset;
}

static void _moveMenuCursorUp() {
    MenuNavigationContext context = {
        s_menuContext.currentMenu,
        s_menuContext.menuCursor,
        s_menuContext.scrollOffset
    };
    menuNavigatorMoveUp(&context, &kMenuNavigationCallbacks, VISIBLE_LINES);
    s_menuContext.currentMenu = context.currentMenu;
    s_menuContext.menuCursor = context.menuCursor;
    s_menuContext.scrollOffset = context.scrollOffset;
}

static void _moveMenuCursorDown() {
    MenuNavigationContext context = {
        s_menuContext.currentMenu,
        s_menuContext.menuCursor,
        s_menuContext.scrollOffset
    };
    menuNavigatorMoveDown(&context, &kMenuNavigationCallbacks, VISIBLE_LINES);
    s_menuContext.currentMenu = context.currentMenu;
    s_menuContext.menuCursor = context.menuCursor;
    s_menuContext.scrollOffset = context.scrollOffset;
}

static void _activateCurrentMenuItem() {
    const Menu* menuBeforeActivate = s_menuContext.currentMenu;
    const int cursorBeforeActivate = s_menuContext.menuCursor;
    const int scrollBeforeActivate = s_menuContext.scrollOffset;

    MenuNavigationContext context = {
        s_menuContext.currentMenu,
        s_menuContext.menuCursor,
        s_menuContext.scrollOffset
    };
    menuNavigatorActivateCurrentItem(&context, &kMenuNavigationCallbacks, VISIBLE_LINES);
    s_menuContext.currentMenu = context.currentMenu;
    s_menuContext.menuCursor = context.menuCursor;
    s_menuContext.scrollOffset = context.scrollOffset;

    // 菜单发生了切换
    if (s_menuContext.currentMenu != menuBeforeActivate) {
        // 检查目标菜单是否在历史栈中（Return 项跳回父菜单的情况）
        bool restoredFromHistory = false;
        for (int i = s_menuHistoryDepth - 1; i >= 0; i--) {
            if (s_menuHistory[i].menu == s_menuContext.currentMenu) {
                // 恢复光标并裁剪历史栈
                s_menuContext.menuCursor = s_menuHistory[i].menuCursor;
                s_menuContext.scrollOffset = s_menuHistory[i].scrollOffset;
                s_menuContext.isDisplayNeedsUpdate = true;
                s_menuHistoryDepth = i;
                restoredFromHistory = true;
                break;
            }
        }
        // 目标菜单不在栈中：正向进入子菜单，压栈
        if (!restoredFromHistory && s_menuHistoryDepth < kMenuHistoryMaxDepth) {
            s_menuHistory[s_menuHistoryDepth++] = {menuBeforeActivate, cursorBeforeActivate, scrollBeforeActivate};
        }
    }
}

static void _handleMenuState() {
    // 菜单状态处理入口，同时维持状态栏动画刷新。
    _handleMenuInterface();
    if (s_menuContext.scrollOffset == -1 && Animations::getNeedToUpdate()) {
        s_menuContext.isDisplayNeedsUpdate = true;
    }
}

static void _dispatchCurrentInterfaceState() {
    // 直接调用已注册的处理函数；为 null 则进入菜单状态。
    if (s_currentHandler != nullptr) {
        s_currentHandler();
    } else {
        _handleMenuState();
    }
}

void _handleMenuInterface() {
    // 处理菜单输入与选择逻辑。
    _syncMenuNavigationState();

    // 显示菜单项
    if (s_menuContext.isDisplayNeedsUpdate || _checkStateChanges()) {
        _displayMenu(s_menuContext.currentMenu, s_menuContext.menuCursor, s_menuContext.scrollOffset);
        s_menuContext.isDisplayNeedsUpdate = false;
    }

    // 只处理一个键：一次循环最多响应一次输入
    int lastPressedButton = _readMenuButtonOnce();

    switch (lastPressedButton) {
        // 光标上移（LEFT）
        case LEFT:
            buzzerPlayNavigateSound();
            _moveMenuCursorUp();
            break;

        // 光标下移（RIGHT）
        case RIGHT:
            buzzerPlayNavigateSound();
            _moveMenuCursorDown();
            break;

        case CENTER:
            buzzerPlaySelectSound();
            _activateCurrentMenuItem();
            break;

        default:
            break;
    }
}

// 检查状态变化，返回是否需要更新显示
bool _checkStateChanges() {
    // 检测状态栏相关状态变化，决定是否需要重绘。
    return statusBarRendererNeedsRedraw();
}

// 菜单任务
TaskHandle_t _menuTaskHandle = NULL;
void _menuTask(void* parameter) {
    // 菜单后台任务：维护状态栏缓存、动画更新与状态分发。
    while (!shouldExitTasks) {
        if (powerKeyOverlayActive) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        statusBarRendererTick();
        
        if (inMenuMode) {
            _dispatchCurrentInterfaceState();
        }
        const bool menuIdleNoSync = inMenuMode
            && !inConfigMode
            && !clientConnected
            && (timeSyncState != TIME_SYNC_IN_PROGRESS);
        vTaskDelay(pdMS_TO_TICKS(menuIdleNoSync ? 120 : 50));
    }
    
    // 任务退出清理
    LOG_MENU_DEBUG("_menuTask exiting...");
    _menuTaskHandle = NULL;
    vTaskDelete(NULL);
}