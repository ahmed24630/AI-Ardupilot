#include <iostream>

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