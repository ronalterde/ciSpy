#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include "common.h"

namespace pwm {

class PwmOutput {
public:
	virtual ~PwmOutput() {}

	virtual void enable(bool en) = 0;
	virtual void setPeriodNs(unsigned long int value) = 0;
	virtual void setDutyCycleNs(unsigned long int value) = 0;
};

class LinuxPwmOutput : public PwmOutput {
public:
	LinuxPwmOutput(const std::string& basePath, unsigned int pwmchip, unsigned int pwm);
	void enable(bool en) override;
	void setPeriodNs(unsigned long int value) override;
	void setDutyCycleNs(unsigned long int value) override;

private:
	void exportPwm();
	void setProperty(const std::string& prop, const std::string& value);

private:
	std::string basePath;
	unsigned int pwmchip;
	unsigned int pwm;
};

class PwmBeeper : public common::Beeper {
public:
	using SleepFunction = std::function<void(uint16_t)>;

	PwmBeeper(PwmOutput& pwmOutput, SleepFunction sleepFunction);
	void setVolume(uint8_t volume) override;
	uint8_t getVolume() override;
	void playTone(const common::BeeperTone& tone) override;

private:
	PwmOutput& pwmOutput;
	SleepFunction sleepFunction;
	uint8_t volume{50};
};

class PwmRgbLed : public common::RgbLight {
public:
	PwmRgbLed(PwmOutput& pwmRed, PwmOutput& pwmGreen, PwmOutput& pwmBlue);

	/**
	 * This maximum is due to wrong hardware dimensioning (LEDs get too hot).
	 */
	static constexpr double DUTY_CYCLE_MAX_RATIO{0.8};

	/**
	 * For LEDs the period may remain constant.
	 * It is fine-tuned in order to avoid audible crosstalk effects.
	 */
	unsigned long getDefaultPeriodNs();
	void set(common::LightSetting value);
	common::LightSetting get();

private:
	void setChannel(PwmOutput& pwmOutput, unsigned long value);
	void setChannelDefaults(PwmOutput& pwmOutput);

private:
	common::LightSetting bufferedSetting;
	PwmOutput& pwmRed;
	PwmOutput& pwmGreen;
	PwmOutput& pwmBlue;
};

} // namespace pwm
