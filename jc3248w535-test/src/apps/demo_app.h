#pragma once
#include "../drivers/axs15231b_qspi.h"   // <-- правильный относительный путь

namespace DemoApp {
    void runPhase(AXS15231B_QSPI* lcd, uint8_t phase);
}