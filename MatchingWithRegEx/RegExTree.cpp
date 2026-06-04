#include "pch.h"
#include "RegExTree.h"
#include <string>
#include <vector>
#include <utility>

namespace {

    /*!
     * \brief Проверяет, является ли символ по заданному индексу разделителем.
     * \param str Анализируемая строка.
     * \param index Индекс проверяемого символа.
     * \param delim Строка, содержащая набор разделителей.
     * \return true, если символ является разделителем, иначе false.
     */
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
    /*!
     * \brief Сдвигает индекс вперед, пропуская идущие подряд разделители.
     * \param str Анализируемая строка.
     * \param delim Строка, содержащая набор разделителей.
     * \param index Ссылка на индекс, который будет обновлен.
     */
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
    /*!
     * \brief Вычисляет длину текущего токена (до следующего разделителя или конца строки).
     * \param str Анализируемая строка.
     * \param delim Строка, содержащая набор разделителей.
     * \param index Индекс начала токена.
     * \return Длина токена в символах.
     */
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

    /*!
     * \brief Фабрика узлов дерева (по внутренней спецификации).
     */
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

    /*!
     * \brief Разбирает неотрицательное целое из подстроки [begin, end).
     * \return true, если подстрока состоит только из цифр и число разобрано.
     */
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

    /*!
     * \brief Заполняет charRanges для маски «.» (символы с кодами 1–127).
     */
    std::vector<std::pair<char, char>> buildDotMaskRanges()
    {
        std::vector<std::pair<char, char>> ranges;
        ranges.push_back(std::make_pair((char)1, (char)127));
        return ranges;
    }

    /*!
     * \brief 2.1: Создание CharNode из escape-токена (\t, \n, \s и экранирование спецсимволов).
     */
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

    /*!
     * \brief 2.4: Разбор интервального квантификатора {n}, {n,m}, {n,}, {,m}.
     */
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

    /*!
     * \brief 2.5: Извлечение арности из токена &, |, &n, |n и создание узла операции.
     */
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

/*!
* \brief Разбивает входную строку на токены, используя заданные разделители.
* \param str Входная строка.
* \param delim Строка с символами-разделителями.
* \return Вектор пар, где первая часть — текст токена, вторая — позиция его начала в исходной строке.
*/
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


/*!
 * \brief Создаёт упорядоченный список узлов по токенам ОПЗ (алгоритм createRegExNodesFromTokens).
 */
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

/*!
 * \brief Строит синтаксическое дерево регулярного выражения из обратной польской записи (ОПЗ).
 * \param str Регулярное выражение в формате ОПЗ (токены разделены пробелами).
 * \param errorMessages Множество для записи найденных ошибок парсинга.
 * \return Указатель на корень синтаксического дерева (nullptr при ошибках).
 */
std::shared_ptr<RegExNode> buildRegExTreeFromPostfixNotation(
    const std::string& str,
    std::unordered_set<Error>& errorMessages)
{
    std::shared_ptr<RegExNode> root = nullptr;

    // 1: Разделитель токенов по спецификации проекта — пробел
    const std::string delimiter = " ";

    // 2: Разбиваем строку на токены, сохраняя их позиции для возможных сообщений об ошибках.
    std::vector<std::pair<std::string, size_t>> tokenPairs = splitStringIntoTokens(str, delimiter);

    // 3: Если токенов нет (строка пуста или состоит только из пробелов) — фиксируем ошибку
    if (tokenPairs.empty()) {
        errorMessages.insert(Error(Error::emptyTemplate, 0));
    }
    
    // 4: Стек для построения дерева

    return root;
}
