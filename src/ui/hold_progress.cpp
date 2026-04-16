#include "./ui/hold_progress.h"
#include "./hardware/lcd_driver.h"

namespace {
const uint8_t kProgressGlyphs[6][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C},
    {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E},
    {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}
};

void _prepareProgressGlyphs() {
    for (int slot = 0; slot <= 5; ++slot) {
        lcdCreateChar(slot, kProgressGlyphs[slot]);
    }
}

void _fitToLcdLine(const char* src, char out[17]) {
    if (!src) {
        src = "";
    }
    snprintf(out, 17, "%-16.16s", src);
}
}

void renderHoldProgressBar(const char* title, uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }

    char line1[17];
    _fitToLcdLine(title, line1);
    lcdText(line1, 1);

    _prepareProgressGlyphs();

    const int barSlots = 16;
    const int cellCols = 5;
    const int gapCols = 1;
    const int totalVirtualCols = barSlots * cellCols + (barSlots - 1) * gapCols;
    const int filledVirtualCols = (percent * totalVirtualCols) / 100;

    lcdSetCursor(16);
    for (int slot = 0; slot < barSlots; ++slot) {
        const int cellStart = slot * (cellCols + gapCols);
        int fillInCell = filledVirtualCols - cellStart;
        if (fillInCell < 0) fillInCell = 0;
        if (fillInCell > cellCols) fillInCell = cellCols;

        if (fillInCell == 0) {
            lcdDisChar(' ');
        } else {
            lcdDisCustom(fillInCell);
        }
    }
}
