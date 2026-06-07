#include "pch.h"
/*!
 * \file TestHelpers.cpp
 * \brief Реализация алгоритмов структурного сравнения деревьев (AST) и проверки эквивалентности автоматов (НКА).
 */

#include "TestHelpers.h"
#include <sstream>
#include <stack>
#include <queue>
#include <algorithm>
#include <cctype>
#include <map>
#include <set>

#include <unordered_set>
#include <memory>

namespace TestHelpers {

    /*!
     * \brief Класс-адаптер для безопасного доступа к защищенному полю charRanges внутри CharTrans.
     * Позволяет извлекать информацию о символах для точного сравнения переходов автоматов.
     */
    class CharTransAccessor : public CharTrans {
    public:
        // Возвращает константную ссылку на вектор диапазонов символов, чтобы избежать лишнего копирования данных.
        static const std::vector<std::pair<char, char>>& getRanges(const CharTrans* ct) {
            // принудительно делаем указатель базового класса в указатель класса-наследника,для  прочтения скрытого поля.
            return static_cast<const CharTransAccessor*>(ct)->charRanges;
        }
    };

    //вектор из пар например{ 'a','z' } и{ '0','9' } переделываем в строку "{['a'-'z'], ['0'-'9']}"
    std::string charGroupsToString(const std::vector<std::pair<char, char>>& groups) {
        std::string str = "{";
        bool first = true;
        for (const auto& group : groups) {
            if (!first) str += ", ";
            char start = group.first;
            char end = group.second;
            str += "['" + std::string(1, (isprint(start) ? start : ':')) + "'-'" +
                std::string(1, (isprint(end) ? end : ':')) + "']";
            first = false;
        }
        str += "}";
        return str;
    }
    static std::string nodeToString(const RegExNode* node) {
        if (!node) return "nullptr";

        if (auto charNode = dynamic_cast<const CharNode*>(node)) {
            return "CharNode " + charGroupsToString(charNode->charRanges);
        }
        if (dynamic_cast<const ConcatNode*>(node)) {
            return "ConcatNode (&)";
        }
        if (dynamic_cast<const AlternateNode*>(node)) {
            return "AlternateNode (|)";
        }
        if (auto qNode = dynamic_cast<const QuantifierNode*>(node)) {
            return "QuantifierNode {" + std::to_string(qNode->minOccur) + ", " + std::to_string(qNode->maxOccur) + "}";
        }
        return "UnknownNode";
    }
    static std::string treeToString(const RegExNode* root) {
        if (!root) return "";
        // Получаем строковое описание текущего корня
        std::string result = nodeToString(root);
        std::vector<std::shared_ptr<RegExNode>> children;

        // В зависимости от типа узла извлекаем список его дочерних элементов
        if (auto cNode = dynamic_cast<const ConcatNode*>(root)) {
            children = cNode->children;
        }
        else if (auto aNode = dynamic_cast<const AlternateNode*>(root)) {
            children = aNode->children;
        }
        else if (auto qNode = dynamic_cast<const QuantifierNode*>(root)) {
            children.push_back(qNode->child);
        }
        // Если у узла есть потомки, рекурсивно обходим их и добавляем в строку в скобках
        if (!children.empty()) {
            result += "(";
            for (size_t i = 0; i < children.size(); ++i) {
                result += treeToString(children[i].get());
                if (i < children.size() - 1) result += ", ";
            }
            result += ")";
        }
        return result;
    }

