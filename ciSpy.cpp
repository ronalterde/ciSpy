#include "common.h"
#include "pwm.h"
#include "network.h"
#include "filesystem.h"

using namespace std;

static const uint16_t LISTEN_PORT{5555};
static const string PWM_BASE_PATH = "/sys/class/pwm";
static const string STORE_FILE =  "/var/local/ciSpy-store";

int main(void) {
	pwm::LinuxPwmOutput pwmBeeper{ PWM_BASE_PATH, 0, 0 };
	pwm::LinuxPwmOutput pwmBlue{ PWM_BASE_PATH, 1, 0 };
	pwm::LinuxPwmOutput pwmGreen{ PWM_BASE_PATH, 2, 0 };
	pwm::LinuxPwmOutput pwmRed{ PWM_BASE_PATH, 3, 0 };

	auto sleepFunction = [](uint16_t duration_ms) {
		this_thread::sleep_for(chrono::milliseconds(duration_ms));
	};
	pwm::PwmBeeper beeper(pwmBeeper, sleepFunction);
	pwm::PwmRgbLed led(pwmRed, pwmGreen, pwmBlue);
	common::Signalizer signalizer{beeper, led};

	filesystem::FileStreamFactory fac;
	filesystem::FileStore fileStore(fac, STORE_FILE);
	common::StateSaver stateSaver{fileStore, led};
	stateSaver.restoreLightSetting();

	network::TcpServer tcpServer{LISTEN_PORT};
	common::JenkinsBuildResultParser buildResultParser;

	while(1) {
		auto msg = tcpServer.receiveClientMsg();
		auto result = buildResultParser.parseMsg(msg);
		signalizer.update(result);
		stateSaver.saveCurrentLightSetting();
	}

	return 0;
}
