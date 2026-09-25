// Placeholder — this is just your hello-world exercise, standing in here so
// you can verify the whole CMake + toolchain setup builds correctly before
// any real project code exists. Replace this as you work through the
// milestones.
#include <iostream>
#include <thread> // Required header
#include "State_Context.hpp"
#include "AppConfig.hpp"


CommDriverConfig driver_test_config = {
    .transport = TransportType::SERIAL,
    .device_path = "/dev/ttyUSB0",
    .baud_rate = 57600,
    .host = "127.0.0.1",
    .port = 14550,
    .system_id = 1,
    .component_id = 1,
    .target_system_id = 1,
    .target_component_id = 1,
    .protocol_version = MavlinkVersion::V2,
    .use_signing = false,
    .signing_key = std::nullopt,
    .own_heartbeat_rate_hz = 1.0,
    .heartbeat_timeout_ms = 3000,
    .receive_buffer_size = 1024,
    .max_reassembly_attempts = 3,
    .connect_timeout_ms = 5000,
    .max_retries = 3
};

int main() {

    check_Mavsdk_Config();

    std::thread App_thread(state_machine_engine);

    App_thread.join();
    return 0;
}