    // Рекурсивно сравнивает два синтаксических дерева (AST) по узлам, собирая все ошибки.
    void compareExpressionTrees(const RegExNode* expected, const RegExNode* actual, std::vector<std::string>& errors, std::string path) {
        if (!expected && !actual) return; // оба узла пустые, значит ветви совпали
        if (!expected || !actual) { // Если один узел существует, а второй нет
            errors.push_back("Расходятся ветви в узле [" + path + "]. Один из узлов отсутствует (nullptr).");
            return; // Дальше в этой ветви идти некуда
        }

        // Сверяем типы и внутренние параметры узлов
        std::string expType = nodeToString(expected);
        std::string actType = nodeToString(actual);

        if (expType != actType) {
            errors.push_back("Несовпадение типов или параметров в [" + path + "].\nОжидалось: " + expType + "\nФактически: " + actType);
        }

        // Подготавливаем массивы потомков для дальнейшего рекурсивного сравнения
        std::vector<std::shared_ptr<RegExNode>> expChildren, actChildren;

        // Извлекаем потомков из эталонного и фактического узлов
        if (auto cExp = dynamic_cast<const ConcatNode*>(expected)) {
            expChildren = cExp->children;
            if (auto cAct = dynamic_cast<const ConcatNode*>(actual)) actChildren = cAct->children;
        }
        else if (auto aExp = dynamic_cast<const AlternateNode*>(expected)) {
            expChildren = aExp->children;
            if (auto aAct = dynamic_cast<const AlternateNode*>(actual)) actChildren = aAct->children;
        }
        else if (auto qExp = dynamic_cast<const QuantifierNode*>(expected)) {
            expChildren.push_back(qExp->child);
            if (auto qAct = dynamic_cast<const QuantifierNode*>(actual)) actChildren.push_back(qAct->child);
        }

        if (expChildren.size() != actChildren.size()) {
            errors.push_back("Разное количество потомков в узле [" + path + "].\nОжидалось: " +
                std::to_string(expChildren.size()) + "\nФактически: " + std::to_string(actChildren.size()));
        }

        // Перед сравнением потомков альтернативы мы сортируем их по их строковому представлению.
        if (dynamic_cast<const AlternateNode*>(expected) && expChildren.size() == actChildren.size()) {
            struct RegExNodePtrLess {
                static bool compare(const std::shared_ptr<RegExNode>& a, const std::shared_ptr<RegExNode>& b) {
                    return treeToString(a.get()) < treeToString(b.get());
                }
            };
            std::sort(expChildren.begin(), expChildren.end(), RegExNodePtrLess::compare);
            std::sort(actChildren.begin(), actChildren.end(), RegExNodePtrLess::compare);
        }

        // Синхронно погружаемся в каждого потомка для проверки следующих уровней дерева
        size_t minChildren = std::min(expChildren.size(), actChildren.size());
        for (size_t i = 0; i < minChildren; ++i) {
            std::string newPath = path + " -> child[" + std::to_string(i) + "]";
            compareExpressionTrees(expChildren[i].get(), actChildren[i].get(), errors, newPath);
        }
        // Если все проверки пройдены, деревья идентичны (массив ошибок останется пустым)
    }

    static std::set<NFAState*> getEpsilonClosure(const std::set<NFAState*>& startStates) {
        std::stack<NFAState*> stack;
        std::set<NFAState*> closure = startStates;
        // Помещаем все начальные состояния в стек для обработки
        for (auto s : startStates) {
            stack.push(s);
        }
        // Пока есть необработанные состояния
        while (!stack.empty()) {
            NFAState* s = stack.top();
            stack.pop();
            // Проверяем все исходящие пути из текущего состояния
            for (const auto& trans : s->outgoingTransitions) {
                NFAState* targetState = trans->getTarget().get();
                // Если переход является пустым (эпсилон) и целевое состояние еще не добавлено в замыкание
                if (trans->isEpsilon() && closure.find(targetState) == closure.end()) {
                    closure.insert(targetState);// Фиксируем новое достижимое состояние
                    stack.push(targetState);// Отправляем его в стек, чтобы проверить и его эпсилон-пути
                }
            }
        }
        return closure;// Возвращаем полное множество достижимых состояний
    }
    static std::map<std::string, std::set<NFAState*>> getTransitionsFromClosure(const std::set<NFAState*>& closure) {
        std::map<std::string, std::set<NFAState*>> transitions;
        // Проходим по каждому состоянию внутри текущего эпсилон-замыкания
        for (auto state : closure) {
            // Проверяем все исходящие ребра из текущего состояния
            for (const auto& trans : state->outgoingTransitions) {
                // Игнорируем эпсилон-переходы, так как нас интересуют только сдвиги по символам
                if (!trans->isEpsilon()) {
                    std::string label = "unknown";
                    // Извлекаем символ или маску перехода для использования в качестве ключа словаря
                    if (auto charTrans = dynamic_cast<const CharTrans*>(trans.get())) {
                        label = charGroupsToString(CharTransAccessor::getRanges(charTrans));
                    }
                    // Добавляем целевое состояние в группу, соответствующую данному символу
                    transitions[label].insert(trans->getTarget().get());
                }
            }
        }
        return transitions;
    }
    static bool containsTerminal(const std::set<NFAState*>& states, const NFAState* terminal) {
        // Ищем указатель на терминальное состояние внутри переданного множества
        return states.find(const_cast<NFAState*>(terminal)) != states.end();
    }

