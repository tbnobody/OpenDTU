#ifndef UNITY_INCLUDE_PRINT_FORMATTED
#define UNITY_INCLUDE_PRINT_FORMATTED
#endif
#include "Mocks.hpp"
#include "unity.h"

#define MAX_NUM_INVERTERS 1

#include "PowercontrolPlugin.h"

void setUp(void) {}

void tearDown(void) {
  // clean stuff up here
}

SystemMock testSystem;

void test_PowercontrolLimit(void) {
  TEST_MESSAGE("test_PowercontrolLimit");

auto pmptr = testSystem.createPowerMessage(800);
  auto mmptr = testSystem.createMeterMessage(400);

  DynamicJsonDocument doc(1024);
  JsonObject config = doc.createNestedObject("config");
  config["enabled"] = true;
  config["meter_serial"] = mmptr->serial;
  config["inverter_serial"] = pmptr->deviceId;

  auto powerControl =
      testSystem.createPlugin<PowercontrolPlugin>(PowercontrolPlugin(), config);

  LimitControlMessage *lmc = nullptr;
  testSystem.cb = [&lmc](const std::shared_ptr<PluginMessage> m) {
    if (m.get()->isMessageType<LimitControlMessage>())
      lmc = (LimitControlMessage *)m.get();
  };

  powerControl->internalCallback(pmptr);
  powerControl->internalCallback(mmptr);
  powerControl->loop();


  TEST_ASSERT_NOT_NULL(lmc);
}
void test_PowercontrolLimitUnknownSender(void) {
  TEST_MESSAGE("test_PowercontrolLimitUnknownSender");

  auto pmptr = testSystem.createPowerMessage(800);
  auto mmptr = testSystem.createMeterMessage(400);

  DynamicJsonDocument doc(1024);
  JsonObject config = doc.createNestedObject("config");
  config["enabled"] = true;
  config["meter_serial"] = mmptr->serial;
  config["inverter_serial"] = pmptr->deviceId;

  auto powerControl =
      testSystem.createPlugin<PowercontrolPlugin>(PowercontrolPlugin(), config);

  LimitControlMessage *lmc = nullptr;
  testSystem.cb = [&lmc](const std::shared_ptr<PluginMessage> m) {
    if (m.get()->isMessageType<LimitControlMessage>())
      lmc = (LimitControlMessage *)m.get();
  };
  mmptr->serial = "unknown";
  powerControl->internalCallback(pmptr);
  powerControl->internalCallback(mmptr);
  powerControl->loop();

  TEST_ASSERT_NULL(lmc);
}

void test_function_should_doAlsoDoBlah(void) {
  // more test stuff
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_PowercontrolLimit);
  RUN_TEST(test_PowercontrolLimitUnknownSender);
  return UNITY_END();
}

/**
 * For Arduino framework
 */
void setup() {
  // Wait ~2 seconds before the Unity test runner
  // establishes connection with a board Serial interface
  delay(2000);

  runUnityTests();
}
void loop() {}
