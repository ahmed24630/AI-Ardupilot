#ifndef READY_HPP
#define READY_HPP

#include "State_Context.hpp"

class READY : public state {
public:
    state* handle(Event e) override;
    std::string name() const override { return "READY"; }
};

extern READY Ready_State;

#endif //READY_HPP