#include "common.h"
#include "pwm.h"
#include "strings.h"

namespace common {

bool operator==(const LightSetting& lhs, const LightSetting& rhs) {
	return (lhs.r == rhs.r &&
			lhs.g == rhs.g &&
			lhs.b == rhs.b);
}

bool operator!=(const LightSetting& lhs, const LightSetting& rhs) {
	return !(lhs == rhs);
}

bool operator==(const BeeperTone& lhs, const BeeperTone& rhs) {
	return (lhs.duration_ms == rhs.duration_ms &&
			lhs.frequency_hz == rhs.frequency_hz);
}

bool operator!=(const BeeperTone& lhs, const BeeperTone& rhs) {
	return !(lhs==rhs);
}

Signalizer::Signalizer(Beeper& beeper, RgbLight& rgbLight) :
	beeper(beeper),
	rgbLight(rgbLight) {
}

void Signalizer::update(BuildResult buildResult) {
	if (buildResult == BuildResult::OK) {
		rgbLight.set(LightSetting{0, 255, 0});
	} else if (buildResult == BuildResult::BROKEN) {
		rgbLight.set(LightSetting{255, 0, 0});

		BeeperTone errorTone;
		errorTone.duration_ms = 1000;
		errorTone.frequency_hz = 1000;
		beeper.playTone(errorTone);
	}
}

BuildResult JenkinsBuildResultParser::parseMsg(const std::string& msg) {
	if (msg.find("SUCCESS") != std::string::npos)
		return BuildResult::OK;
	else if (msg.find("FAILURE") != std::string::npos)
		return BuildResult::BROKEN;
	return BuildResult::DONTKNOW;
}

StateSaver::StateSaver(KeyValueStore& store, RgbLight& rgbLight) :
	store(store),
	rgbLight(rgbLight) {
}

void StateSaver::saveCurrentLightSetting() {
	auto lightSetting = rgbLight.get();
	store.set(strings::LED_R, std::to_string(lightSetting.r));
	store.set(strings::LED_G, std::to_string(lightSetting.g));
	store.set(strings::LED_B, std::to_string(lightSetting.b));
}

void StateSaver::restoreLightSetting() {
	auto r = convert(store.get(strings::LED_R));
	auto g = convert(store.get(strings::LED_G));
	auto b = convert(store.get(strings::LED_B));
	rgbLight.set(LightSetting{r, g, b});
}

uint8_t StateSaver::convert(const std::string& str) {
	if (str.size() == 0)
		return 0;

	auto value = stoi(str);
	if (value > 255)
		throw std::runtime_error("Stored value too high");

	return (uint8_t)value;
}

}
