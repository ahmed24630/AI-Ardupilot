#include "Running.hpp"

class RUNNING : public state {
    public:
        virtual state* handle(Event e){
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
        virtual std::string name() const {return "RUNNING";};

};