    // Алгоритм синхронно обходит оба автомата в ширину (BFS), формируя макросостояния и собирая все рассинхроны.
    void areNFAsEquivalent(const NFA& expected, const NFA& actual, std::vector<std::string>& errors) {
        // Шаг 1: Формируем стартовые состояния (эпсилон-замыкания от начальных узлов)
        std::set<NFAState*> expStartClosure = getEpsilonClosure({ expected.startState.get() });
        std::set<NFAState*> actStartClosure = getEpsilonClosure({ actual.startState.get() });
        
        // Очередь для синхронного обхода графов. Хранит пары сравниваемых множеств состояний.
        std::queue<std::pair<std::set<NFAState*>, std::set<NFAState*>>> queue;
        queue.push({ expStartClosure, actStartClosure });

        // Вектор посещенных пар множеств для предотвращения зацикливания при наличии петель в графе
        std::vector<std::pair<std::set<NFAState*>, std::set<NFAState*>>> visited;
        visited.push_back({ expStartClosure, actStartClosure });

        // Шаг 2: Основной цикл синхронного обхода
        while (!queue.empty()) {
            auto currentPair = queue.front();
            queue.pop();

            const auto& expSet = currentPair.first;
            const auto& actSet = currentPair.second;

            // Проверяем статус допуска для текущей пары множеств.
            // Если один автомат считает текущую последовательность символов финальной, а другой нет — они не равны.
            bool expIsTerminal = containsTerminal(expSet, expected.terminalState.get());
            bool actIsTerminal = containsTerminal(actSet, actual.terminalState.get());

            if (expIsTerminal != actIsTerminal) {
                errors.push_back("Рассинхронизация: при идентичной последовательности символов один автомат переходит в допускающее состояние, а другой — нет.");
            }

            // Извлекаем все доступные символьные переходы из текущих
            auto expTrans = getTransitionsFromClosure(expSet);
            auto actTrans = getTransitionsFromClosure(actSet);

            // Количество уникальных символов для продолжения пути должно совпадать
            if (expTrans.size() != actTrans.size()) {
                errors.push_back("Автоматы имеют разное количество уникальных исходящих путей по символам из текущего замыкания.");
            }

            // Шаг 3: Сверяем переходы по каждому конкретному символу
            for (const auto& pair : expTrans) {
                const std::string& symbol = pair.first;
                // Убеждаемся, что фактический автомат может совершить переход по такому же символу
                if (actTrans.find(symbol) == actTrans.end()) {
                    errors.push_back("Фактический автомат не имеет перехода по группе символов, которая присутствует в эталоне: " + symbol);
                    continue; // Переход не найден, пропускаем и проверяем следующие
                }
                
                // Вычисляем следующие состояния: делаем шаг по символу и сразу накрываем результаты эпсилон-замыканием
                std::set<NFAState*> nextExpSet = getEpsilonClosure(expTrans[symbol]);
                std::set<NFAState*> nextActSet = getEpsilonClosure(actTrans[symbol]);
                std::pair<std::set<NFAState*>, std::set<NFAState*>> nextPair = { nextExpSet, nextActSet };

                // Если эта пара множеств формируется впервые, добавляем ее в очередь на проверку
                if (std::find(visited.begin(), visited.end(), nextPair) == visited.end()) {
                    visited.push_back(nextPair);
                    queue.push(nextPair);
                }
            }
        }
    }
    void compareErrorSetsTwoWay(const std::vector<Error>& expected, const std::unordered_set<Error>& actual, std::vector<std::string>& errors, const std::string& context) {
        // 1. Ищем недостающие (есть в эталоне, нет по факту)
        for (const Error& expErr : expected) {
            bool found = false;
            for (const Error& actErr : actual) {
                if (expErr == actErr) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                errors.push_back(context + " - Недостающая ошибка (тип " + std::to_string(expErr.type) + ", позиция " + std::to_string(expErr.pos) + ")");
            }
        }

        // 2. Ищем лишние (есть по факту, нет в эталоне)
        for (const Error& actErr : actual) {
            bool isExpected = false; 
            for (const Error& expErr : expected) {
                if (expErr == actErr) {
                    isExpected = true;
                    break;
                }
            }
            if (!isExpected) {
                errors.push_back(context + " - Лишняя ошибка (тип " + std::to_string(actErr.type) + ", позиция " + std::to_string(actErr.pos) + ")");
            }
        }
    }

    std::vector<int> collectStateIndicesFromActiveSet(const std::vector<std::shared_ptr<NFAState>>& allStates, const std::unordered_set<ActiveState>& activeSet) {
        std::vector<int> actualIndices;
        for (const ActiveState& active : activeSet) {
            for (size_t i = 0; i < allStates.size(); ++i) {
                if (active.state == allStates[i]) {
                    actualIndices.push_back((int)i);
                    break;
                }
            }
        }
        return actualIndices;
    }

