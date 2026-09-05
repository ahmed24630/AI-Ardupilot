#ifndef RUNNING_HPP
#define RUNNING_HPP

#include <iostream>
#include "State_Context.hpp"


class RUNNING : public state {
public:
    state* handle(Event e) override;
    std::string name() const override { return "RUNNING"; }
};

extern RUNNING Running_State;

#endif //RUNNING_HPP