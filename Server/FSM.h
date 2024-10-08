#ifndef FSM_H
#define FSM_H

#include <iostream>

enum class State {
    SERVER,
    STATE2,
    STATE3
};

class FSM {
private:
    State currentState;

public:
    FSM();
    void onCreate();
    void setState(State newState);
    void getState();
};

#endif // FSM_H
