#pragma once

#include <functional>
#include <cstdint>
#include <string>

// Return pair {startCb, stopCb}
std::pair<std::function<bool()>, std::function<bool()>> makeProxyAdapter(
  const std::string& listenIp, uint16_t listenPort,
  const std::string& authIp, uint16_t authPort,
  const std::string& masterIp, uint16_t masterPort);

std::pair<std::function<bool()>, std::function<bool()>> makeStorageAdapter(
  uint16_t storagePort, const std::string& masterIp, uint16_t masterPort,
  const std::string& nodeId, const std::string& diskPath);

std::pair<std::function<bool()>, std::function<bool()>> makeIntermediaryAdapter(
  const std::string& masterIp, int listenPort,  int masterPort);

std::pair<std::function<bool()>, std::function<bool()>> makeAuthAdapter(
  const std::string& listenIp, uint16_t listenPort);

std::pair<std::function<bool()>, std::function<bool()>> makeArduinoAdapter(
  const std::string& masterIp, int masterPort, const std::string& serialPath, const std::string& format);

std::pair<std::function<bool()>, std::function<bool()>> makeMasterAdapter(
  const std::string& bindIp, uint16_t bindPort,
  const std::string& storageIp, uint16_t storagePort,
  const std::string& eventsIp, uint16_t eventsPort,
  const std::string& proxyIp, uint16_t proxyPort);

std::pair<std::function<bool()>, std::function<bool()>> makeEventsAdapter(
  const std::string& listenIp, uint16_t listenPort, const std::string& outPath);
