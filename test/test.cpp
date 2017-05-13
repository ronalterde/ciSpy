#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "common.h"
#include "pwm.h"
#include "filesystem.h"
#include "strings.h"

#include <sstream>
#include <unordered_map>

/**
 * TEST LIST
 *
 * - red led sometimes blinks
 * - test LinuxPwmOutput via (real) files
 */

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Args;
using ::testing::SaveArg;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

using namespace std;
using namespace common;

namespace {

class TestRgbLight : public RgbLight {
public:
	void set(LightSetting value) override {
		setting = value;
	}

	LightSetting get() override {
		return setting;
	}

private:
	LightSetting setting;
};

class TestBeeper : public Beeper {
public:
	void setVolume(uint8_t volume) override {
		(void)volume;
	}

	uint8_t getVolume() override {
		return 0;
	}

	void playTone(const BeeperTone& tone) override {
		lastPlayed = tone;
	}

	BeeperTone lastPlayed;
};

class LightSettingTest : public testing::Test {
};

TEST_F(LightSettingTest, comparison) {
	LightSetting setting0{0, 255, 0};
	LightSetting setting1{0, 255, 0};
	LightSetting setting2{5, 6, 7};
	ASSERT_EQ(setting0, setting1);
	ASSERT_NE(setting0, setting2);
}

class BeeperToneTest : public testing::Test {
};

TEST_F(BeeperToneTest, construction) {
	BeeperTone tone{1000, 1};
}

TEST_F(BeeperToneTest, comparison) {
	BeeperTone tone0;
	BeeperTone tone1;
	tone1.duration_ms = 5;
	tone1.frequency_hz = 8;
	ASSERT_EQ(tone0, tone0);
	ASSERT_NE(tone0, tone1);

	tone1 = tone0;
	ASSERT_EQ(tone0, tone1);
}

class SignalizerTest : public testing::Test {
protected:
	TestBeeper beeper;
	TestRgbLight rgbLight;
	Signalizer s{ beeper, rgbLight };
};

TEST_F(SignalizerTest, lightTurnsGreenIfBuildOk) {
	s.update(BuildResult::OK);
	LightSetting l{0, 255, 0};
	ASSERT_EQ(rgbLight.get(), l);
}

TEST_F(SignalizerTest, noBeepIfBuildOk) {
	s.update(BuildResult::OK);
	BeeperTone noTone;
	ASSERT_EQ(beeper.lastPlayed, noTone);
}

TEST_F(SignalizerTest, lightTurnsRedIfBuildBroken) {
	s.update(BuildResult::BROKEN);
	LightSetting l{255, 0, 0};
	ASSERT_EQ(rgbLight.get(), l);
}

TEST_F(SignalizerTest, errorBeepIfBuildBroken) {
	s.update(BuildResult::BROKEN);
	BeeperTone errorTone;
	errorTone.duration_ms = 1000;
	errorTone.frequency_hz = 1000;
	ASSERT_EQ(beeper.lastPlayed, errorTone);
}

class JenkinsBuildResultParserTest : public ::testing::Test {
};

TEST_F(JenkinsBuildResultParserTest, canBeAssignedToInterface) {
	JenkinsBuildResultParser parser;
	BuildResultParser& interface = parser;
	(void)interface;
}

TEST_F(JenkinsBuildResultParserTest, success) {
	JenkinsBuildResultParser parser;
	auto successMsg =
		"<job><name>Foo</name><build>\
			<phase>COMPLETED</phase>\
			<status>SUCCESS</status></build></job>";
	auto result = parser.parseMsg(successMsg);
	ASSERT_EQ(result, BuildResult::OK);
}

TEST_F(JenkinsBuildResultParserTest, fail) {
	JenkinsBuildResultParser parser;
	auto failMsg = "anything";
	auto result = parser.parseMsg(failMsg);
	ASSERT_EQ(result, BuildResult::BROKEN);
}

class TestKeyValueStore : public KeyValueStore {
public:
	void set(const std::string& key, const std::string& value) override {
		map[key] = value;
	}