    void assertAllReachableNodesHaveDistance(NFAState* startState, std::vector<std::string>& errors) {
        if (!startState) {
            errors.push_back("Стартовое состояние автомата равно nullptr");
            return;
        }
        std::vector<NFAState*> visited;
        std::vector<NFAState*> queue;
        queue.push_back(startState);
        visited.push_back(startState);

        size_t head = 0;
        while (head < queue.size()) {
            NFAState* curr = queue[head];
            head++;

            if (curr->distanceToTerminal == static_cast<size_t>(-1)) {
                errors.push_back("Узел автомата имеет невалидное расстояние до терминала");
            }

            for (const auto& trans : curr->outgoingTransitions) {
                NFAState* target = trans->getTarget().get();
                bool alreadyVisited = false;
                for (NFAState* v : visited) {
                    if (v == target) {
                        alreadyVisited = true;
                        break;
                    }
                }
                if (!alreadyVisited) {
                    visited.push_back(target);
                    queue.push_back(target);
                }
            }
        }
    }

    // Двустороннее сравнение множеств для выявления лишних и недостающих элементов
    void compareSetsTwoWay(const std::vector<int>& expected, const std::vector<int>& actual, std::vector<std::string>& errors, const std::string& context) {
        std::vector<int> expSorted = expected;
        std::vector<int> actSorted = actual;
        std::sort(expSorted.begin(), expSorted.end());
        std::sort(actSorted.begin(), actSorted.end());

        //Сортируем векторы и находим разницу A - B (недостающие) и B - A (лишние).
        std::vector<int> missing;
        std::set_difference(expSorted.begin(), expSorted.end(), actSorted.begin(), actSorted.end(), std::back_inserter(missing));
        std::vector<int> extra;
        std::set_difference(actSorted.begin(), actSorted.end(), expSorted.begin(), expSorted.end(), std::back_inserter(extra));

        if (!missing.empty()) {
            std::string msg = context + " - Недостающие элементы (есть в ожидаемом, нет по факту): ";
            for (int val : missing) msg += std::to_string(val) + " ";
            errors.push_back(msg);
        }
        if (!extra.empty()) {
            std::string msg = context + " - Лишние элементы (есть по факту, нет в ожидаемом): ";
            for (int val : extra) msg += std::to_string(val) + " ";
            errors.push_back(msg);
        }
    }

    void appendErrors(std::vector<std::string>& allErrors, const std::vector<std::string>& newErrors) {
        size_t i = 0;
        while (i < newErrors.size()) {
            allErrors.push_back(newErrors[i]);
            i++;
        }
    }

    void validateMatchResult(int testId, const std::string& inputString, size_t queryStartPos,
        bool expectValid, bool expectFullMatch, size_t expectedLength,
        const Match& actualMatch, std::vector<std::string>& errors)
    {
        std::string prefix = "Тест 3.4." + std::to_string(testId) + ": ";
        if (expectValid != actualMatch.isValid) {
            errors.push_back(prefix + "Ошибка isValid");
            return;
        }
        if (!expectValid) {
            return;
        }
        if (expectFullMatch != actualMatch.isFullMatch) {
            errors.push_back(prefix + "Ошибка типа (Full/Partial)");
        }
        if (queryStartPos != actualMatch.start) {
            errors.push_back(prefix + "Ошибка позиции начала (start)");
        }
        size_t actualLen = actualMatch.end - actualMatch.start;
        if (expectedLength != actualLen) {
            errors.push_back(prefix + "Ошибка длины");
        }
        if (expectedLength > 0) {
            std::string expectedText = inputString.substr(queryStartPos, expectedLength);
            std::string actualText = inputString.substr(actualMatch.start, actualLen);
            if (expectedText != actualText) {
                errors.push_back(prefix + "Ошибка текста");
            }
        }
    }

    void validateDetermineFinalMatch(int testId, bool expectValid, bool expectFull, size_t expectLen,
        const Match& expectedFull, const Match& expectedPartial,
        const Match& result, std::vector<std::string>& errors)
    {
        std::string prefix = "Тест 3.3." + std::to_string(testId) + ": ";
        if (expectValid != result.isValid) {
            errors.push_back(prefix + "Ошибка isValid");
            return;
        }
        if (!expectValid) {
            return;
        }
        if (expectFull != result.isFullMatch) {
            errors.push_back(prefix + "Ошибка типа (Full/Partial)");
        }
        const Match& expectedMatch = expectFull ? expectedFull : expectedPartial;
        if (expectedMatch.start != result.start) {
            errors.push_back(prefix + "Ошибка позиции начала (start)");
        }
        size_t resultLen = result.end - result.start;
        if (expectLen != resultLen) {
            errors.push_back(prefix + "Ошибка длины");
        }
        if (expectedMatch.distanceToTerminal != result.distanceToTerminal) {
            errors.push_back(prefix + "Ошибка дистанции");
        }
    }

} // namespace TestHelpers