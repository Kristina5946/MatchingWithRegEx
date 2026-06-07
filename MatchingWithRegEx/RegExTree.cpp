/*!
 * \file RegExTree.cpp
 * \brief Реализация разбора ОПЗ, построения AST и генерации фрагментов НКА.
 */

#include "pch.h"
#include "RegExTree.h"
#include <string>
#include <vector>
#include <utility>

namespace {
    bool charIsInDelimiter(const std::string& str, size_t index, const std::string& delim)
    {
        bool found = false;
        size_t delimIndex = 0;
        while (!found && delimIndex < delim.length()) {
            if (str[index] == delim[delimIndex]) {
                found = true;
            }
            delimIndex++;
        }
        return found;
    }
    void advancePastDelimiters(const std::string& str, const std::string& delim, size_t& index)
    {
        bool stillOnDelimiter = index < str.length();
        while (stillOnDelimiter) {
            if (charIsInDelimiter(str, index, delim)) {
                index++;
                stillOnDelimiter = index < str.length();
            }
            else {
                stillOnDelimiter = false;
            }
        }
    }
     size_t measureTokenLength(const std::string& str, const std::string& delim, size_t index)
    {
        size_t length = 0;
        size_t probe = index;
        bool insideToken = probe < str.length() && !charIsInDelimiter(str, probe, delim);
        while (insideToken) {
            length++;
            probe++;
            insideToken = probe < str.length() && !charIsInDelimiter(str, probe, delim);
        }
        return length;
    }
    struct NodeFactory {
        static std::shared_ptr<CharNode> makeCharNode(
            const std::vector<std::pair<char, char>>& ranges,
            size_t pos)
        {
            return std::make_shared<CharNode>(ranges, pos);
        }

        static std::shared_ptr<QuantifierNode> makeQuantifierNode(
            int minOccur,
            int maxOccur,
            size_t pos)
        {
            return std::make_shared<QuantifierNode>(nullptr, minOccur, maxOccur, pos);
        }

        static std::shared_ptr<ConcatNode> makeConcatNode(
            size_t arity,
            size_t pos,
            bool binaryWithoutExplicitArity)
        {
            std::vector<std::shared_ptr<RegExNode>> children;
            if (binaryWithoutExplicitArity) {
                return std::make_shared<ConcatNode>(children, pos);
            }
            size_t childIndex = 0;
            while (childIndex < arity) {
                children.push_back(nullptr);
                childIndex++;
            }
            return std::make_shared<ConcatNode>(children, pos);
        }

