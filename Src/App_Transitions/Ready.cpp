#include "Ready.hpp"

state* READY::handle(Event e) {
    if (e == Event::EVENT_READY){
        return &Ready_State;
    }
    if (e == Event::EVENT_RUNNING){
        return &Running_State;
    }
    if (e == Event::EVENT_STOP){
        return &Stop_State;
    }
};

std::string READY::name() const {
    return "READY";
};