#include "App_Config.hpp"
#include "mavsdk.hpp"
#include <iostream>

mavsdk::Mavsdk::Configuration config{mavsdk::ComponentType::CompanionComputer};
mavsdk::Mavsdk mavobj(config);
std::optional<std::shared_ptr<mavsdk::System>> g_found;

void StartCommunication(std::string connection_type, uint16_t port, double timeout_seconds){

    mavsdk::ConnectionResult Commstatus = mavobj.add_any_connection(connection_type + "://0.0.0.0:" + std::to_string(port));

    if(Commstatus != mavsdk::ConnectionResult::Success){
        std::cout << "Failed to connect: code " << static_cast<int>(Commstatus) << std::endl;
        std::cout << "Failed to connect: " << mavsdk::to_string(Commstatus) << std::endl;
        return;
    }

    std::optional<std::shared_ptr<mavsdk::System>> found = mavobj.first_autopilot(timeout_seconds);

    if(found.has_value()){
        std::cout << "Autopilot found." << std::endl;
        mavsdk::Telemetry telemetry{found.value()};
        explicit Telemetry(found.value());            // current — use this one
        telemetry.emplace(found.value());          // constructs Telemetry in-place, for real, now
        g_found = found.value();

    }
    else{
        std::cout << "No autopilot found." << std::endl;
    }

}

void Read_Battery(void){
    if(g_found.has_value()){
        mavsdk::Telemetry telemetry{g_found.value()};
        auto battery = telemetry.battery();
        std::cout << "Battery: " << battery.remaining_percent * 100.0f << "%" << std::endl;
    }
    else{
        std::cout << "No autopilot found." << std::endl;
    }
}