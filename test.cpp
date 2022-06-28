#include "Log.h"

void logTest() {
    Log::getInstance().init(10, 128);
    Log::getInstance().start();

    for (int i = 0; i < 20; i++) {
        // sleep(1);
        LOG_DEBUG("main", "%d%s", i, "jest a test");
    }

    sleep(10);
    Log::getInstance().stop();
}

int main() {
    logTest();
    return 0;
}