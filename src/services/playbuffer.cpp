#include "./services/playbuffer.h"

#include <esp_timer.h>
#include "./services/frame_stats.h"

unsigned long lastDisplayTime = 0;   // 上一帧显示时间戳
bool isDisplayingCache = false;      // 当前是否正在播放缓存

// ==============================
// T2→T4 端到端延迟统计
//   T2 = 帧入队时刻 (enqueueMs，毫秒)
//   T3 = processIncoming 开始 (esp_timer_get_time，微秒)
//   T4 = processIncoming 返回，即 LCD GPIO 写完
// ==============================
static uint32_t s_latFrames    = 0;
static uint64_t s_latQueueUs   = 0;   // 累计的排队延迟 (T2→T3)，微秒
static uint64_t s_latRenderUs  = 0;   // 累计的渲染延迟 (T3→T4)，微秒
static uint32_t s_latMaxTotalUs = 0;  // 最大端到端延迟 (T2→T4)

static constexpr uint32_t kLatencyLogInterval = 300; // 每 300 帧输出一次汇总

static void _measureAndDisplay(const FramePacket& pkt) {
    const uint32_t now = millis();
    const uint32_t queueMs = (uint32_t)(now - pkt.enqueueMs);

    const int64_t t3 = esp_timer_get_time();
    processIncoming(pkt.data.data(), pkt.data.size());
    const int64_t t4 = esp_timer_get_time();

    const uint32_t renderUs  = static_cast<uint32_t>(t4 - t3);
    const uint32_t totalUs   = queueMs * 1000u + renderUs;

    s_latQueueUs  += queueMs * 1000u;
    s_latRenderUs += renderUs;
    if (totalUs > s_latMaxTotalUs) s_latMaxTotalUs = totalUs;
    ++s_latFrames;

    if (s_latFrames % kLatencyLogInterval == 0) {
        const uint32_t avgQueueUs  = static_cast<uint32_t>(s_latQueueUs  / s_latFrames);
        const uint32_t avgRenderUs = static_cast<uint32_t>(s_latRenderUs / s_latFrames);
        const uint32_t avgTotalUs  = avgQueueUs + avgRenderUs;
        LOG_DISPLAY_INFO(
            "latency[%lu frames] queue_avg=%lu us  render_avg=%lu us  total_avg=%lu us  total_max=%lu us",
            (unsigned long)s_latFrames,
            (unsigned long)avgQueueUs,
            (unsigned long)avgRenderUs,
            (unsigned long)avgTotalUs,
            (unsigned long)s_latMaxTotalUs
        );
    }
}

// 播放缓存内容
void tryDisplayCachedFrames() {
    if (frameCache.empty()) {
        isDisplayingCache = false;
        return;
    }

    unsigned long now = millis();

    // 丢弃过期帧（仅 MAX_LATENCY_MS 一道防线，不再按队列深度跳帧）
    while (!frameCache.empty() && (uint32_t)(now - frameCache.front().enqueueMs) > (uint32_t)MAX_LATENCY_MS) {
        frameCache.pop_front();
        gFramesDropped++;
    }
    if (frameCache.empty()) {
        isDisplayingCache = false;
        return;
    }

    // 立即显示第一帧（首次触发播放）
    if (!isDisplayingCache) {
        isDisplayingCache = true;
        lastDisplayTime = now;
        _measureAndDisplay(frameCache.front());
        LOG_DISPLAY_VERBOSE("display buffer (first)");
        frameCache.pop_front();
        return;
    }

    // 匹配帧率：当播放落后时，小批量追帧以提升实际显示FPS，降低实时替换丢帧。
    static constexpr uint8_t kMaxBurstPerTick = 3;
    uint8_t displayedThisTick = 0;

    while (!frameCache.empty() && displayedThisTick < kMaxBurstPerTick) {
        const uint16_t interval = frameCache.front().frameIntervalMs;

        // 立即帧（interval=0）在网络侧已做“只保留最新帧”，这里每轮仅显示一帧避免占满CPU。
        if (interval == 0) {
            lastDisplayTime = now;
            _measureAndDisplay(frameCache.front());
            frameCache.pop_front();
            displayedThisTick++;
            break;
        }

        if ((uint32_t)(now - lastDisplayTime) < interval) {
            break;
        }

        lastDisplayTime += interval; // 累加而非赋值 now，避免吞帧后的连续加速
        // 若落差超过 2 帧说明卡帧较久，直接对齐 now 防止追帧风暴
        if ((uint32_t)(now - lastDisplayTime) > (uint32_t)(interval * 2)) {
            lastDisplayTime = now;
        }

        _measureAndDisplay(frameCache.front());
        frameCache.pop_front();
        displayedThisTick++;

        // 刷新当前时刻，避免连续渲染时使用过旧的时间戳。
        now = millis();
    }

    if (displayedThisTick > 1) {
        LOG_DISPLAY_VERBOSE("display buffer (burst=%u)", static_cast<unsigned int>(displayedThisTick));
    } else if (displayedThisTick == 1) {
        LOG_DISPLAY_VERBOSE("display buffer (interval)");
    }

    // 如果播放完了,清除播放状态
    if (frameCache.empty()) {
        isDisplayingCache = false;
    }
}