#include "pwm.h"

namespace pwm {

LinuxPwmOutput::LinuxPwmOutput(const std::string& basePath, unsigned int pwmchip, unsigned int pwm) :
	basePath(basePath),
	pwmchip(pwmchip),
	pwm(pwm) {

	exportPwm();
	setProperty("enable", "0");
	setProperty("period", "0");
	setProperty("duty_cycle", "0");
}

void LinuxPwmOutput::enable(bool en) {
	if (en)
		setProperty("enable", "1");
	else
		setProperty("enable", "0");
}

void LinuxPwmOutput::setPeriodNs(unsigned long int value) {
	setProperty("period", std::to_string(value));
}

void LinuxPwmOutput::setDutyCycleNs(unsigned long int value) {
	setProperty("duty_cycle", std::to_string(value));
}

void LinuxPwmOutput::exportPwm() {
	auto exportPath = basePath + std::string("/pwmchip") +
		std::to_string(pwmchip) + std::string("/export");

	int fd = open(exportPath.c_str(), O_WRONLY);
	if (fd == -1) {
		printf("ERROR while exporting %s.\n", exportPath.c_str());
		return;
	}

	auto pwmString = (std::to_string(pwm));
	size_t bytesWritten = write(fd, pwmString.c_str(), pwmString.size());
	if (bytesWritten != pwmString.size()) {
		printf("ERROR while exporting %s.\n", pwmString.c_str());
		close(fd);
		return;
	}

	close(fd);
}

void LinuxPwmOutput::setProperty(const std::string& prop, const std::string& value) {
	auto propertyPath = basePath + std::string("/pwmchip") +
		std::to_string(pwmchip) + std::string("/pwm") + 
		std::to_string(pwm) + std::string("/") + prop;
	int fd = open(propertyPath.c_str(), O_WRONLY);
	if (fd == -1) {
		printf("ERROR while opening property %s.\n", propertyPath.c_str());
		return;
	}

	size_t bytesWritten = write(fd, value.c_str(), value.size());
	if (bytesWritten != value.size()) {
		printf("ERROR while opening property %s.\n", propertyPath.c_str());
		close(fd);
		return;
	}

	close(fd);
}

PwmBeeper::PwmBeeper(PwmOutput& pwmOutput, SleepFunction sleepFunction) :
	pwmOutput(pwmOutput),
	sleepFunction(sleepFunction) {

	pwmOutput.setPeriodNs(0);
	pwmOutput.setDutyCycleNs(0);
	pwmOutput.enable(false);
}

void PwmBeeper::setVolume(uint8_t volume) {
	if (volume <= 100)
		this->volume = volume;
	else
		this->volume = 100;
}

uint8_t PwmBeeper::getVolume() {
	return volume;
}

void PwmBeeper::playTone(const common::BeeperTone& tone) {
	auto period = tone.frequency_hz ? common::GIGA / tone.frequency_hz : 0;
	pwmOutput.setPeriodNs(period);
	pwmOutput.setDutyCycleNs(period * volume / 100);
	pwmOutput.enable(true);
	sleepFunction(tone.duration_ms);
	pwmOutput.enable(false);
}

PwmRgbLed::PwmRgbLed(PwmOutput& pwmRed, PwmOutput& pwmGreen, PwmOutput& pwmBlue) :
	pwmRed(pwmRed),
	pwmGreen(pwmGreen),
	pwmBlue(pwmBlue) {

	setChannelDefaults(pwmRed);
	setChannelDefaults(pwmGreen);
	setChannelDefaults(pwmBlue);
}

unsigned long PwmRgbLed::getDefaultPeriodNs() {
	return 70000UL; // 14 kHz
}

void PwmRgbLed::set(common::LightSetting value) {
	bufferedSetting = value;
	setChannel(pwmRed, value.r);
	setChannel(pwmGreen, value.g);
	setChannel(pwmBlue, value.b);
}

common::LightSetting PwmRgbLed::get() {
	return bufferedSetting;
}

void PwmRgbLed::setChannel(PwmOutput& pwmOutput, unsigned long value) {
	pwmOutput.setPeriodNs(getDefaultPeriodNs());
	pwmOutput.setDutyCycleNs(getDefaultPeriodNs() * DUTY_CYCLE_MAX_RATIO * value / 255);
	pwmOutput.enable(true);
}

void PwmRgbLed::setChannelDefaults(PwmOutput& pwmOutput) {
	pwmOutput.setPeriodNs(getDefaultPeriodNs());
	pwmOutput.setDutyCycleNs(0);
	pwmOutput.enable(false);
}

}
