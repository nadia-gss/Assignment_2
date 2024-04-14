#include <pigpio.h>
#include <stdio.h>

#define PIN 4  // GPIO4 对应的BCM引脚编号

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialisation failed.\n");
        return 1;
    }

    // 设置 PIN 为输出
    gpioSetMode(PIN, PI_OUTPUT);

    // 拉高 GPIO4
    gpioWrite(PIN, 1);
    printf("GPIO4 set to HIGH\n");
    time_sleep(5); // 等待5秒

    // 拉低 GPIO4
    gpioWrite(PIN, 0);
    printf("GPIO4 set to LOW\n");

    // 清理并退出
    gpioTerminate();
    return 0;
}
