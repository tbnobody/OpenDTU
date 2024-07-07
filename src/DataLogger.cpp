#include "DataLogger.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "SD.h"

DataLoggerClass DataLogger;

DataLoggerClass::DataLoggerClass()
    : _loopTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&DataLoggerClass::loop, this))
{
}

void DataLoggerClass::init(Scheduler& scheduler) {
    scheduler.addTask(_loopTask);
    _loopTask.enable();

    const CONFIG_T& config = Configuration.get();
    setSaveInterval(config.DataLogger.SaveInterval);

    if(strlen(config.DataLogger.FileName) > 0) {
      File file = SD.open("/" + String(config.DataLogger.FileName));
      if(!file) {
          MessageOutput.println("Datalogger: File doesn't exist");
          MessageOutput.println("Datalogger: Creating file...");
          writeFile(SD, "/" + String(config.DataLogger.FileName), "timestamp; totalPower; totalYieldDay; totalYieldTotal \r\n");
      }
      else {
          MessageOutput.println("Datalogger: File already exists");  
      }
      file.close();
    } else {
      MessageOutput.println("Datalogger: Filename is empty");  
    }
    
}

void DataLoggerClass::loop()
{
  if (millis() - _lastSave > (_saveInterval * 1000)) {
    logToSDCard();

    _lastSave = millis();
  }
}

void DataLoggerClass::logToSDCard() {
    const CONFIG_T& config = Configuration.get();

    String dataMessage = String(getTime()) + ";" + String(config.DataLogger.OutputConfig.TotalPower ? Datastore.getTotalAcPowerEnabled() : 0) + ";" + String(config.DataLogger.OutputConfig.TotalYieldDay ? Datastore.getTotalAcYieldDayEnabled() : 0) + ";" + String(config.DataLogger.OutputConfig.TotalYieldTotal ? Datastore.getTotalAcYieldTotalEnabled() : 0) + " \r\n";
    MessageOutput.printf("DataLogger: %s saved. \n", dataMessage.c_str());
    appendFile(SD, "/" + String(config.DataLogger.FileName), dataMessage.c_str());
}

void DataLoggerClass::setSaveInterval(const uint32_t interval)
{
    _saveInterval = interval;
}

void DataLoggerClass::writeFile(fs::FS &fs, const String path, const char * message) {
  MessageOutput.printf("DataLogger: Writing file: %s\n", path.c_str());

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    MessageOutput.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    MessageOutput.println("DataLogger: File written successfully");
  } else {
    MessageOutput.println("DataLogger: Write failed");
  }
  file.close();
}

void DataLoggerClass::appendFile(fs::FS &fs, const String path, const char * message) {
  MessageOutput.printf("DataLogger: Appending to file: %s\n", path.c_str());

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    MessageOutput.println("DataLogger: Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    MessageOutput.println("DataLogger: Data appended");
  } else {
    MessageOutput.println("DataLogger: Append failed");
  }
  file.close();
}

uint64_t DataLoggerClass::getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    MessageOutput.println("DataLogger: Failed to fetch current time");
    return(0);
  }
  time(&now);
  return now;
}
