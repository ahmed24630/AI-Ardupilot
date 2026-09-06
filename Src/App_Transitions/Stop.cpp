#include "Stop.hpp"
#include "Running.hpp"
#include "Ready.hpp"

extern READY Ready_State;
extern RUNNING Running_State;
extern STOP Stop_State;

state* STOP::handle(Event e) {
    state* Next_state = &Ready_State;
    if (e == Event::EVENT_STOP){
        Next_state = &Stop_State;
    }
    if (e == Event::EVENT_RUNNING){
        Next_state = &Running_State;
    }
    if (e == Event::EVENT_READY){
        Next_state = &Ready_State;
    }
    return Next_state;
};

std::string STOP::name() const {
    return "STOP";
};