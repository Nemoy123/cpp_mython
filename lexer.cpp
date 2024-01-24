#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <sstream>
#include <iterator>

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

std::optional <int> SearchStringEnd (char* start_pos, char* end_pos, char delimiter) {
    auto iter = find (start_pos, end_pos+1, delimiter);
        if (iter != end_pos + 1) {

            if (*(iter-1) != 92) { // проверяем что не спецсимвол
                // нашли конец
                return std::distance (start_pos, iter);
            }
            else { // спецсимвол, ищем дальше в буфере, рекурсия?
                if (SearchStringEnd ( iter+1, end_pos, delimiter ).has_value()) {
                    return std::distance (start_pos, iter+1) + SearchStringEnd ( iter+1, end_pos, delimiter ).value();
                } else {
                    return std::nullopt;
                }
            }
        }
    return std::nullopt; // считываем еще буфер
}

// ostream& operator<< (ostream& out, const vector <char>& vec) {
//     for (const auto& item : vec) {
//         out << item << "-";
//     }
//     return out;
// }

void CleanVectorChar (std::vector <char>& vec) {    
    std::vector <char> result;
    result.reserve(vec.size());
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == 92 && vec[i + 1] == '"') {
            //result.push_back(vec[i + 1]); 
            continue;    
        }
        else if (vec[i] == 92 && vec[i + 1] == '\'') {
            //result.push_back(vec[i + 1]);
            continue;
        }
        else if (vec[i] == 92 && vec[i + 1] == 't') {
            //result.push_back(vec[i + 1]);
            char tempor = 9;
            result.push_back(tempor);
            vec.erase ((vec.begin()+i+1));
        }
        else if (vec[i] == 92 && vec[i + 1] == 'n') {
            //result.push_back(vec[i + 1]);
            char tempor = '\n';
            result.push_back(tempor);
            vec.erase ((vec.begin()+i+1));
        }
        
        else {
            result.push_back(vec[i]);
        }
    }
    vec = result;
}

Token Lexer::StringParsing () const {
    
    char delimiter; 
    string out_str;
    delimiter = std:: move (vector_buff_.front());
    if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
    //vector_buff_.erase(vector_buff_.begin());
    std::vector <char> temp2;
    while (true) {
        
        auto temp = SearchStringEnd (&vector_buff_.front(), &vector_buff_.back(), delimiter);

        if (temp.has_value()) {
            std::move(&vector_buff_.front(), &vector_buff_.front() + temp.value(), std::back_inserter(temp2));
            vector_buff_.erase(vector_buff_.begin(), vector_buff_.begin() + temp.value() + 1);
            break;
        }
    }

    CleanVectorChar (temp2);
   return Token{parse::token_type::String (temp2)};

}

Token Lexer::NameParsing () const { 
    string result{};
    char ch;
    bool begin = true;
    while(!vector_buff_.empty()) {
       // char test = input_.peek();
        if ((vector_buff_.front() > 64 && vector_buff_.front() < 91) || (vector_buff_.front() > 96 && vector_buff_.front() < 123) || vector_buff_.front() == 95) { 
            ch = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            //buffer_stream_ >> ch;
            result.push_back(ch);
            begin = false;
        }
        else if (vector_buff_.front() > 47 && vector_buff_.front() < 58 && !begin) {
            ch = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            //buffer_stream_ >> ch;
            result.push_back(ch);
        }
        else {
            break;
        }

    }
    if (tokens_map_.find(result) != tokens_map_.end()) {
        return tokens_map_.at(result);
    }
    else {
        return Token{parse::token_type::Id {result}};
    }
}

Token Lexer::NumberParsing () const {
    string result{};
    char ch;
    while(!vector_buff_.empty()) {
        if (vector_buff_.front() > 47 && vector_buff_.front() < 58 ) { // число
            ch = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            //buffer_stream_ >> ch;
            result.push_back(ch);
        }
        else {
            break;
        }
    }
    return Token{parse::token_type::Number {std::stoi (result)}};
}

std::optional<Token> Lexer:: CheckIntend() {
    //char skip;
    int count = 0;
    bool change_input = false;
    while (vector_buff_.front() == ' ') {
        //buffer_stream_.get(skip);
        if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
        change_input = true;
        if (vector_buff_.front() == ' ') {
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            //buffer_stream_.get(skip);
            ++count;
        } else {break;}
    }
    if (count > offset_) {
        token_ = (Token{parse::token_type::Indent{}});
        ++offset_;
        return token_;
    } 
    else if (count < offset_) {
        token_ = (Token{parse::token_type::Dedent{}});
        --offset_;
        if (change_input) {
            for (auto i = 0; i < offset_; ++i) {
                char temp = 32;
                vector_buff_.insert(vector_buff_.begin(), temp);
                vector_buff_.insert(vector_buff_.begin(), temp);
                //buffer_stream_.putback(temp);
                //buffer_stream_.putback(temp);
            }
        }
        return token_;
    }
    //if (count == offset_) {
    else {
        return std::nullopt;
    }
                
}

