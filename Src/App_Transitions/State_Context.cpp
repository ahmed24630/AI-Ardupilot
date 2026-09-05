#include "State_Context.hpp"


class context {
    state* current;
    void handle_event(Event e){
        current = current->handle(e);
    }
};

std::atomic<bool> g_engine_running(true);

void state_machine_engine() {
    context ctx; // The State Machine lives entirely inside this function!
    while (true) {
        // Thread safely freezes right here, consuming 0% CPU power until nudged
        Event requested_event = EVENT_RUNNING;//g_mailbox.pop(g_engine_running);
        
        ctx.handle_event(requested_event);
    }