#include "common.h"
#include "pwm.h"

using namespace std;
using namespace common;

static const string PWM_BASE_PATH = "/sys/class/pwm";

int main(void) {
	pwm::LinuxPwmOutput pwmBlue{ PWM_BASE_PATH, 1, 0 };
	pwm::LinuxPwmOutput pwmGreen{ PWM_BASE_PATH, 2, 0 };
	pwm::LinuxPwmOutput pwmRed{ PWM_BASE_PATH, 3, 0 };
	pwm::PwmRgbLed led(pwmRed, pwmGreen, pwmBlue);

	auto sleep = [](uint16_t duration_ms) {
		this_thread::sleep_for(chrono::milliseconds(duration_ms));
	};

	while(1) {
		led.set(RED);
		sleep(200);
		led.set(GREEN);
		sleep(200);
	}

	return 0;
}
