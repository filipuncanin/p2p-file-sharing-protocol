#include "FSM.h"

FSM::FSM() : currentState(State::SERVER) {
    onCreate();
}

void FSM::onCreate() {
    std::cout << "FSM created. Initial state: SERVER" << std::endl;
}

void FSM::setState(State newState) {
    currentState = newState;
    std::cout << "State set to ";
}

void FSM::getState() {
    switch (currentState) {
    case State::SERVER:
        std::cout << "SERVER";
        break;
    case State::STATE2:
        std::cout << "STATE2";
        break;
    case State::STATE3:
        std::cout << "STATE3";
        break;
    }
    std::cout << std::endl;
}