	std::string get(const std::string& key) override {
		return map[key];
	}

public:
	unordered_map<string, string> map;
};

class StateSaverTest : public testing::Test {
public:
	void SetUp() override {
		ASSERT_EQ(store.get(strings::LED_R), "");
		ASSERT_EQ(store.get(strings::LED_G), "");
		ASSERT_EQ(store.get(strings::LED_B), "");
	}

protected:
	TestRgbLight rgbLight;
	TestKeyValueStore store;
	StateSaver stateSaver{ store, rgbLight };
};

TEST_F(StateSaverTest, savesCurrentLightSetting) {
	rgbLight.set(LightSetting{1, 2, 3});
	stateSaver.saveCurrentLightSetting();
	ASSERT_EQ(store.get(strings::LED_R), "1");
	ASSERT_EQ(store.get(strings::LED_G), "2");
	ASSERT_EQ(store.get(strings::LED_B), "3");

	rgbLight.set(LightSetting{253, 254, 255});
	stateSaver.saveCurrentLightSetting();
	ASSERT_EQ(store.get(strings::LED_R), "253");
	ASSERT_EQ(store.get(strings::LED_G), "254");
	ASSERT_EQ(store.get(strings::LED_B), "255");
}

TEST_F(StateSaverTest, restore) {
	store.set(strings::LED_R, "100");
	store.set(strings::LED_G, "100");
	store.set(strings::LED_B, "100");
	ASSERT_NO_THROW(stateSaver.restoreLightSetting());
	auto lightSetting = rgbLight.get();
	ASSERT_EQ(lightSetting.r, 100);
	ASSERT_EQ(lightSetting.g, 100);
	ASSERT_EQ(lightSetting.b, 100);

	store.set(strings::LED_R, "11");
	store.set(strings::LED_G, "12");
	store.set(strings::LED_B, "13");
	ASSERT_NO_THROW(stateSaver.restoreLightSetting());
	lightSetting = rgbLight.get();
	ASSERT_EQ(lightSetting.r, 11);
	ASSERT_EQ(lightSetting.g, 12);
	ASSERT_EQ(lightSetting.b, 13);
}

TEST_F(StateSaverTest, storageValueAboveMax) {
	store.set(strings::LED_R, "256");
	store.set(strings::LED_G, "1");
	store.set(strings::LED_B, "1");
	ASSERT_ANY_THROW(stateSaver.restoreLightSetting());

	store.set(strings::LED_R, "1");
	store.set(strings::LED_G, "256");
	store.set(strings::LED_B, "1");
	ASSERT_ANY_THROW(stateSaver.restoreLightSetting());

	store.set(strings::LED_R, "1");
	store.set(strings::LED_G, "1");
	store.set(strings::LED_B, "256");
	ASSERT_ANY_THROW(stateSaver.restoreLightSetting());
}

TEST_F(StateSaverTest, storageValueNotANumber) {
	store.set(strings::LED_R, "1");
	store.set(strings::LED_G, "1");
	store.set(strings::LED_B, "ABC");
	ASSERT_ANY_THROW(stateSaver.restoreLightSetting());
}

TEST_F(StateSaverTest, emptyStorageIsReadAsDefault) {
	store.set(strings::LED_R, "");
	store.set(strings::LED_G, "");
	store.set(strings::LED_B, "");

	ASSERT_NO_THROW(stateSaver.restoreLightSetting());

	auto lightSetting = rgbLight.get();
	ASSERT_EQ(lightSetting.r, 0);
	ASSERT_EQ(lightSetting.g, 0);
	ASSERT_EQ(lightSetting.b, 0);
}

class TestPwmOutput : public pwm::PwmOutput {
public:
	MOCK_METHOD1(enable, void(bool));
	MOCK_METHOD1(setPeriodNs, void(unsigned long int));
	MOCK_METHOD1(setDutyCycleNs, void(unsigned long int));
};

class PwmBeeperTest : public ::testing::Test {
protected:
	class SleepMock {
	public:
		MOCK_METHOD1(sleep, void(uint16_t));
	};

protected:
	BeeperTone anyBeeperTone{1, 1};
	NiceMock<SleepMock> sleepMock;
	pwm::PwmBeeper::SleepFunction sleepFunction{[this](uint16_t duration_ms) {
		return sleepMock.sleep(duration_ms);
	}};
	NiceMock<TestPwmOutput> pwmOutput;
	pwm::PwmBeeper beeper{ pwmOutput, sleepFunction };
};

TEST_F(PwmBeeperTest, baseClass) {
	Beeper& baseClass = beeper;
	(void)baseClass;
}

TEST_F(PwmBeeperTest, appliesDefaultsOnConstruction) {
	EXPECT_CALL(pwmOutput, setPeriodNs(0));
	EXPECT_CALL(pwmOutput, setDutyCycleNs(0));
	EXPECT_CALL(pwmOutput, enable(false));
	pwm::PwmBeeper beeper{ pwmOutput, sleepFunction };
}

TEST_F(PwmBeeperTest, volumeInPercent) {
	ASSERT_EQ(beeper.getVolume(), 50);

	beeper.setVolume(100);
	ASSERT_EQ(beeper.getVolume(), 100);

	beeper.setVolume(101);
	ASSERT_EQ(beeper.getVolume(), 100);
}

TEST_F(PwmBeeperTest, configurationHappensInTheRightOrder) {
	::testing::InSequence inSeq;
	EXPECT_CALL(pwmOutput, setPeriodNs(_));
	EXPECT_CALL(pwmOutput, setDutyCycleNs(_));
	EXPECT_CALL(pwmOutput, enable(true));
	EXPECT_CALL(pwmOutput, enable(false));

	beeper.playTone(anyBeeperTone);
}

TEST_F(PwmBeeperTest, sleepsAccordingToDuration) {
	BeeperTone tone;

	tone.duration_ms = 0;
	EXPECT_CALL(sleepMock, sleep(tone.duration_ms));
	beeper.playTone(tone);

	tone.duration_ms = 60000;
	EXPECT_CALL(sleepMock, sleep(tone.duration_ms));
	beeper.playTone(tone);
}

TEST_F(PwmBeeperTest, configuresPwmPeriodAccordingToFrequency) {
	BeeperTone tone;

	tone.frequency_hz = 0;
	EXPECT_CALL(pwmOutput, setPeriodNs(0));
	beeper.playTone(tone);

	tone.frequency_hz = 1;
	EXPECT_CALL(pwmOutput, setPeriodNs(GIGA / tone.frequency_hz));
	beeper.playTone(tone);

	tone.frequency_hz = 60000;
	EXPECT_CALL(pwmOutput, setPeriodNs(GIGA / tone.frequency_hz));
	beeper.playTone(tone);
}

TEST_F(PwmBeeperTest, updatesDutyCycleTogetherWithFrequencyDependingOnVolume) {
	BeeperTone tone;

	auto volume = beeper.getVolume();
	tone.frequency_hz = 10;
	EXPECT_CALL(pwmOutput, setPeriodNs(GIGA / tone.frequency_hz));
	EXPECT_CALL(pwmOutput, setDutyCycleNs(GIGA / tone.frequency_hz * volume / 100));
	beeper.playTone(tone);

	tone.frequency_hz = 10;
	beeper.setVolume(90);
	EXPECT_CALL(pwmOutput, setPeriodNs(GIGA / tone.frequency_hz));
	EXPECT_CALL(pwmOutput, setDutyCycleNs(GIGA / tone.frequency_hz * 90 / 100));
	beeper.playTone(tone);
}

class PwmRgbLedTest : public ::testing::Test {
protected:
	void SetUp() override {
		ON_CALL(pwmOutputs[0], setDutyCycleNs(_))
			.WillByDefault(SaveArg<0>(&lastDutyCycle[0]));
		ON_CALL(pwmOutputs[1], setDutyCycleNs(_))
			.WillByDefault(SaveArg<0>(&lastDutyCycle[1]));
		ON_CALL(pwmOutputs[2], setDutyCycleNs(_))
			.WillByDefault(SaveArg<0>(&lastDutyCycle[2]));
	}

