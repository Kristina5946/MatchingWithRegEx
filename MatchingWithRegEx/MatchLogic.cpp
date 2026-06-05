#include "pch.h"
#include "MatchLogic.h"
#include <deque>
#include <vector>

void calculateDistanceToTerminal(std::shared_ptr<NFAState> endState)
{
    // 1: Терминальное состояние — расстояние 0
    endState->distanceToTerminal = 0;

    // 2: Очередь для обхода в ширину 0-1 BFS (deque)
    std::deque<std::shared_ptr<NFAState>> queue;
    queue.push_back(endState);

    // 3: Пока очередь не пуста
    bool queueNotEmpty = !queue.empty();
    while (queueNotEmpty) {
        // 4.1: Извлечь текущее состояние u из начала очереди
        std::shared_ptr<NFAState> u = queue.front();
        queue.pop_front();

        // 4.2: Для каждого входящего перехода в состояние u
        size_t inIndex = 0;
        while (inIndex < u->incomingTransitions.size()) {
            std::shared_ptr<Trans> trans = u->incomingTransitions[inIndex].lock();
            if (trans) {
                std::shared_ptr<NFAState> v = trans->getSource();
                if (v) {
                    // 4.2.2: Вес перехода: 0 для eps, 1 для символьного CharTrans
                    size_t stepWeight = 0;
                    if (!trans->isEpsilon()) {
                        stepWeight = 1;
                    }
                    size_t newDistance = u->distanceToTerminal + stepWeight;

                    // 4.2.3: Если путь короче — обновить расстояние и добавить v в очередь
                    if (newDistance < v->distanceToTerminal) {
                        v->distanceToTerminal = newDistance;
                        if (stepWeight == 0) {
                            queue.push_front(v);
                        }
                        else {
                            queue.push_back(v);
                        }
                    }
                }
            }
            inIndex++;
        }

        queueNotEmpty = !queue.empty();
    }
}

std::unordered_set<std::shared_ptr<NFAState>> SimulateNFA(
    const std::unordered_set<std::shared_ptr<NFAState>>& currentStates, char c)
{
    // 1: Создать пустое множество следующих состояний
    std::unordered_set<std::shared_ptr<NFAState>> nextStates;

    std::string inputChar;
    inputChar.push_back(c);

    // 2: Для каждого текущего состояния проверить исходящие переходы
    for (const std::shared_ptr<NFAState>& state : currentStates) {
        size_t transIndex = 0;
        while (transIndex < state->outgoingTransitions.size()) {
            std::shared_ptr<Trans> trans = state->outgoingTransitions[transIndex];

            // 3: Символьный переход применим к c — добавить целевое состояние
            if (!trans->isEpsilon()) {
                bool applicable = trans->isApplicable(inputChar, 0);
                // 3.1: Маска ANY не принимает перенос строки (спецификация маски ".")
                if (applicable && c == '\n' && trans->isApplicable("a", 0)) {
                    applicable = false;
                }
                if (applicable) {
                    std::shared_ptr<NFAState> target = trans->getTarget();
                    if (target) {
                        nextStates.insert(target);
                    }
                }
            }
            transIndex++;
        }
    }

    // 4: eps-замыкание — стек обхода, добавить состояния по пустым переходам
    std::vector<std::shared_ptr<NFAState>> stack;
    for (const std::shared_ptr<NFAState>& state : nextStates) {
        stack.push_back(state);
    }

    size_t stackIndex = 0;
    bool stackNotEmpty = stackIndex < stack.size();
    while (stackNotEmpty) {
        // 4.1: Извлечь состояние из стека
        std::shared_ptr<NFAState> current = stack[stackIndex];
        stackIndex++;

        // 4.2: Для каждого исходящего перехода
        size_t outIndex = 0;
        while (outIndex < current->outgoingTransitions.size()) {
            std::shared_ptr<Trans> trans = current->outgoingTransitions[outIndex];
            if (trans->isEpsilon()) {
                std::shared_ptr<NFAState> target = trans->getTarget();
                if (target) {
                    // 4.2.1: Если целевого состояния ещё нет — добавить в множество и стек
                    if (nextStates.find(target) == nextStates.end()) {
                        nextStates.insert(target);
                        stack.push_back(target);
                    }
                }
            }
            outIndex++;
        }

        stackNotEmpty = stackIndex < stack.size();
    }

    // 5: Вернуть множество после символьного шага и eps-замыкания
    return nextStates;
}

void epsilonClosure(std::unordered_set<ActiveState>& states)
{
    // 1: Создать стек и поместить в него начальные ActiveState из states
    std::vector<ActiveState> stack;
    for (const ActiveState& activeState : states) {
        stack.push_back(activeState);
    }

    // 2: Пока стек не пуст
    size_t stackIndex = 0;
    bool stackNotEmpty = stackIndex < stack.size();
    while (stackNotEmpty) {
        // 2.1: Извлечь активное состояние из стека
        ActiveState current = stack[stackIndex];
        stackIndex++;

        // 2.2: Для каждого исходящего перехода
        size_t transIndex = 0;
        while (transIndex < current.state->outgoingTransitions.size()) {
            std::shared_ptr<Trans> trans = current.state->outgoingTransitions[transIndex];

            // 2.2.1: Если переход пустой (EpsTrans)
            if (trans->isEpsilon()) {
                std::shared_ptr<NFAState> target = trans->getTarget();
                if (target) {
                    // 2.2.1.2: Проверка уникальности целевого состояния во множестве states
                    ActiveState probe;
                    probe.state = target;

                    bool targetAlreadyPresent = states.find(probe) != states.end();
                    if (!targetAlreadyPresent) {
                        ActiveState newActive;
                        newActive.state = target;
                        newActive.currentMatch = current.currentMatch;

                        states.insert(newActive);
                        stack.push_back(newActive);
                    }
                }
            }
            transIndex++;
        }

        stackNotEmpty = stackIndex < stack.size();
    }
}
