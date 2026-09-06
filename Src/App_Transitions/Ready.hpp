#ifndef READY_HPP
#define READY_HPP

#include "State_Context.hpp"

class READY : public state {
public:
    state* handle(Event e) override;
    std::string name() const override;
};


#endif //READY_HPP