	unsigned long lastDutyCycle[3];
	LightSetting anySetting{5, 5, 5};
	array<NiceMock<TestPwmOutput>, 3> pwmOutputs;
	pwm::PwmRgbLed led{pwmOutputs[0], pwmOutputs[1], pwmOutputs[2]};
};

TEST_F(PwmRgbLedTest, baseClass) {
	RgbLight& baseClass = led;
	(void)baseClass;
}

TEST_F(PwmRgbLedTest, appliesDefaultsOnConstruction) {
	EXPECT_CALL(pwmOutputs[0], setPeriodNs(led.getDefaultPeriodNs()));
	EXPECT_CALL(pwmOutputs[0], setDutyCycleNs(0));
	EXPECT_CALL(pwmOutputs[0], enable(false));

	EXPECT_CALL(pwmOutputs[1], setPeriodNs(led.getDefaultPeriodNs()));
	EXPECT_CALL(pwmOutputs[1], setDutyCycleNs(0));
	EXPECT_CALL(pwmOutputs[1], enable(false));

	EXPECT_CALL(pwmOutputs[2], setPeriodNs(led.getDefaultPeriodNs()));
	EXPECT_CALL(pwmOutputs[2], setDutyCycleNs(0));
	EXPECT_CALL(pwmOutputs[2], enable(false));
	pwm::PwmRgbLed led{pwmOutputs[0], pwmOutputs[1], pwmOutputs[2]};
}

TEST_F(PwmRgbLedTest, configurationHappensInTheRightOrder) {
	::testing::InSequence inSeq;
	EXPECT_CALL(pwmOutputs[0], setPeriodNs(_));
	EXPECT_CALL(pwmOutputs[0], setDutyCycleNs(_));
	EXPECT_CALL(pwmOutputs[0], enable(true));

	EXPECT_CALL(pwmOutputs[1], setPeriodNs(_));
	EXPECT_CALL(pwmOutputs[1], setDutyCycleNs(_));
	EXPECT_CALL(pwmOutputs[1], enable(true));

	EXPECT_CALL(pwmOutputs[2], setPeriodNs(_));
	EXPECT_CALL(pwmOutputs[2], setDutyCycleNs(_));
	EXPECT_CALL(pwmOutputs[2], enable(true));

	led.set(anySetting);
}

TEST_F(PwmRgbLedTest, pwmPeriodIsConstant) {
	EXPECT_CALL(pwmOutputs[0], setPeriodNs(led.getDefaultPeriodNs()));
	EXPECT_CALL(pwmOutputs[1], setPeriodNs(led.getDefaultPeriodNs()));
	EXPECT_CALL(pwmOutputs[2], setPeriodNs(led.getDefaultPeriodNs()));
	led.set(anySetting);
}

TEST_F(PwmRgbLedTest, dutyCycleIsSetInRelationToPeriodButLimited) {
	unsigned long delta = led.getDefaultPeriodNs() / 255;

	led.set(LightSetting{255, 127, 63});
	ASSERT_NEAR(lastDutyCycle[0], pwm::PwmRgbLed::DUTY_CYCLE_MAX_RATIO * led.getDefaultPeriodNs(), delta);
	ASSERT_NEAR(lastDutyCycle[1], pwm::PwmRgbLed::DUTY_CYCLE_MAX_RATIO * 0.5 * led.getDefaultPeriodNs(), delta);
	ASSERT_NEAR(lastDutyCycle[2], pwm::PwmRgbLed::DUTY_CYCLE_MAX_RATIO * 0.25 * led.getDefaultPeriodNs(), delta);

	led.set(LightSetting{63, 127, 255});
	ASSERT_NEAR(lastDutyCycle[0], pwm::PwmRgbLed::DUTY_CYCLE_MAX_RATIO * 0.25 * led.getDefaultPeriodNs(), delta);
	ASSERT_NEAR(lastDutyCycle[1], pwm::PwmRgbLed::DUTY_CYCLE_MAX_RATIO * 0.5 * led.getDefaultPeriodNs(), delta);
	ASSERT_NEAR(lastDutyCycle[2], pwm::PwmRgbLed::DUTY_CYCLE_MAX_RATIO * 1 * led.getDefaultPeriodNs(), delta);
}

TEST_F(PwmRgbLedTest, lightSettingIsBuffered) {
	auto in = LightSetting{255, 127, 63};
	led.set(in);
	LightSetting out = led.get();
	ASSERT_EQ(in, out);
}

TEST_F(PwmRgbLedTest, defaultSetting) {
	auto out = led.get();
	auto expected = LightSetting{0, 0, 0};
	ASSERT_EQ(out, expected);
}

class TestStreamFactory : public filesystem::StreamFactory {
public:
	std::unique_ptr<std::istream> makeInputStream(const std::string& path) {
		(void)path;
		auto stream = make_unique<stringstream>();
		*stream << inStringBuf;
		return stream;
	}