        static std::shared_ptr<AlternateNode> makeAlternateNode(
            size_t arity,
            size_t pos,
            bool binaryWithoutExplicitArity)
        {
            std::vector<std::shared_ptr<RegExNode>> children;
            if (binaryWithoutExplicitArity) {
                return std::make_shared<AlternateNode>(children, pos);
            }
            size_t childIndex = 0;
            while (childIndex < arity) {
                children.push_back(nullptr);
                childIndex++;
            }
            return std::make_shared<AlternateNode>(children, pos);
        }
    };
    bool parseNonNegativeInteger(
        const std::string& text,
        size_t begin,
        size_t end,
        int& value)
    {
        bool parsed = false;
        if (begin < end) {
            int accumulated = 0;
            size_t index = begin;
            bool digitsOnly = true;
            while (digitsOnly && index < end) {
                char ch = text[index];
                if (ch >= '0' && ch <= '9') {
                    accumulated = accumulated * 10 + (ch - '0');
                    index++;
                }
                else {
                    digitsOnly = false;
                }
            }
            if (digitsOnly && index == end) {
                value = accumulated;
                parsed = true;
            }
        }
        return parsed;
    }
    std::vector<std::pair<char, char>> buildDotMaskRanges()
    {
        std::vector<std::pair<char, char>> ranges;
        ranges.push_back(std::make_pair((char)1, (char)127));
        return ranges;
    }
    std::shared_ptr<RegExNode> createCharNodeFromEscapeToken(
        const std::string& token,
        size_t pos,
        std::unordered_set<Error>& errorMessages)
    {
        std::shared_ptr<RegExNode> node = nullptr;
        if (token.length() < 2) {
            errorMessages.insert(Error(Error::invalidEscapeSequence, pos));
        }
        else {
            char escaped = token[1];
            std::vector<std::pair<char, char>> ranges;
            bool recognized = true;
            if (escaped == 't') {
                ranges.push_back(std::make_pair('\t', '\t'));
            }
            else if (escaped == 'n') {
                ranges.push_back(std::make_pair('\n', '\n'));
            }
            else if (escaped == 's') {
                ranges.push_back(std::make_pair(' ', ' '));
            }
            else if (escaped == '*' || escaped == '+' || escaped == '?' || escaped == '|'
                || escaped == '{' || escaped == '}' || escaped == '&'
                || escaped == '\\' || escaped == '.') {
                ranges.push_back(std::make_pair(escaped, escaped));
            }
            else {
                recognized = false;
            }
            if (recognized) {
                node = NodeFactory::makeCharNode(ranges, pos);
            }
            else {
                errorMessages.insert(Error(Error::invalidEscapeSequence, pos));
            }
        }
        return node;
    }
    std::shared_ptr<RegExNode> createQuantifierFromIntervalToken(
        const std::string& token,
        size_t pos,
        std::unordered_set<Error>& errorMessages)
    {
        std::shared_ptr<RegExNode> node = nullptr;
        bool intervalValid = token.length() >= 2
            && token[0] == '{'
            && token[token.length() - 1] == '}';
        if (!intervalValid) {
            errorMessages.insert(Error(Error::unrecognizedSequence, pos));
        }
        else {
            size_t contentBegin = 1;
            size_t contentEnd = token.length() - 1;
            size_t commaPos = contentBegin;
            bool commaFound = false;
            while (!commaFound && commaPos < contentEnd) {
                if (token[commaPos] == ',') {
                    commaFound = true;
                }
                else {
                    commaPos++;
                }
            }

            int minOccur = 0;
            int maxOccur = 0;
            bool parsed = false;

            if (!commaFound) {
                parsed = parseNonNegativeInteger(token, contentBegin, contentEnd, minOccur);
                if (parsed) {
                    maxOccur = minOccur;
                }
            }
            else {
                bool minParsed = true;
                if (commaPos > contentBegin) {
                    minParsed = parseNonNegativeInteger(token, contentBegin, commaPos, minOccur);
                }
                else {
                    minOccur = 0;
                }

                bool maxParsed = true;
                if (commaPos + 1 < contentEnd) {
                    maxParsed = parseNonNegativeInteger(token, commaPos + 1, contentEnd, maxOccur);
                }
                else {
                    maxOccur = -1;
                }

                parsed = minParsed && maxParsed;
            }

            if (!parsed) {
                errorMessages.insert(Error(Error::unrecognizedSequence, pos));
            }
            else if (maxOccur != -1 && minOccur > maxOccur) {
                errorMessages.insert(Error(Error::invalidInterval, pos));
            }
            else {
                node = NodeFactory::makeQuantifierNode(minOccur, maxOccur, pos);
            }
        }
        return node;
    }
    std::shared_ptr<RegExNode> createOperationNodeFromToken(
        const std::string& token,
        size_t pos,
        std::unordered_set<Error>& errorMessages)
    {
        std::shared_ptr<RegExNode> node = nullptr;
        char opSymbol = token[0];
        int arity = 2;
        bool arityParsed = true;
        if (token.length() > 1) {
            arityParsed = parseNonNegativeInteger(token, 1, token.length(), arity);
        }
        if (!arityParsed) {
            errorMessages.insert(Error(Error::unrecognizedSequence, pos));
        }
        else if (arity < 2) {
            errorMessages.insert(Error(Error::invalidArity, pos));
        }
        else {
            bool binaryWithoutNumber = token.length() == 1;
            if (opSymbol == '&') {
                node = NodeFactory::makeConcatNode(
                    static_cast<size_t>(arity),
                    pos,
                    binaryWithoutNumber);
            }
            else {
                node = NodeFactory::makeAlternateNode(
                    static_cast<size_t>(arity),
                    pos,
                    binaryWithoutNumber);
            }
        }
        return node;
    }

}
std::vector<std::pair<std::string, size_t>> splitStringIntoTokens(
    const std::string& str,
    const std::string& delim)
{
    std::vector<std::pair<std::string, size_t>> tokens;
    size_t index = 0;

    // Пропускаем возможные разделители в самом начале строки
    advancePastDelimiters(str, delim, index);

    bool hasMoreInput = index < str.length();
    while (hasMoreInput) {
        size_t tokenStart = index;
        size_t tokenLength = measureTokenLength(str, delim, index);
        index = tokenStart + tokenLength;

        std::string tokenText = str.substr(tokenStart, tokenLength);
        tokens.push_back(std::make_pair(tokenText, tokenStart));

        advancePastDelimiters(str, delim, index);
        hasMoreInput = index < str.length();
    }

    return tokens;
}

