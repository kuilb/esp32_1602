/**
 * @file status_bar_renderer.h
 * @brief 主菜单状态栏渲染与状态跟踪接口
 */
#ifndef STATUS_BAR_RENDERER_H
#define STATUS_BAR_RENDERER_H

/** @brief 初始化状态栏渲染器（动画注册、初始缓存） */
void statusBarRendererInit();

/** @brief 每轮调用一次，更新动画与电池缓存 */
void statusBarRendererTick();

/** @brief 渲染状态栏（电池/WiFi/时间） */
void renderStatusBar();

/** @brief 判断状态栏是否需要重绘 */
bool statusBarRendererNeedsRedraw();

#endif
