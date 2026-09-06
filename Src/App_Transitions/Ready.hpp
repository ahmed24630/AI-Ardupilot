#ifndef READY_HPP
#define READY_HPP

class READY : public state {
public:
    state* handle(Event e) override;
    std::string name() const override;
};


#endif //READY_HPP