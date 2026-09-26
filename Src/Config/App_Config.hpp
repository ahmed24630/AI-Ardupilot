#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <string>
#include <iostream>
#include <cstdint>
// struct AppConfig {
//     std::string board_type;
//     HostingMode hosting_mode;                    // new field
//     std::vector<DriverConfig> drivers;
//     ModelConfig model;
// };

extern void StartCommunication(std::string connection_type, uint16_t port, double timeout_seconds);
extern void send_HeartBeat(void);
#endif // APP_CONFIG_HPP