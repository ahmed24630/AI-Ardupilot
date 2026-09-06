#include "State_Context.hpp"
#include <atomic>


state* Running_State;
state* Ready_State;
state* Stop_State;
std::atomic<bool> g_engine_running(true);
class context {
    public:
        context() : current(Ready_State) {}   // pick whichever state should be "first"
        state* current;
        void handle_event(Event e){current = current->handle(e);}
};

void state_machine_engine() {
    context ctx; // The State Machine lives entirely inside this function!
    while (true) {
        // Thread safely freezes right here, consuming 0% CPU power until nudged
        Event requested_event = Event::EVENT_RUNNING;//g_mailbox.pop(g_engine_running);
        
        ctx.handle_event(requested_event);
    }
}