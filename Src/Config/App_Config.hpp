#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP
// struct AppConfig {
//     std::string board_type;
//     HostingMode hosting_mode;                    // new field
//     std::vector<DriverConfig> drivers;
//     ModelConfig model;
// };

extern void check_Mavsdk_Config(std::string connection_type, double port, double timeout_seconds);

#endif // APP_CONFIG_HPP