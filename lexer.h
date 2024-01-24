#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <algorithm>
#include <unordered_map>


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
    String (std::string&& stroke) : value(std::move(stroke)) {}
    String (const std::string& stroke) : value(stroke) {}
    String (std::vector <char>& vec) {
        value  = { vec.begin(), vec.end() };
    }
    std::string value;
};

struct Class {  // Лексема «class»
    std::string name; // Имя класса
};    

struct Return {};   // Лексема «return»
struct If {};       // Лексема «if»
struct Else {};     // Лексема «else»
struct Def {};      // Лексема «def»
struct Newline {    // Лексема «конец строки»
    //char two_dots = '/n';
};  
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
    = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
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
    explicit Lexer(std::istream& input) 
                    : input_(input)
                    
                    {   
                        vector_buff_.resize(input_.rdbuf()->in_avail());
                        input_.read(vector_buff_.data(), input_.rdbuf()->in_avail());
                        //std::reverse(vector_buff_.begin(), vector_buff_.end()); 
                       //std::cout << vector_buff_;
                        //vector_buff_ = {std::istreambuf_iterator<char>{input_},{}};
                        //buffer_stream_ << input_;
                        do  {
                            token_ = NextToken();
                        } while (token_ == Token(token_type::Newline{}));
                    }

    // Возвращает ссылку на текущий токен или token_type::Eof, если поток токенов закончился
    [[nodiscard]] const Token& CurrentToken() const;

    // Возвращает следующий токен, либо token_type::Eof, если поток токенов закончился
    Token NextToken();

    // Если текущий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& Expect() const {
        using namespace std::literals;
        // Заглушка. Реализуйте метод самостоятельно
        if ( !token_.Is<T>() )  {
            throw LexerError("Not implemented"s);
        } 
        return token_.As<T>();
    }

    // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void Expect(const U& val) const {
        using namespace std::literals;
        // Заглушка. Реализуйте метод самостоятельно
        if ( !token_.Is<T>()  )  {
            throw LexerError("Not implemented"s);
        } 
        else if ( token_.As<T>() != T{val} ) {
            throw LexerError("Not implemented"s);
        }
        //return token_.As<T>();
        
    }

    // Если следующий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& ExpectNext() {
        using namespace std::literals;
        
        if (!NextToken().Is<T>()) {
             throw LexerError("Not implemented"s);
        }
        return token_.As<T>();
    }

    // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void ExpectNext(const U& val) {
        using namespace std::literals;
        if (!NextToken().Is<T>()) {
             throw LexerError("Not implemented"s);
        }
        else if ( token_.As<T>() != T{val} ) {
            throw LexerError("Not implemented"s);
        }
    }

    Token NameParsing () const;
    Token StringParsing () const;
    Token NumberParsing () const;
    std::optional<Token> CheckIntend (); 
    std::optional<Token> EOFParsing ();

private:
    
    //std::vector <Token> lexems_;
    Token token_;
    std::istream& input_;
    std::unordered_map <std::string, Token> tokens_map_ {
          {"class", Token {parse::token_type::Class {}}}
        , {"return", Token {parse::token_type::Return {}}}
        , {"if", Token {parse::token_type::If {}}}
        , {"else", Token {parse::token_type::Else {}}}
        , {"def", Token {parse::token_type::Def {}}}
        , {"print", Token {parse::token_type::Print {}}}
        , {"or", Token {parse::token_type::Or {}}}
        , {"None", Token {parse::token_type::None {}}}
        , {"and", Token {parse::token_type::And {}}}
        , {"not", Token {parse::token_type::Not {}}}
        , {"True", Token {parse::token_type::True {}}}
        , {"False", Token {parse::token_type::False {}}}
    };
    int offset_ = 0;
    //mutable std::stringstream buffer_stream_;
    mutable std::vector <char> vector_buff_;
};
//"class return if else def print or None and not True False"
}  // namespace parse