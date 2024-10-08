#ifndef FSM_H
#define FSM_H

#include <iostream>

enum class State {
    PEER_IDLE,
    PEER_CONNECTED,
    PEER_SERVER_TRANSFER,
    PEER_2_PEER_TRANSFER
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
