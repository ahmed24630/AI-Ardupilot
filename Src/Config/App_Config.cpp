#include "App_Config.hpp"
#include "telemetry.hpp"
#include "mavsdk.hpp"
#include <iostream>

mavsdk::Mavsdk::Configuration config{mavsdk::ComponentType::CompanionComputer};
mavsdk::Mavsdk mavobj(config);
std::optional<mavsdk::Telemetry> g_telemetry;   // file scope, alongside mavobj and g_found

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
        g_telemetry.emplace(found.value());
    }
    else{
        std::cout << "No autopilot found." << std::endl;
    }

}

void Read_Battery(void){
    if(g_telemetry.has_value()){
        auto battery = g_telemetry.value().battery();
        std::cout << "Battery: " << battery.remaining_percent * 100.0f << "%" << std::endl;
    }
    else{
        std::cout << "No autopilot found." << std::endl;
    }
}