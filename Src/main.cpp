// Placeholder — this is just your hello-world exercise, standing in here so
// you can verify the whole CMake + toolchain setup builds correctly before
// any real project code exists. Replace this as you work through the
// milestones.
#include <iostream>
#include <thread> // Required header
#include "State_Context.hpp"

int main() {

    std::thread App_thread(state_machine_engine);
    return 0;
}
