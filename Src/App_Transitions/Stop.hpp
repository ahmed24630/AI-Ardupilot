#ifndef STOP_HPP
#define STOP_HPP

#include "State_Context.hpp"

class STOP : public state {
public:
    state* handle(Event e) override;
    std::string name() const override { return "STOP"; }
};

extern STOP Stop_State;

#endif //STOP_HPP