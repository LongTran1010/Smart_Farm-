#ifndef TASK_BASE_H
#define TASK_BASE_H

#include <Arduino.h>

//stackSize = 4 bytes

class TaskBase {
public:
  TaskBase(const char* name, uint32_t stackSize, UBaseType_t priority, BaseType_t core)
    : taskName(name), stackSize(stackSize), priority(priority), core(core), taskHandle(nullptr) {}

  virtual ~TaskBase() {
    stop();
  }
  void start() {
    if(taskHandle == nullptr){
    xTaskCreatePinnedToCore(taskRunner, taskName, stackSize, this, priority, &taskHandle, core);
    }
  }
  void stop() {
    if (taskHandle != nullptr) {
      Serial.printf("Stopping task: %s\n", taskName);
      vTaskDelete(taskHandle);
      taskHandle = nullptr;
    }
  }

protected:
  virtual void run() = 0;  // Override trong class con

private:
  const char* taskName;
  uint32_t stackSize;
  UBaseType_t priority;
  TaskHandle_t taskHandle = nullptr;
  BaseType_t core;

  static void taskRunner(void* pvParameters) {
    TaskBase* task = static_cast<TaskBase*>(pvParameters);
    if(task != nullptr){
      task->run();  // Gọi hàm run() của class con
    }
  }
};

#endif