std::vector<std::shared_ptr<RegExNode>> createRegExNodesFromTokens(
    const std::vector<std::pair<std::string, size_t>>& tokens,
    std::unordered_set<Error>& errorMessages)
{
    // 1: Считать, что список узлов пуст
    std::vector<std::shared_ptr<RegExNode>> nodes;

    // 2: Для каждой пары (токен, позиция), пока множество ошибок пустое
    size_t tokenIndex = 0;
    bool continueProcessing = errorMessages.empty() && tokenIndex < tokens.size();
    while (continueProcessing) {
        const std::string& token = tokens[tokenIndex].first;
        size_t pos = tokens[tokenIndex].second;
        std::shared_ptr<RegExNode> createdNode = nullptr;

        // 2.1: Токен начинается с «\» — escape-последовательность
        if (!token.empty() && token[0] == '\\') {
            createdNode = createCharNodeFromEscapeToken(token, pos, errorMessages);
        }
        // 2.2: Токен «.» — маска любого символа
        else if (token == ".") {
            createdNode = NodeFactory::makeCharNode(buildDotMaskRanges(), pos);
        }
        // 2.3: Токены *, +, ? — стандартные квантификаторы
        else if (token == "*") {
            createdNode = NodeFactory::makeQuantifierNode(0, -1, pos);
        }
        else if (token == "+") {
            createdNode = NodeFactory::makeQuantifierNode(1, -1, pos);
        }
        else if (token == "?") {
            createdNode = NodeFactory::makeQuantifierNode(0, 1, pos);
        }
        // 2.4: Токен начинается с «{» — интервальный квантификатор
        else if (!token.empty() && token[0] == '{') {
            createdNode = createQuantifierFromIntervalToken(token, pos, errorMessages);
        }
        // 2.5: Токен начинается с «&» или «|» — операция с арностью
        else if (!token.empty() && (token[0] == '&' || token[0] == '|')) {
            createdNode = createOperationNodeFromToken(token, pos, errorMessages);
        }
        // 2.6: Иначе — один литеральный символ
        else if (token.length() == 1) {
            std::vector<std::pair<char, char>> ranges;
            ranges.push_back(std::make_pair(token[0], token[0]));
            createdNode = NodeFactory::makeCharNode(ranges, pos);
        }
        else {
            errorMessages.insert(Error(Error::unrecognizedSequence, pos));
        }

        // 2.7: Поместить созданный узел в конец списка
        if (createdNode != nullptr) {
            nodes.push_back(createdNode);
        }

        tokenIndex++;
        continueProcessing = errorMessages.empty() && tokenIndex < tokens.size();
    }

    // 3: Очистить список узлов, если при разборе появились ошибки
    if (!errorMessages.empty()) {
        nodes.clear();
    }

    return nodes;
}

