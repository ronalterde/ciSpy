#pragma once

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <string>
#include <exception>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace common {

const unsigned long GIGA{1000000000UL};

class LightSetting {
public:
	LightSetting() : LightSetting(0, 0, 0) {
	}

	LightSetting(uint8_t r, uint8_t g, uint8_t b) :
		r(r),
		g(g),
		b(b) {
	}

	uint8_t r{0};
	uint8_t g{0};
	uint8_t b{0};
};

bool operator==(const LightSetting& lhs, const LightSetting& rhs);
bool operator!=(const LightSetting& lhs, const LightSetting& rhs);

const common::LightSetting RED{255, 0, 0};
const common::LightSetting GREEN{0, 255, 0};

class RgbLight {
public:
	virtual ~RgbLight() {}
	virtual void set(LightSetting value) = 0;
	virtual LightSetting get() = 0;
};

struct BeeperTone {
	BeeperTone(uint16_t duration_ms, uint16_t frequency_hz) :
		duration_ms(duration_ms),
		frequency_hz(frequency_hz) {
	}

	BeeperTone() : BeeperTone(0, 0) {
	}

	uint16_t duration_ms{0};
	uint16_t frequency_hz{0};
};

bool operator==(const BeeperTone& lhs, const BeeperTone& rhs);
bool operator!=(const BeeperTone& lhs, const BeeperTone& rhs);

class Beeper {
public:
	virtual ~Beeper() {}
	virtual void setVolume(uint8_t volume) = 0;
	virtual uint8_t getVolume() = 0;
	virtual void playTone(const BeeperTone& tone) = 0;
};

enum class BuildResult {
	OK,
	BROKEN,
	DONTKNOW
};

class Signalizer {
public:
	Signalizer(Beeper& beeper, RgbLight& rgbLight);
	void update(BuildResult buildResult);

private:
	Beeper& beeper;
	RgbLight& rgbLight;
};

class BuildResultParser {
	virtual BuildResult parseMsg(const std::string& msg) = 0;
};

class JenkinsBuildResultParser : public BuildResultParser {
public:
	BuildResult parseMsg(const std::string& msg) override;
};

class KeyValueStore {
public:
	virtual ~KeyValueStore() {}
	virtual void set(const std::string& key, const std::string& value) = 0;
	virtual std::string get(const std::string& key) = 0;
};

class StateSaver {
public:
	StateSaver(KeyValueStore& store, RgbLight& rgbLight);
	void saveCurrentLightSetting();
	void restoreLightSetting();

private:
	uint8_t convert(const std::string& str);

private:
	KeyValueStore& store;
	RgbLight& rgbLight;
};

} // namespace common
