#include <WatchDogDtu.h>

//3 seconds WDT
#define WDT_TIMEOUT 30
int last = millis();

WatchDogDtuClass WatchDogDtu;

void WatchDogDtuClass::init(){
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

}


void WatchDogDtuClass::loop() {
  // resetting WDT every 2s, 5 times only
  if (millis() - last >= 1000) {
      esp_task_wdt_reset();
      last = millis();
  }
}