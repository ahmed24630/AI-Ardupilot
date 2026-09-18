struct AppConfig {
    std::string board_type;
    HostingMode hosting_mode;                    // new field
    std::vector<DriverConfig> drivers;
    ModelConfig model;
};