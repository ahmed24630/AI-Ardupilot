#ifndef RUNNING_HPP
#define RUNNING_HPP

#include "State_Context.hpp"


class RUNNING : public state {
public:
    state* handle(Event e) override;
    std::string name() const override;
};

#endif //RUNNING_HPP