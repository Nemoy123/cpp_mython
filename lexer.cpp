#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <iostream>

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

Lexer::Lexer(std::istream& input)
:input_(input){
    GetNextToken();
}

const Token& Lexer::CurrentToken() const {
   return lexer_[curr_index_];
}

Token Lexer::NextToken(){
    if (static_cast<int>(lexer_.size()) > curr_index_ + 1){
        ++curr_index_;
        return lexer_[curr_index_];
    }
    else {
        GetNextToken();
        ++curr_index_;
        return lexer_[curr_index_];
    }
}

bool Lexer::GotEmptyLine(std::string input_str){
    size_t pos = input_str.find_first_not_of(' ');
    if (pos != std::string::npos && input_str[pos] != '\n' && input_str[pos] !='#'){
        return false;
    }
    return true;
}

void Lexer::ProcessTokens(std::string input_str){
    if (input_str.size() == 0){
        return;
    }
    size_t pos = input_str.find_first_not_of(' ');
    if (pos == std::string::npos){
        return;
    }
    input_str = input_str.substr(pos);
    std::string lex = ""s;
    if (input_str[0] =='#'){
        lexer_.push_back(token_type::Newline{});
        return;
    }
    else if (input_str[0]  == '=' || input_str[0] == '!' || input_str[0] == '<' || input_str[0] == '>'){
        if (input_str.size() == 1){
            lexer_.push_back(token_type::Char(input_str[0]));
        }
        else {
            lex += input_str[0];
            lex += input_str[1];
            if (lex == "=="){
                lexer_.push_back(token_type::Eq{});
                if (input_str.size()>2)
                    ProcessTokens(input_str.substr(2));
            }
            else if (lex == "<="){
                lexer_.push_back(token_type::LessOrEq{});
                if (input_str.size()>2)
                    ProcessTokens(input_str.substr(2));
            }
            else if (lex == ">="){
                lexer_.push_back(token_type::GreaterOrEq{});
                if (input_str.size()>2)
                    ProcessTokens(input_str.substr(2));
            } 
            else if (lex == "!="){
                lexer_.push_back(token_type::NotEq{});
                if (input_str.size()>2)
                    ProcessTokens(input_str.substr(2));
            }
            else {
                lexer_.push_back(token_type::Char(input_str[0]));
                ProcessTokens(input_str.substr(1));
            }
        }
    }
    else if (input_str[0] == '\n'){
        lexer_.push_back(token_type::Newline{});
        return;
    }
    else if  (std::isdigit(input_str[0])){
        pos = 0;
        while (pos <= input_str.size()-1 && std::isdigit(input_str[pos])){
            ++pos;
        }
        lexer_.push_back(token_type::Number(stoi(input_str.substr(0,pos))));
        ProcessTokens(input_str.substr(pos));
    }
    else if (std::isalpha(input_str[0])|| input_str[0]=='_'){
        pos = 0;
        while (pos <= input_str.size()-1 && (std::isalpha(input_str[pos])|| std::isdigit(input_str[pos]) || input_str[pos] == '_')){
            ++pos;
        }
        std::string lex = input_str.substr(0, pos);
        if (lex == "class"s) lexer_.push_back(token_type::Class{});
        else if (lex == "return"s) lexer_.push_back(token_type::Return{});
        else if (lex == "if"s) lexer_.push_back(token_type::If{});
        else if (lex == "else"s) lexer_.push_back(token_type::Else{});
        else if (lex == "def"s) lexer_.push_back(token_type::Def{});
        else if (lex == "None"s) lexer_.push_back(token_type::None{});
        else if (lex == "True"s) lexer_.push_back(token_type::True{});
        else if (lex == "False"s) lexer_.push_back(token_type::False{});
        else if (lex == "and"s) lexer_.push_back(token_type::And{});
        else if (lex == "or"s) lexer_.push_back(token_type::Or{});
        else if (lex == "not"s) lexer_.push_back(token_type::Not{});
        else if (lex == "print"s) lexer_.push_back(token_type::Print{});
        else lexer_.push_back(token_type::Id(lex));
        ProcessTokens(input_str.substr(pos));
    }
    else if  (input_str[0] == '\''){
        pos = 1;
        std::string result_str = ""s;
        while (pos <= input_str.size()-1 && input_str[pos] != '\''){
            if (input_str[pos] != 92){
                result_str += input_str[pos];
            }
            else{
                if (input_str[pos+1] == '\'')
                    result_str += '\'';
                if (input_str[pos+1] == '"')
                    result_str += '"';
                if  (input_str[pos+1] == 't')
                        result_str += '\t';
                if  (input_str[pos+1] == 'n')
                        result_str += '\n';
                ++pos;
            }
            ++pos;
        }
        lexer_.push_back(token_type::String(result_str));
        if (input_str.size()> pos+1)
            ProcessTokens(input_str.substr(pos+1));
    }
    else if  (input_str[0] == '\"'){
        size_t pos = 1;
        std::string result_str = ""s;
        while (pos <= input_str.size()-1 && input_str[pos] != '\"' && input_str[pos-1]!='\\'){
           if (input_str[pos] != 92){
                result_str += input_str[pos];
            }
            else{
                if (input_str[pos+1] == '\'')
                    result_str += '\'';
                if (input_str[pos+1] == '"')
                    result_str += '"';
                if  (input_str[pos+1] == 't')
                        result_str += '\t';
                if  (input_str[pos+1] == 'n')
                        result_str += '\n';
                ++pos;
            }
            ++pos;
        }
        lexer_.push_back(token_type::String(result_str));
        if (input_str.size()> pos+1)
            ProcessTokens(input_str.substr(pos+1));
    }
    else{
        lexer_.push_back(token_type::Char(input_str[0]));
        if (input_str.size()>1) {
            ProcessTokens(input_str.substr(1));
        }
    }
}
    
void Lexer::GetNextToken() {
    if (!input_){
        lexer_.push_back(token_type::Eof{});
    }
    std::string input_str = "";
    while (input_ && GotEmptyLine(input_str)){
       input_str = "";
       char ch;
        while (input_){
            if (input_.get(ch)) input_str += ch;
            if (ch == '\n') break;
        }
    }
    if (!input_ && GotEmptyLine(input_str)){
        if (indent_ > 0){
            for (int i=0; i < indent_; ++i){
                lexer_.push_back(token_type::Dedent{});
            }
        }
        lexer_.push_back(token_type::Eof{});
        return;
    }

    size_t pos = input_str.find_first_not_of(' ');
    if (pos % 2 == 1){
        throw LexerError("Wrong indent"s);
    }
    int new_indent = pos / 2;
    if (new_indent > indent_){
        for (int i=0; i < new_indent - indent_; ++i){
            lexer_.push_back(token_type::Indent{});
        }
        indent_ = new_indent;
    }
    if (new_indent < indent_){
        for (int i=0; i < indent_ - new_indent; ++i){
            lexer_.push_back(token_type::Dedent{});
        }
        indent_ = new_indent;
    } 
    input_str = input_str.substr(pos);
    if (input_str.back() != '\n'){
        input_str += '\n';
    }
    ProcessTokens(input_str);
}

}  // namespace parse