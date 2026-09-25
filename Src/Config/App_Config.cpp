#include <mavsdk/mavsdk.hpp>
#include <iostream>
#include "App_Config.hpp"

mavsdk::Mavsdk::Configuration config{mavsdk::ComponentType::CompanionComputer};
mavsdk::Mavsdk mavobj(config);

void check_Mavsdk_Config(std::string connection_type, double port, double timeout_seconds){

    mavsdk::ConnectionResult Commstatus = mavobj.add_any_connection(connection_type + "://" + std::to_string(port));

    if(Commstatus != mavsdk::ConnectionResult::Success){
        std::cout << "Failed to connect: " << std::endl;
        return;
    }

    std::optional<std::shared_ptr<mavsdk::System>> found = mavobj.first_autopilot(timeout_seconds);

    if(found.has_value()){
        std::cout << "Autopilot found." << std::endl;
    }
    else{
        std::cout << "No autopilot found." << std::endl;
    }
}