std::shared_ptr<RegExNode> createRegExTreeFromRegExNodes(
    std::vector<std::shared_ptr<RegExNode>>& nodes,
    std::unordered_set<Error>& errorMessages)
{
    std::shared_ptr<RegExNode> root = nullptr;

    // 1: Создать пустой стек для хранения указателей на узлы-операнды
    std::vector<std::shared_ptr<RegExNode>> operandStack;

    // 2: Для каждого текущего узла из входного списка последовательно
    size_t nodeIndex = 0;
    bool keepProcessing = errorMessages.empty() && nodeIndex < nodes.size();
    while (keepProcessing) {
        std::shared_ptr<RegExNode> current = nodes[nodeIndex];

        // 2.1: Текущий узел — операнд (символ или маска)
        if (dynamic_cast<CharNode*>(current.get()) != nullptr) {
            // 2.1.1: Поместить узел в стек операндов
            operandStack.push_back(current);
        }
        // 2.2: Текущий узел — квантификатор (унарная операция)
        else if (dynamic_cast<QuantifierNode*>(current.get()) != nullptr) {
            if (!operandStack.empty()) {
                // 2.2.1: Взять операнд со стека, привязать к квантификатору, вернуть квантификатор в стек
                std::shared_ptr<RegExNode> operand = operandStack.back();
                operandStack.pop_back();
                std::shared_ptr<QuantifierNode> quantNode =
                    std::dynamic_pointer_cast<QuantifierNode>(current);
                quantNode->child = operand;
                operandStack.push_back(quantNode);
            }
            else {
                // 2.2.2: Недостаточно операндов
                errorMessages.insert(
                    Error(Error::insufficientOperands, current->startPosInStr));
            }
        }
        // 2.3: Текущий узел — операция (конкатенация или альтернатива)
        else if (dynamic_cast<ConcatNode*>(current.get()) != nullptr
            || dynamic_cast<AlternateNode*>(current.get()) != nullptr) {
            // 2.3.1: Определить арность операции
            size_t arity = 2;
            if (auto concatNode = dynamic_cast<ConcatNode*>(current.get())) {
                if (!concatNode->children.empty()) {
                    arity = concatNode->children.size();
                }
            }
            else if (auto altNode = dynamic_cast<AlternateNode*>(current.get())) {
                if (!altNode->children.empty()) {
                    arity = altNode->children.size();
                }
            }

            if (operandStack.size() >= arity) {
                // 2.3.2: Извлечь операнды со стека в исходном порядке следования
                std::vector<std::shared_ptr<RegExNode>> operands;
                size_t takenCount = 0;
                while (takenCount < arity) {
                    operands.insert(operands.begin(), operandStack.back());
                    operandStack.pop_back();
                    takenCount++;
                }
                if (auto concatNode = std::dynamic_pointer_cast<ConcatNode>(current)) {
                    concatNode->children = operands;
                    operandStack.push_back(concatNode);
                }
                else {
                    std::shared_ptr<AlternateNode> altNode =
                        std::dynamic_pointer_cast<AlternateNode>(current);
                    altNode->children = operands;
                    operandStack.push_back(altNode);
                }
            }
            else {
                // 2.3.3: Недостаточно операндов
                errorMessages.insert(
                    Error(Error::insufficientOperands, current->startPosInStr));
            }
        }

        nodeIndex++;
        keepProcessing = errorMessages.empty() && nodeIndex < nodes.size();
    }

    // 3: После обхода в стеке ровно один элемент — корень дерева
    if (errorMessages.empty() && operandStack.size() == 1) {
        root = operandStack.back();
    }
    // 4: В стеке более одного элемента — избыточные операнды
    else if (errorMessages.empty() && operandStack.size() > 1) {
        errorMessages.insert(
            Error(Error::excessiveOperands, operandStack.back()->startPosInStr));
        root = nullptr;
    }

    return root;
}

