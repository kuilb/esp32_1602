/**
 * @file menu_navigator.h
 * @brief 菜单导航内核接口，负责光标/滚动/可见项映射与激活流程
 *
 * 该模块通过上下文与回调解耦导航算法与业务规则：
 * - 上下文保存当前菜单、光标与滚动状态
 * - 回调提供主菜单判定、隐藏规则、跳转查表与事件通知
 */
#ifndef MENU_NAVIGATOR_H
#define MENU_NAVIGATOR_H

#include "./menu/menu.h"

/** @brief 菜单导航运行时上下文 */
typedef struct MenuNavigationContext {
    const Menu* currentMenu; /**< 当前菜单指针 */
    int menuCursor;          /**< 光标在可见项中的索引 */
    int scrollOffset;        /**< 列表滚动偏移（主菜单可为 -1 显示状态栏） */
} MenuNavigationContext;

/** @brief 菜单导航回调集合（由上层注入业务规则） */
typedef struct MenuNavigationCallbacks {
    bool (*isMainMenu)(const Menu* menu);                           /**< 判断是否主菜单 */
    bool (*shouldHideItem)(const Menu* menu, int menuItemIndex);    /**< 菜单项是否应隐藏 */
    const Menu* (*getMenuByState)(MenuState state);                 /**< 根据状态获取菜单指针 */
    void (*onDisplayNeedsUpdate)();                                 /**< 标记界面需要重绘 */
    void (*onPostActivate)();                                       /**< 激活后回调（如按键防抖） */
} MenuNavigationCallbacks;

/**
 * @brief 统计菜单可见项数量
 * @param menu 菜单指针
 * @param callbacks 导航回调集合
 * @return 可见项数量
 */
int menuNavigatorGetVisibleMenuItemCount(const Menu* menu, const MenuNavigationCallbacks* callbacks);

/**
 * @brief 将可见索引映射为真实菜单项索引
 * @param menu 菜单指针
 * @param visibleIndex 可见索引
 * @param callbacks 导航回调集合
 * @return 真实索引；无效时返回 -1
 */
int menuNavigatorGetActualMenuIndexFromVisible(const Menu* menu, int visibleIndex, const MenuNavigationCallbacks* callbacks);

/**
 * @brief 根据光标位置修正滚动窗口，确保当前项可见
 * @param context 导航上下文
 * @param callbacks 导航回调集合
 * @param visibleLines 列表可显示行数
 */
void menuNavigatorFollowCursorToKeepVisible(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines);

/**
 * @brief 收敛导航状态（光标边界、滚动边界、主菜单状态栏规则）
 * @param context 导航上下文
 * @param callbacks 导航回调集合
 * @param visibleLines 列表可显示行数
 */
void menuNavigatorSyncState(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines);

/**
 * @brief 切换到指定菜单并重置导航状态
 * @param context 导航上下文
 * @param menu 目标菜单
 * @param callbacks 导航回调集合
 * @param visibleLines 列表可显示行数
 */
void menuNavigatorSwitchToMenu(MenuNavigationContext* context, const Menu* menu, const MenuNavigationCallbacks* callbacks, int visibleLines);

/**
 * @brief 光标上移
 * @param context 导航上下文
 * @param callbacks 导航回调集合
 * @param visibleLines 列表可显示行数
 */
void menuNavigatorMoveUp(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines);

/**
 * @brief 光标下移
 * @param context 导航上下文
 * @param callbacks 导航回调集合
 * @param visibleLines 列表可显示行数
 */
void menuNavigatorMoveDown(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines);

/**
 * @brief 激活当前光标项（优先跳转菜单，否则执行动作）
 * @param context 导航上下文
 * @param callbacks 导航回调集合
 * @param visibleLines 列表可显示行数
 */
void menuNavigatorActivateCurrentItem(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines);

#endif
