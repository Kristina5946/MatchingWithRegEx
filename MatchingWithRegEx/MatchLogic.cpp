#include "pch.h"
#include "MatchLogic.h"
#include <deque>

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
