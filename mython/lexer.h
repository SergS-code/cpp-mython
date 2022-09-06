#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <iostream>
#include <string>

namespace parse {

namespace token_type {
struct Number {  // Лексема «число»
    int value;   // число
};

struct Id {             // Лексема «идентификатор»
    std::string value;  // Имя идентификатора
};

struct Char {    // Лексема «символ»
    char value;  // код символа
};

struct String {  // Лексема «строковая константа»
    std::string value;
};

struct Class {};    // Лексема «class»
struct Return {};   // Лексема «return»
struct If {};       // Лексема «if»
struct Else {};     // Лексема «else»
struct Def {};      // Лексема «def»
struct Newline {};  // Лексема «конец строки»
struct Print {};    // Лексема «print»
struct Indent {};  // Лексема «увеличение отступа», соответствует двум пробелам
struct Dedent {};  // Лексема «уменьшение отступа»
struct Eof {};     // Лексема «конец файла»
struct And {};     // Лексема «and»
struct Or {};      // Лексема «or»
struct Not {};     // Лексема «not»
struct Eq {};      // Лексема «==»
struct NotEq {};   // Лексема «!=»
struct LessOrEq {};     // Лексема «<=»
struct GreaterOrEq {};  // Лексема «>=»
struct None {};         // Лексема «None»
struct True {};         // Лексема «True»
struct False {};        // Лексема «False»
}  // namespace token_type

using TokenBase
    = std::variant<std::monostate, token_type::Number, token_type::Id, token_type::Char, token_type::String,
                   token_type::Class, token_type::Return, token_type::If, token_type::Else,
                   token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
                   token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
                   token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                   token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase {
    using TokenBase::TokenBase;

    template <typename T>
    [[nodiscard]] bool Is() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T& As() const {
        return std::get<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T* TryAs() const {
        return std::get_if<T>(this);
    }
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

std::ostream& operator<<(std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Lexer {
public:
    explicit Lexer(std::istream& input);

    // Возвращает ссылку на текущий токен или token_type::Eof, если поток токенов закончился
    [[nodiscard]] const Token& CurrentToken() const;

    // // Возвращает следующий токен, либо token_type::Eof, если поток токенов закончился
    Token NextToken();

    // // Если текущий токен имеет тип T, метод возвращает ссылку на него.
    // // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& Expect() const {
        using namespace std::literals;
        if(!tempToken_.Is<T>()) {
            throw LexerError("Not implemented"s);
        }
        return tempToken_.As<T>();
    }

    // // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
    // // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void Expect(const U& value) const {
        using namespace std::literals;
        if(tempToken_.Is<T>()) {
            T valuee = tempToken_.As<T>();
            if(valuee.value == value) return;
        }
        throw LexerError("Not implemented"s);
    }

    // // Если следующий токен имеет тип T, метод возвращает ссылку на него.
    // // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& ExpectNext() {
        using namespace std::literals;
        Token rez = NextToken();
        if(!rez.Is<T>()) {
            throw LexerError("Not implemented"s);
        }
        return tempToken_.As<T>();
    }

    // // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
    // // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void ExpectNext(const U& value) {
        using namespace std::literals;
        Token rez = NextToken();
        if(rez.Is<T>()) {
            T val = tempToken_.As<T>();
            if(val.value == value) return;
        }
        throw LexerError("Not implemented"s);
    }

private:
    int tempIndent_=0;
    int prevIndent_=0;
    std::istream& input_;
    std::vector<Token>dataTokens_;
    Token tempToken_;
    int tempPos_=0;
    std::istreambuf_iterator<char> it_;
    std::istreambuf_iterator<char> end_;

    std::string LoadString(char charStop);
    int LoadNumber();
    Token LoadId();
    Token CheckLexem(std::string & str);
    Token CheckCharOrDoubleLexem(char tempChar);
    bool CheckToNewLine();
    char Get();
    char SkipCom(char charElem);

};

}  // namespace parse
