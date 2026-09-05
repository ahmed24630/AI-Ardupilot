#include <iostream>
#include "State_Context.hpp"
#include "Ready.hpp"
#include "Stop.hpp"


class RUNNING : public state {
public:
    state* handle(Event e) override;
    std::string name() const override { return "RUNNING"; }
};

extern RUNNING Running_State;