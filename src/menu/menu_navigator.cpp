#include "./menu/menu_navigator.h"

int menuNavigatorGetVisibleMenuItemCount(const Menu* menu, const MenuNavigationCallbacks* callbacks) {
    if (!menu || !callbacks || !callbacks->shouldHideItem) {
        return 0;
    }

    int visibleCount = 0;
    for (int i = 0; i < menu->itemCount; ++i) {
        if (!callbacks->shouldHideItem(menu, i)) {
            visibleCount++;
        }
    }
    return visibleCount;
}

int menuNavigatorGetActualMenuIndexFromVisible(const Menu* menu, int visibleIndex, const MenuNavigationCallbacks* callbacks) {
    if (!menu || !callbacks || !callbacks->shouldHideItem || visibleIndex < 0) {
        return -1;
    }

    int currentVisibleIndex = 0;
    for (int i = 0; i < menu->itemCount; ++i) {
        if (callbacks->shouldHideItem(menu, i)) {
            continue;
        }

        if (currentVisibleIndex == visibleIndex) {
            return i;
        }
        currentVisibleIndex++;
    }

    return -1;
}

void menuNavigatorFollowCursorToKeepVisible(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines) {
    if (!context || !callbacks || !context->currentMenu || visibleLines <= 0) {
        return;
    }

    if (context->scrollOffset < 0) {
        return; // -1 表示主菜单显示状态栏
    }

    int visibleCount = menuNavigatorGetVisibleMenuItemCount(context->currentMenu, callbacks);
    if (visibleCount <= 0) {
        context->menuCursor = 0;
        context->scrollOffset = 0;
        return;
    }

    context->menuCursor = constrain(context->menuCursor, 0, visibleCount - 1);

    if (context->menuCursor < context->scrollOffset) {
        context->scrollOffset = context->menuCursor;
    } else if (context->menuCursor >= context->scrollOffset + visibleLines) {
        context->scrollOffset = context->menuCursor - visibleLines + 1;
    }

    int maxScrollOffset = visibleCount - visibleLines;
    if (maxScrollOffset < 0) {
        maxScrollOffset = 0;
    }
    context->scrollOffset = constrain(context->scrollOffset, 0, maxScrollOffset);
}

void menuNavigatorSyncState(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines) {
    if (!context || !callbacks || !context->currentMenu || !callbacks->isMainMenu) {
        return;
    }

    const bool isMain = callbacks->isMainMenu(context->currentMenu);
    const int visibleItemCount = menuNavigatorGetVisibleMenuItemCount(context->currentMenu, callbacks);

    if (visibleItemCount <= 0) {
        context->menuCursor = 0;
        context->scrollOffset = isMain ? -1 : 0;
        return;
    }

    context->menuCursor = constrain(context->menuCursor, 0, visibleItemCount - 1);

    if (isMain) {
        if (context->scrollOffset < -1) {
            context->scrollOffset = -1;
        }
    } else if (context->scrollOffset < 0) {
        context->scrollOffset = 0;
    }

    menuNavigatorFollowCursorToKeepVisible(context, callbacks, visibleLines);
}

void menuNavigatorSwitchToMenu(MenuNavigationContext* context, const Menu* menu, const MenuNavigationCallbacks* callbacks, int visibleLines) {
    if (!context || !callbacks || !callbacks->isMainMenu || !menu) {
        return;
    }

    context->currentMenu = menu;
    context->menuCursor = 0;
    context->scrollOffset = callbacks->isMainMenu(menu) ? -1 : 0;
    menuNavigatorSyncState(context, callbacks, visibleLines);

    if (callbacks->onDisplayNeedsUpdate) {
        callbacks->onDisplayNeedsUpdate();
    }
}

void menuNavigatorMoveUp(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines) {
    if (!context || !callbacks || !callbacks->isMainMenu) {
        return;
    }

    menuNavigatorSyncState(context, callbacks, visibleLines);

    const bool isMain = callbacks->isMainMenu(context->currentMenu);
    if (isMain) {
        if (context->menuCursor > 0) {
            context->menuCursor--;
        } else if (context->scrollOffset > -1) {
            context->scrollOffset = -1;
        }
    } else if (context->menuCursor > 0) {
        context->menuCursor--;
    }

    menuNavigatorSyncState(context, callbacks, visibleLines);
    if (callbacks->onDisplayNeedsUpdate) {
        callbacks->onDisplayNeedsUpdate();
    }
}

void menuNavigatorMoveDown(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines) {
    if (!context || !callbacks || !callbacks->isMainMenu) {
        return;
    }

    menuNavigatorSyncState(context, callbacks, visibleLines);

    const int visibleItemCount = menuNavigatorGetVisibleMenuItemCount(context->currentMenu, callbacks);
    if (visibleItemCount <= 0) {
        if (callbacks->onDisplayNeedsUpdate) {
            callbacks->onDisplayNeedsUpdate();
        }
        return;
    }

    if (callbacks->isMainMenu(context->currentMenu) && context->scrollOffset == -1 && context->menuCursor == 0) {
        context->scrollOffset = 0;
    } else {
        context->menuCursor = constrain(context->menuCursor + 1, 0, visibleItemCount - 1);
    }

    menuNavigatorSyncState(context, callbacks, visibleLines);
    if (callbacks->onDisplayNeedsUpdate) {
        callbacks->onDisplayNeedsUpdate();
    }
}

void menuNavigatorActivateCurrentItem(MenuNavigationContext* context, const MenuNavigationCallbacks* callbacks, int visibleLines) {
    if (!context || !callbacks || !callbacks->getMenuByState) {
        return;
    }

    menuNavigatorSyncState(context, callbacks, visibleLines);

    int actualMenuIndex = menuNavigatorGetActualMenuIndexFromVisible(context->currentMenu, context->menuCursor, callbacks);
    if (actualMenuIndex < 0) {
        if (callbacks->onDisplayNeedsUpdate) {
            callbacks->onDisplayNeedsUpdate();
        }
        return;
    }

    const MenuItem& item = context->currentMenu->items[actualMenuIndex];

    if (item.nextState != MENU_NONE) {
        const Menu* nextMenu = callbacks->getMenuByState(item.nextState);
        if (nextMenu) {
            menuNavigatorSwitchToMenu(context, nextMenu, callbacks, visibleLines);
            if (callbacks->onPostActivate) {
                callbacks->onPostActivate();
            }
            return;
        }
    }

    if (item.action) {
        item.action();
        if (callbacks->onPostActivate) {
            callbacks->onPostActivate();
        }
    }

    menuNavigatorSyncState(context, callbacks, visibleLines);
    if (callbacks->onDisplayNeedsUpdate) {
        callbacks->onDisplayNeedsUpdate();
    }
}
