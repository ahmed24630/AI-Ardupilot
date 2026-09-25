#include <mavsdk/mavsdk.hpp>
#include <iostream>
#include "App_Config.hpp"

mavsdk::Mavsdk::Configuration config{mavsdk::ComponentType::CompanionComputer};
mavsdk::Mavsdk mavobj(config);

void check_Mavsdk_Config(){

    mavsdk::ConnectionResult Commstatus = mavobj.add_any_connection("udp://:14540");

    if(Commstatus != mavsdk::ConnectionResult::Success){
        std::cout << "Failed to connect: " << std::endl;
        return;
    }

    std::optional<std::shared_ptr<mavsdk::System>> found = mavobj.first_autopilot(10.0);

    if(found.has_value()){
        std::cout << "Autopilot found." << std::endl;
    }
    else{
        std::cout << "No autopilot found." << std::endl;
    }
}