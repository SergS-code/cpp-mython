#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <cctype>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input) : input_(input) {
    it_ = std::istreambuf_iterator<char>(input_);
    end_ = std::istreambuf_iterator<char>();
    NextToken();
}

const Token& Lexer::CurrentToken() const {
    return tempToken_;
}

char Lexer::SkipCom(char charElem) {
    if(charElem == '#') {
        while(input_.peek() != '\n' && !(static_cast<int>((input_.peek())) <= 0)) {
            input_.get();
        }
        if(tempPos_ > 0) {
            return input_.get();
        }
        else {
            return SkipCom(input_.get());
        }
    }
    else {
        return charElem;
    }
}


char Lexer::Get() {
    char tempChar=input_.get();

    if(tempPos_ == 0 && tempChar == '\n') {
        while(tempChar == '\n') {
            tempChar = input_.get();
        }
    }
    if(tempPos_ != 0 && tempChar == ' ') {
        while(input_.peek() == ' ') {
            tempChar = input_.get();
        }
        tempChar = input_.get();
    }

    tempChar=SkipCom(tempChar);

    if(tempIndent_ == 0 && tempChar == ' ') {
        tempIndent_++;
        while(input_.peek() == ' ') {
            tempChar = input_.get();
            tempIndent_++;
        }
        if(tempIndent_ == prevIndent_) {
            tempChar = input_.get();
        }
        if(prevIndent_ > tempIndent_) {
            int checkIndent = prevIndent_  - tempIndent_ - 2;
            if( checkIndent > 0) {
                while(checkIndent > 0) {
                    input_.putback(' ');
                    checkIndent--;
                }
                tempIndent_ = 0;
            }
        }
    }
    if(tempPos_ == 0 && tempChar == '\n') {
        while(tempChar == '\n') {
            tempChar = input_.get();
        }
    }
    tempPos_++;
    return tempChar;
}

Token Lexer::NextToken() {
    Token curToken;
    char tempChar = Get();
    if (static_cast<int>(tempChar) <= 0) {
        if(prevIndent_ > tempIndent_) {
            if((prevIndent_ - tempIndent_) % 2 != 0) {
                throw LexerError("Wrong dedent");
            }
            prevIndent_ -= 2;
            tempToken_= token_type::Dedent{};
            dataTokens_.push_back(tempToken_);
        }
        else if(CheckToNewLine()) {
            tempToken_ = token_type::Newline();
             dataTokens_.push_back(tempToken_);
        }
        else {
            tempToken_ = token_type::Eof();
             dataTokens_.push_back(tempToken_);
        }
         dataTokens_.push_back(tempToken_);
        return tempToken_;
    }

    if(tempChar == ' ') {
        if(tempIndent_ > prevIndent_) {
            if((tempIndent_ - prevIndent_) != 2) {
                throw LexerError("Wrong indent");
            }
            curToken = token_type::Indent{};
        }
        else if(prevIndent_ > tempIndent_) {
            if((prevIndent_ - tempIndent_) % 2 != 0) {
                throw LexerError("Wrong dedent");
            }
            prevIndent_ -= 2;
            curToken= token_type::Dedent{};
        }
        tempPos_ = 0;

    } else if(prevIndent_ > tempIndent_) {
        tempPos_--;
        input_.putback(tempChar);
        if((prevIndent_ - tempIndent_) % 2 != 0) {
            throw LexerError("Wrong dedent");
        }
        prevIndent_ -= 2;
        curToken= token_type::Dedent{};

    } else if(tempChar == '\n') {
        curToken = token_type::Newline{};
        prevIndent_ = tempIndent_;
        tempIndent_ = 0;
        tempPos_ = 0;

    }else if(tempChar == '"'|| tempChar == '\''  ) {

        curToken = token_type::String{LoadString(tempChar)};

    }else if(tempChar =='.'|| tempChar == '*'|| tempChar == '/' || tempChar == ',' || tempChar == '(' ||
            tempChar == '-' || tempChar == '+'|| tempChar == ')' || tempChar == '>' || tempChar == '<' ||
            tempChar == '=' || tempChar == '!'||  tempChar == ':'){

        curToken=CheckCharOrDoubleLexem(tempChar);

    }
    else if(std::isdigit(tempChar)) {
        tempPos_--;
        input_.putback(tempChar);
        curToken = token_type::Number{LoadNumber()};
    }
    else {
        tempPos_--;
        input_.putback(tempChar);
        curToken = LoadId();
    }
    tempToken_ = curToken;
     dataTokens_.push_back(tempToken_);
    return tempToken_;

}
Token Lexer::LoadId() {
    Token token;
    std::string str;
    char ch = input_.peek();
    while (ch != ' ' && ch != '.' && ch != '*' && ch != '/' &&
           ch != ',' && ch != '(' && ch != '-' && ch != '+' &&
           ch != ')' && ch != '>' && ch != '<' && ch != '=' &&
           ch != '!' && ch != ':' && ch != '\n'&&
           !(static_cast<int>(ch) <= 0) && ch != '#') {

        str += Get();
        ch = input_.peek();
    }
    return CheckLexem(str);

}

