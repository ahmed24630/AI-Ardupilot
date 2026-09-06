#include "Ready.hpp"


state* READY::handle(Event e) {
    state* Next_state = Ready_State;
    if (e == Event::EVENT_READY){
        Next_state = Ready_State;
    }
    if (e == Event::EVENT_RUNNING){
        Next_state = Running_State;
    }
    if (e == Event::EVENT_STOP){
        Next_state = Stop_State;
    }
    return Next_state;
};

std::string READY::name() const {
    return "READY";
};