std::shared_ptr<RegExNode> buildRegExTreeFromPostfixNotation(
    const std::string& str,
    std::unordered_set<Error>& errorMessages)
{
    std::shared_ptr<RegExNode> root = nullptr;

    // 1: Разделитель токенов в постфиксной записи — пробел
    const std::string delimiter = " ";

    // 2: Разбить строку на токены (подстрока и позиция начала в исходной строке)
    std::vector<std::pair<std::string, size_t>> tokenPairs = splitStringIntoTokens(str, delimiter);

    // 3: Если токенов нет — ошибка emptyTemplate
    if (tokenPairs.empty()) {
        errorMessages.insert(Error(Error::emptyTemplate, 0));
    }

    // 4: Создать упорядоченный список узлов по токенам
    std::vector<std::shared_ptr<RegExNode>> nodeList;
    if (errorMessages.empty()) {
        nodeList = createRegExNodesFromTokens(tokenPairs, errorMessages);
    }

    // 5: Собрать дерево из списка узлов, если ошибок пока нет
    if (errorMessages.empty()) {
        root = createRegExTreeFromRegExNodes(nodeList, errorMessages);
    }

    // 6: При любой ошибке корень дерева — нулевой указатель
    if (!errorMessages.empty()) {
        root = nullptr;
    }

    return root;
}

void CharNode::buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end)
{
    // 1: Создать один объект CharTrans с вектором charRanges
    std::shared_ptr<CharTrans> transition =
        std::make_shared<CharTrans>(end, start, charRanges);

    // 2: Целевое состояние перехода — end (задано в конструкторе CharTrans)

    // 3: Добавить переход в исходящие переходы состояния start
    start->outgoingTransitions.push_back(transition);

    // 4: Добавить переход во входящие переходы состояния end
    end->incomingTransitions.push_back(transition);
}

void ConcatNode::buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end)
{
    // 1: Если дочерних узлов нет — eps-переход напрямую из start в end
    if (children.empty()) {
        std::shared_ptr<EpsTrans> eps = std::make_shared<EpsTrans>(end, start);
        start->outgoingTransitions.push_back(eps);
        end->incomingTransitions.push_back(eps);
    }
    else {
        // 2: Промежуточных состояний на один меньше, чем дочерних узлов
        std::shared_ptr<NFAState> blockStart = start;
        size_t childIndex = 0;
        while (childIndex < children.size()) {
            std::shared_ptr<NFAState> blockEnd;
            // 5: Последний ребёнок заканчивается в общем end
            if (childIndex + 1 == children.size()) {
                blockEnd = end;
            }
            else {
                blockEnd = std::make_shared<NFAState>();
            }
            // 3–4: Построить фрагмент НКА для текущего дочернего узла
            children[childIndex]->buildNFA(blockStart, blockEnd);
            blockStart = blockEnd;
            childIndex++;
        }
    }
}

void AlternateNode::buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end)
{
    // 1: Если дочерних узлов нет — eps-переход напрямую из start в end
    if (children.empty()) {
        std::shared_ptr<EpsTrans> eps = std::make_shared<EpsTrans>(end, start);
        start->outgoingTransitions.push_back(eps);
        end->incomingTransitions.push_back(eps);
    }
    else {
        // 2: Пройти по всем дочерним узлам альтернативы
        size_t childIndex = 0;
        while (childIndex < children.size()) {
            // 2.1: Построить ветвь с общими start и end (параллельные пути)
            children[childIndex]->buildNFA(start, end);
            childIndex++;
        }
    }
}

