#include <pigpio.h>
#include <stdio.h>

#define PIN 4 

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialisation failed.\n");
        return 1;
    }

    gpioSetMode(PIN, PI_OUTPUT);

    gpioWrite(PIN, 1);
    printf("GPIO4 set to HIGH\n");
    time_sleep(5);

    gpioWrite(PIN, 0);
    printf("GPIO4 set to LOW\n");

    gpioTerminate();
    return 0;
}
