#include "FSM.h"

FSM::FSM() : currentState(State::PEER_IDLE) {
    onCreate();
}

void FSM::onCreate() {
    std::cout << "FSM created. Initial state: PEER_IDLE" << std::endl;
}

void FSM::setState(State newState) {
    currentState = newState;
    //getState();
}

void FSM::getState() {
    switch (currentState) {
    case State::PEER_IDLE:
        std::cout << "\nFSM State: PEER_IDLE\n";
        break;
    case State::PEER_CONNECTED:
        std::cout << "\nFSM State: PEER_CONNECTED\n";
        break;
    case State::PEER_SERVER_TRANSFER:
        std::cout << "\nFSM State: PEER_SERVER_TRANSFER\n";
        break;
    case State::PEER_2_PEER_TRANSFER:
        std::cout << "\nFSM State: PEER_2_PEER_TRANSFER\n";
        break;
    }

    std::cout << std::endl;
}

