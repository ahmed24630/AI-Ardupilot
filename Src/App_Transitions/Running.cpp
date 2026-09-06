#include "Running.hpp"


state* RUNNING::handle(Event e) {
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

std::string RUNNING::name() const {
    return "RUNNING";
};