#include "pch.h"
#include "ClosureGraphSetup.h"

namespace ClosureGraphSetup {

    void buildGraph(int testId,
        const std::vector<std::pair<char, char>>& charA,
        const std::vector<std::pair<char, char>>& charB,
        std::vector<std::shared_ptr<NFAState>>& states,
        std::unordered_set<ActiveState>& input) {

        states.clear();
        input.clear();
        Match emptyMatch;

        if (testId == 1) {
            states.push_back(std::make_shared<NFAState>());
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 2) {
            states.push_back(std::make_shared<NFAState>());
            states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<CharTrans>(states[1], states[0], charA));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 3) {
            states.push_back(std::make_shared<NFAState>());
            states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 4) {
            for (int i = 0; i < 5; ++i) states.push_back(std::make_shared<NFAState>());
            for (int i = 0; i < 4; ++i) states[i]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[i + 1], states[i]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 5 || testId == 6) {
            for (int i = 0; i < 4; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<CharTrans>(states[2], states[1], charA));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[2]));
            if (testId == 5) input.insert(ActiveState{ states[0], emptyMatch });
            else input.insert(ActiveState{ states[2], emptyMatch });
            return;
        }
        if (testId == 7) {
            for (int i = 0; i < 3; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[0]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 8) {
            for (int i = 0; i < 5; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[1]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[4], states[1]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 9) {
            for (int i = 0; i < 4; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[1]));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[2]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 10) {
            states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[0]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 11) {
            states.push_back(std::make_shared<NFAState>());
            states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[1]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 12) {
            for (int i = 0; i < 4; ++i) states.push_back(std::make_shared<NFAState>());
            for (int i = 0; i < 3; ++i) states[i]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[i + 1], states[i]));
            states[3]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[3]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 13) {
            for (int i = 0; i < 3; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[1]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[1]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 14) {
            for (int i = 0; i < 3; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[1]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[0]));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[2]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 15) {
            for (int i = 0; i < 5; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[2]->outgoingTransitions.push_back(std::make_shared<CharTrans>(states[3], states[2], charA));
            states[3]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[4], states[3]));
            input.insert(ActiveState{ states[0], emptyMatch });
            input.insert(ActiveState{ states[3], emptyMatch });
            return;
        }
        if (testId == 16) {
            for (int i = 0; i < 3; ++i) states.push_back(std::make_shared<NFAState>());
            for (int i = 0; i < 2; ++i) states[i]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[i + 1], states[i]));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[2]));
            input.insert(ActiveState{ states[1], emptyMatch });
            input.insert(ActiveState{ states[2], emptyMatch });
            return;
        }
        if (testId == 17) {
            for (int i = 0; i < 3; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[1]));
            input.insert(ActiveState{ states[0], emptyMatch });
            input.insert(ActiveState{ states[1], emptyMatch });
            input.insert(ActiveState{ states[2], emptyMatch });
            return;
        }
        if (testId == 18) {
            states.push_back(std::make_shared<NFAState>());
            states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            return;
        }
        if (testId == 19 || testId == 20) {
            for (int i = 0; i < 4; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<CharTrans>(states[2], states[1], charA));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[2]));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[2]));
            if (testId == 19) input.insert(ActiveState{ states[0], emptyMatch });
            else input.insert(ActiveState{ states[2], emptyMatch });
            return;
        }
        if (testId == 21 || testId == 22) {
            for (int i = 0; i < 6; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<CharTrans>(states[2], states[1], charA));
            states[3]->outgoingTransitions.push_back(std::make_shared<CharTrans>(states[4], states[3], charB));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[5], states[2]));
            states[4]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[5], states[4]));
            if (testId == 21) input.insert(ActiveState{ states[0], emptyMatch });
            else {
                input.insert(ActiveState{ states[2], emptyMatch });
                input.insert(ActiveState{ states[4], emptyMatch });
            }
            return;
        }
        if (testId == 23) {
            for (int i = 0; i < 4; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[3], states[0]));
            states[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[1]));
            states[2]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[2]));
            states[3]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[0], states[3]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 24) {
            int count = 10000;
            for (int i = 0; i < count; ++i) states.push_back(std::make_shared<NFAState>());
            for (int i = 0; i < count - 1; ++i) states[i]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[i + 1], states[i]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
        if (testId == 25) {
            for (int i = 0; i < 3; ++i) states.push_back(std::make_shared<NFAState>());
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[1], states[0]));
            states[0]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(states[2], states[0]));
            input.insert(ActiveState{ states[0], emptyMatch });
            return;
        }
    }
}
