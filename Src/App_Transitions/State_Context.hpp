#ifndef STATE_CONTEXT_HPP
#define STATE_CONTEXT_HPP

#include <iostream>
#include "Ready.hpp"
#include "Running.hpp"
#include "Stop.hpp"

enum class Event {
    EVENT_READY,
    EVENT_RUNNING,
    EVENT_STOP
};

class state {
    public:
        virtual state* handle(Event e)=0;
        virtual ~state(){};
        virtual std::string name() const = 0;

};

extern READY Ready_State;
extern RUNNING Running_State;
extern STOP Stop_State;

extern void state_machine_engine(void);


#endif // State_Context