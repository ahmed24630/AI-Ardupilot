#include "State_Context.hpp"
#include <atomic>
#include "Stop.hpp"
#include "Running.hpp"
#include "Ready.hpp"

READY Ready_State;
RUNNING Running_State;
STOP Stop_State;

std::atomic<bool> g_engine_running(true);
class context {
    public:
        context() : current(&Ready_State) {}   // pick whichever state should be "first"
        state* current;
        void handle_event(Event e){current = current->handle(e);}
};

void state_machine_engine() {
    context ctx; // The State Machine lives entirely inside this function!
    // Thread safely freezes right here, consuming 0% CPU power until nudged
    Event requested_event = Event::EVENT_STOP;//g_mailbox.pop(g_engine_running);

    int input;
    while (true) {
        std::cin >> input;
        Event requested_event = static_cast<Event>(input);

        ctx.handle_event(requested_event);

        std::cout << "Current state: " << ctx.current->name() << std::endl;
    }
}