	class OutStream : public std::stringstream {
	public:
		OutStream(std::string& out) :
			std::stringstream(),
			out(out) {
		}

		~OutStream() {
			// Save buffer right before destruction by client
			out = this->str();
		}

	private:
		std::string& out;

	};

	std::unique_ptr<std::ostream> makeOutputStream(const std::string& path) {
		(void)path;
		auto stream = make_unique<OutStream>(outStringBuf);
		return stream;
	}

public:
	string inStringBuf;
	string outStringBuf;
};

class FileStoreTest : public ::testing::Test {
protected:
	TestStreamFactory streamFactory;
	filesystem::FileStore store{ streamFactory, "" };
};

TEST_F(FileStoreTest, getValuesFromFile) {
	streamFactory.inStringBuf = "key0:value0\nkey1:value1\nkey2:value2\n";

	ASSERT_EQ(store.get("key0"), "value0");
	ASSERT_EQ(store.get("key1"), "value1");
	ASSERT_EQ(store.get("key2"), "value2");
}

TEST_F(FileStoreTest, setKeysInCompletelyEmptyFile) {
	streamFactory.inStringBuf = "";
	store.set("key0", "foo");

	streamFactory.inStringBuf = "key0:foo\n";
	store.set("key56", "BAZ");

	ASSERT_EQ(streamFactory.outStringBuf, "key0:foo\nkey56:BAZ\n");
}

TEST_F(FileStoreTest, setValueForExistingKey) {
	streamFactory.inStringBuf = "key0:value0\nkey1:value1\nkey2:value2\n";
	store.set("key0", "bar");

	ASSERT_EQ(streamFactory.outStringBuf, "key0:bar\nkey1:value1\nkey2:value2\n");
}

} // namespace