void QuantifierNode::buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end)
{
    std::shared_ptr<NFAState> innerStart;
    std::shared_ptr<NFAState> innerEnd;
    std::shared_ptr<NFAState> chainEnd = start;

    // 1: При minOccur == 0 — прямой eps-переход из start в end (пропуск фрагмента)
    if (minOccur == 0) {
        std::shared_ptr<EpsTrans> skipEps = std::make_shared<EpsTrans>(end, start);
        start->outgoingTransitions.push_back(skipEps);
        end->incomingTransitions.push_back(skipEps);
    }

    // 2: Цепочка из minOccur обязательных копий автомата дочернего узла
    std::shared_ptr<NFAState> repeatStart;
    int minIndex = 0;
    while (minIndex < minOccur) {
        innerStart = chainEnd;

        ConcatNode* concatChild = dynamic_cast<ConcatNode*>(child.get());
        bool builtConcatParts = false;
        if (concatChild != nullptr && concatChild->children.size() > 2
            && maxOccur == -1 && minOccur > 0) {
            std::shared_ptr<NFAState> afterFirst = std::make_shared<NFAState>();
            concatChild->children[0]->buildNFA(chainEnd, afterFirst);
            repeatStart = afterFirst;

            std::shared_ptr<NFAState> linkStart = afterFirst;
            size_t childIndex = 1;
            while (childIndex < concatChild->children.size()) {
                std::shared_ptr<NFAState> linkEnd = std::make_shared<NFAState>();
                concatChild->children[childIndex]->buildNFA(linkStart, linkEnd);
                linkStart = linkEnd;
                childIndex++;
            }
            innerEnd = linkStart;
            chainEnd = linkStart;
            builtConcatParts = true;
        }

        if (!builtConcatParts) {
            std::shared_ptr<NFAState> nextState = std::make_shared<NFAState>();
            child->buildNFA(chainEnd, nextState);
            innerEnd = nextState;
            chainEnd = nextState;
        }

        minIndex++;
    }

    // Точный интервал {n}: последнее обязательное звено должно вести в end
    if (maxOccur != -1 && maxOccur == minOccur && minOccur > 0) {
        std::shared_ptr<EpsTrans> finishEps = std::make_shared<EpsTrans>(end, chainEnd);
        chainEnd->outgoingTransitions.push_back(finishEps);
        end->incomingTransitions.push_back(finishEps);
    }

    // 3: Неограниченное число повторений (maxOccur == -1)
    if (maxOccur == -1) {
        // 3.1: Определить тело цикла (innerStart, innerEnd)
        if (minOccur == 0) {
            innerStart = std::make_shared<NFAState>();
            innerEnd = std::make_shared<NFAState>();
            child->buildNFA(innerStart, innerEnd);
        }

        // 3.2: Только при minOccur == 0 — eps из start в начало тела цикла
        if (minOccur == 0) {
            std::shared_ptr<EpsTrans> enterEps = std::make_shared<EpsTrans>(innerStart, start);
            start->outgoingTransitions.push_back(enterEps);
            innerStart->incomingTransitions.push_back(enterEps);
        }

        // 3.3: Обратный eps-переход из конца тела цикла в его начало
        std::shared_ptr<NFAState> loopTarget = innerStart;
        if (minOccur > 0 && repeatStart != nullptr) {
            loopTarget = repeatStart;
        }
        std::shared_ptr<EpsTrans> loopEps = std::make_shared<EpsTrans>(loopTarget, innerEnd);
        innerEnd->outgoingTransitions.push_back(loopEps);
        loopTarget->incomingTransitions.push_back(loopEps);

        // 3.4: eps из конца тела цикла в общее конечное состояние end
        std::shared_ptr<EpsTrans> exitEps = std::make_shared<EpsTrans>(end, innerEnd);
        innerEnd->outgoingTransitions.push_back(exitEps);
        end->incomingTransitions.push_back(exitEps);
    }

    // После обязательной части можно остановиться (диапазон {min,max})
    if (maxOccur != -1 && maxOccur > minOccur && minOccur > 0) {
        std::shared_ptr<EpsTrans> minExitEps = std::make_shared<EpsTrans>(end, chainEnd);
        chainEnd->outgoingTransitions.push_back(minExitEps);
        end->incomingTransitions.push_back(minExitEps);
    }

    // 4: Ограниченный диапазон — необязательные копии (maxOccur > minOccur)
    if (maxOccur > minOccur) {
        std::shared_ptr<NFAState> optionalStart = chainEnd;
        int optionalCount = maxOccur - minOccur;
        int optIndex = 0;
        while (optIndex < optionalCount) {
            std::shared_ptr<NFAState> nextState = std::make_shared<NFAState>();
            child->buildNFA(optionalStart, nextState);
            std::shared_ptr<EpsTrans> earlyExitEps = std::make_shared<EpsTrans>(end, nextState);
            nextState->outgoingTransitions.push_back(earlyExitEps);
            end->incomingTransitions.push_back(earlyExitEps);
            optionalStart = nextState;
            optIndex++;
        }
    }
}
