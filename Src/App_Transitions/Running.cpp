#include "Running.hpp"

state* RUNNING::handle(Event e) {
    if (e == Event::EVENT_STOP){
        return &Stop_State;
    }
    if (e == Event::EVENT_RUNNING){
        return &Running_State;
    }
    if (e == Event::EVENT_READY){
        return &Ready_State;
    }
};

std::string RUNNING::name() const {
    return "RUNNING";
};