Token Lexer::CheckLexem(std::string &str)
{
    Token rez;
    if(str == "True") {
        rez = token_type::True{};
        return rez;
    }
    if(str == "False") {
        rez = token_type::False{};
        return rez;
    }
    if(str == "None") {
        rez = token_type::None{};
        return rez;
    }
    if(str == "else") {
        rez = token_type::Else{};
        return rez;
    }
    if(str == "def") {
        rez = token_type::Def{};
        return rez;
    }
    if(str == "and") {
        rez = token_type::And{};
        return rez;
    }
    if(str == "or") {
        rez = token_type::Or{};
        return rez;
    }
    if(str == "class") {
        rez = token_type::Class{};
        return rez;
    }
    if(str == "return") {
        rez = token_type::Return{};
        return rez;
    }
    if(str == "if") {
        rez = token_type::If{};
        return rez;
    }

    if(str == "not") {
        rez = token_type::Not{};
        return rez;
    }
    if(str == "print") {
        rez = token_type::Print{};
        return rez;
    }
    return  rez = token_type::Id{str};

}

Token Lexer::CheckCharOrDoubleLexem(char tempChar)
{
    Token rez;

    string str = {tempChar, static_cast<char>(input_.peek())};

    if(str == "<=") {
        input_.get();
        tempPos_++;
        rez = token_type::LessOrEq{};
        return rez;
    }

    if(str == ">=") {
        input_.get();
        tempPos_++;
        rez = token_type::GreaterOrEq{};
        return rez;
    }

    if(str == "==") {
        input_.get();
        tempPos_++;
        rez = token_type::Eq{};
        return rez;
    }
    if(str == "!=") {
        input_.get();
        tempPos_++;
        rez = token_type::NotEq{};
        return rez;
    }
    return token_type::Char{tempChar};
}

bool Lexer::CheckToNewLine()
{
    if(tempToken_.Is<token_type::Newline>() || tempToken_.Is<token_type::Eof>()
            || tempToken_.Is<token_type::Dedent>() || tempToken_.Is<std::monostate>()){
        return false;
    }
    return true;
}
string Lexer::LoadString(char charStop) {

    char Stop=charStop ;
    std::string s;
    while (true) {
        if (it_ == end_) {
            break;
            //throw ParsingError("String parsing error");
        }
        const char ch = *it_;

        if (ch ==Stop) {
            ++it_;
            break;
        } else if (ch == '\\') {
            ++it_;
            if (it_ == end_) {
                //throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it_);
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            case '\'':
                s.push_back('\'');
                break;
            default: ;
                //throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            //throw ParsingError("Unexpected end of line"s);
        } else {
            s.push_back(ch);
        }
        ++it_;
    }

    return s;
}
int Lexer::LoadNumber() {

    std::string str;
    int rez=0;
    while (std::isdigit(input_.peek())) {
        str += Get();
    }
    if(!str.empty() && str.find_first_not_of("0123456789") == std::string::npos){
        rez=std::stoi(str);
    }
    return rez;
}

}  // namespace parse