const Token& Lexer::CurrentToken() const {
    //throw std::logic_error("Not implemented"s);
    return token_;
}

std::optional<Token> Lexer::EOFParsing () {
     if (vector_buff_.empty()) {
        if (token_ != Token{parse::token_type::Newline {}} && token_ != Token{parse::token_type::Eof {}} 
            && token_ != Token{parse::token_type::Dedent {}}) {
            token_ = Token{parse::token_type::Newline {}};
            return token_;
        }
        token_ = Token{parse::token_type::Eof {}};
        return token_;
    }
    else {return std::nullopt;}
}

Token Lexer::NextToken() {
    char next_item;
    
     auto eof_optional = EOFParsing();
    if (eof_optional.has_value()) {
        return eof_optional.value();
    }
    
    while(!vector_buff_.empty()) {
        next_item = vector_buff_.front();
        if (next_item == '#') {
            string line;
            vector_buff_.erase(vector_buff_.begin(), std::find(vector_buff_.begin(), vector_buff_.end(), '\n') );
            continue;
        }
        if ( token_ == Token{parse::token_type::Newline {}} && next_item == '\n' ) {
            // char skip;
            // buffer_stream_.get(skip);
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            continue;
        }
        if (   (token_ == Token{parse::token_type::Newline {}} && next_item == ' ') 
            || (token_ == Token{parse::token_type::Dedent {}}  && offset_ > 0) 
            || (token_ == Token{parse::token_type::Newline {}} && next_item != ' ' && offset_ > 0)
            ) { 
                auto off = CheckIntend();
                if (off.has_value()) {
                    return off.value();
                }
                else {
                    //next_item = buffer_stream_.peek();
                    next_item = vector_buff_.front();
                }
        }

        if ((next_item > 64 && next_item < 91) || (next_item > 96 && next_item < 123) || next_item == 95) { // имя переменной, не строка
            token_ = (NameParsing());
            return token_;
            
        }

        else if (next_item > 47 && next_item < 58) { // число
            token_ = (NumberParsing());
            return token_;
            
        }

        else if (next_item == '+' || next_item == '-'|| next_item == '*'|| next_item == '/' 
                || next_item == ':' || next_item == '(' || next_item == ')' || next_item == ','|| next_item == '.') {
            char temp = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            //char temp = buffer_stream_.get();
            token_ = (Token{parse::token_type::Char {temp}});
            return token_;
            //break;
        }
        else if (next_item == '!') {
            char temp = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            //char temp = buffer_stream_.get();
            if (vector_buff_.front() != '=') {
                token_ = (Token{parse::token_type::Char {temp}});
                return token_;
            } else {
                token_ = (Token{parse::token_type::NotEq {}});
                temp = std::move(vector_buff_.front());
                if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
                //buffer_stream_.get(temp);
                return token_;
            }
        }
        else if (next_item == '=') {
            //char temp = buffer_stream_.get();
            char temp = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}

            if (vector_buff_.front() != '=' || vector_buff_.empty()) {
                token_ = (Token{parse::token_type::Char {temp}});
                return token_;
            } else {
                token_ = (Token{parse::token_type::Eq {}});
                temp = std::move(vector_buff_.front());
                if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
                //buffer_stream_.get(temp);
                return token_;
            }
        }    
        

        else if ( next_item == '>' || next_item == '<') {
            //char temp = buffer_stream_.get();
            char temp = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            if (vector_buff_.front() != '=') {
                token_ = (Token{parse::token_type::Char {temp}});
                return token_;
            } else {
                if (temp == '<') {
                    token_ = (Token{parse::token_type::LessOrEq {}});
                }
                else {
                    token_ = (Token{parse::token_type::GreaterOrEq {}});
                }
                //buffer_stream_.get(temp);
                temp = std::move(vector_buff_.front());
                if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
                return token_;
            }
            
        }

        else if (next_item == '\n') {
            //char temp;
            //buffer_stream_.get(temp);
            //char temp = std::move(vector_buff_.front());
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            token_ = (Token{parse::token_type::Newline {}});
            return token_;
            //break;
        }

        else if (vector_buff_.front() == 34 || vector_buff_.front() == 39) { // кавычки начало строки
            token_ = StringParsing ();
            return token_;
        }
        
        else {
            //char skip;
            if (!vector_buff_.empty()) {vector_buff_.erase(vector_buff_.begin());}
            //buffer_stream_.get(skip);
        }
    } // конец while
    
   

    if (vector_buff_.empty() && offset_ > 0) {
        token_ = Token{parse::token_type::Dedent {}};
        --offset_;
        return token_;
    }

    eof_optional = EOFParsing();
    if (eof_optional.has_value()) {
        return eof_optional.value();
    }

    return {};
    
}

}  // namespace parse