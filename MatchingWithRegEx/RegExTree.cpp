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
