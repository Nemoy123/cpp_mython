#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace 

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_] = rv_.get()->Execute(closure, context);
    return closure[var_];
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
                        : var_(var)
                        , rv_(std::move(rv)) 
                        {}

VariableValue::VariableValue(const std::string& var_name) {
    dot_.push_back(var_name);
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
                            : dot_(std::move(dotted_ids) )
                            {}
    

ObjectHolder VariableValue::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    

    // ObjectHolder result;
    // bool begin = true;
    // //std::vector<runtime::ObjectHolder> arg;
    // for (const auto& name : dot_) {
    //     if (begin) {
            
    //         auto find_var = closure.find(name);
    //         if (find_var != closure.end()) {
    //             result = find_var->second;
    //             begin = false; 
                
    //             continue;

    //         }
    //         else {
    //             if (closure.find("self") != closure.end()) {
    //                 if (closure["self"].TryAs<runtime::ClassInstance>()) {
    //                     auto find_var2 = closure["self"].TryAs<runtime::ClassInstance>()->Fields().find(name);
    //                     if (find_var2 != closure["self"].TryAs<runtime::ClassInstance>()->Fields().end()) {
    //                         result = find_var2->second;
    //                         begin = false;
    //                         continue;
    //                     }
    //                     else {
    //                         throw std::runtime_error ("VariableValue Error");
    //                     }
    //                 }
    //                 else {
    //                     throw std::runtime_error ("VariableValue Error");
    //                 }
    //             } else {
    //                 throw std::runtime_error ("VariableValue Error");
    //             }
    //         }
    //     }
       
    //     if ( result.TryAs<runtime::ClassInstance>() ) {
    //         if (result.TryAs<runtime::ClassInstance>()->HasMethod(name, 0)) {
    //             auto temp = result.TryAs<runtime::ClassInstance>()->Call(name, {}, context);
    //             result = std::move(temp);
    //             continue;
    //         }
    //     }
    //     throw std::runtime_error ("VariableValue Error");
    // }
    // return result;
    size_t counter = 0;
    Closure* temp_map = &closure;
    while (counter < dot_.size()-1){
        if (((temp_map)->count(dot_[counter]) == 0) || (!(temp_map)->at(dot_[counter]).TryAs<runtime::ClassInstance>())){
            throw runtime_error("VariableValue Error");
        }
        temp_map = &((temp_map)->at(dot_[counter]).TryAs<runtime::ClassInstance>()->Fields());
        ++counter;
    }
    if ((temp_map)->count(dot_[counter]) == 0){
        if (closure.find("self") != closure.end()) {
            if (closure["self"].TryAs<runtime::ClassInstance>()) {
                auto name = dot_.at(0);
                // auto find_var2 = closure["self"].TryAs<runtime::ClassInstance>()->Fields().find(name);
                // if (find_var2 != closure["self"].TryAs<runtime::ClassInstance>()->Fields().end()) {
                   return closure["self"];
                // }
                // else {
                //     throw std::runtime_error ("VariableValue Error");
                // }
            }
            else {
                throw std::runtime_error ("VariableValue Error");
            }
        } else {
            throw std::runtime_error ("VariableValue Error");
        }
    }
    return (*temp_map).at(dot_[counter]);
} 

unique_ptr<Print> Print::Variable(const std::string& name) {
    
    return make_unique<Print>(make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> arg) 
            //: args_(std::vector{std::move(arg)}) 
            {
                args_.push_back(std::move(arg));
            }

Print::Print(vector<unique_ptr<Statement>> args) 
            : args_(std::move(args)) 
            {}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    ObjectHolder result;
    bool begin = true;
    for (const auto& name : args_) {
        if (name.get() == nullptr) {
            context.GetOutputStream() << "None"s;
        }
        result = name.get()->Execute(closure, context);
        if (!begin) {context.GetOutputStream() << " "s;}
        if (!result) {
            context.GetOutputStream() << "None"s;
        }
        
        else if (result.TryAs<runtime::String>()) {
            result.TryAs<runtime::String>()->Print(context.GetOutputStream(), context);
        }
        else if (result.TryAs<runtime::Number>()) {
            result.TryAs<runtime::Number>()->Print(context.GetOutputStream(), context);
            
        }
        else if (result.TryAs<runtime::ClassInstance>()) {
            result.TryAs<runtime::ClassInstance>()->Print(context.GetOutputStream(), context);
        }
        else if (result.TryAs<runtime::Bool>()) {
            result.TryAs<runtime::Bool>()->Print(context.GetOutputStream(), context);
        }
        begin = false;
    }
    context.GetOutputStream() << std::endl;
    return result;
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args) 
                       : object_(std::move(object))
                       , method_(std::move(method))
                       , args_(std::move(args))
                       {}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    auto holder = object_.get()->Execute(closure, context);
    if (holder.TryAs<runtime::ClassInstance>()->HasMethod(method_, args_.size())) {
        std::vector<ObjectHolder> vec;
        for (size_t i = 0; i < args_.size(); ++i) { 
           vec.push_back( args_[i].get()->Execute(closure, context) );
        }
        return holder.TryAs<runtime::ClassInstance>()->Call(method_, vec, context);
    }
    else {
        return {};
        //throw std::runtime_error ("MethodCall Error");
    }
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    
    auto holder = GetArgument ()->Execute(closure, context);
    
    stringstream out;
    
    if (holder) {
        holder.Get()->Print(out, context);
    }
    else {
        out << "None";
    }
    return ObjectHolder::Own(runtime::String(out.str()));
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    auto holder_lhs = GetArgumentLhs ()->Execute(closure, context);
    auto holder_rhs = GetArgumentRhs ()->Execute(closure, context);
    if (holder_lhs.TryAs<runtime::Number>() && holder_rhs.TryAs<runtime::Number>()) {
       auto res = holder_lhs.TryAs<runtime::Number>()->GetValue() + holder_rhs.TryAs<runtime::Number>()->GetValue();
       return ObjectHolder::Own(runtime::Number(res));
    }
    else if (holder_lhs.TryAs<runtime::String>() && holder_rhs.TryAs<runtime::String>()) {
       auto res = holder_lhs.TryAs<runtime::String>()->GetValue() + holder_rhs.TryAs<runtime::String>()->GetValue();
       return ObjectHolder::Own(runtime::String(res));
    }
    else if (auto obj = holder_lhs.TryAs<runtime::ClassInstance>()) {
        if (obj->HasMethod("__add__"s, 1)) {
            return obj->Call("__add__"s, vector{holder_rhs}, context);
        }
        else {
            throw std::runtime_error ("ADD ERROR");
        }
    }
    else {
            throw std::runtime_error ("ADD ERROR");
    }
    
    //return {};
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    auto holder_lhs = GetArgumentLhs ()->Execute(closure, context);
    auto holder_rhs = GetArgumentRhs ()->Execute(closure, context);
    if (holder_lhs.TryAs<runtime::Number>() && holder_rhs.TryAs<runtime::Number>()) {
       auto res = holder_lhs.TryAs<runtime::Number>()->GetValue() - holder_rhs.TryAs<runtime::Number>()->GetValue();
       return ObjectHolder::Own(runtime::Number(res));    
    }
    else {
        throw std::runtime_error ("SUB ERROR");
    }
    
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    auto holder_lhs = GetArgumentLhs ()->Execute(closure, context);
    auto holder_rhs = GetArgumentRhs ()->Execute(closure, context);
    if (holder_lhs.TryAs<runtime::Number>() && holder_rhs.TryAs<runtime::Number>()) {
       auto res = holder_lhs.TryAs<runtime::Number>()->GetValue() * holder_rhs.TryAs<runtime::Number>()->GetValue();
       return ObjectHolder::Own(runtime::Number(res));    
    }
    else {
        throw std::runtime_error ("SUB ERROR");
    }
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    auto holder_lhs = GetArgumentLhs ()->Execute(closure, context);
    auto holder_rhs = GetArgumentRhs ()->Execute(closure, context);
    if (holder_lhs.TryAs<runtime::Number>() && holder_rhs.TryAs<runtime::Number>()) {
        if (holder_rhs.TryAs<runtime::Number>()->GetValue() == 0) {
            throw std::runtime_error ("DIV ERROR");
        }
       auto res = holder_lhs.TryAs<runtime::Number>()->GetValue() / holder_rhs.TryAs<runtime::Number>()->GetValue();
       return ObjectHolder::Own(runtime::Number(res));    
    }
    else {
        throw std::runtime_error ("DIV ERROR");
    }
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (auto& arg : args_) {
        arg.get()->Execute(closure, context);
    }

    return ObjectHolder {};
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    auto result = statement_.get()->Execute(closure, context);
    throw result;
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
:cls_(std::move(cls)){}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]]Context& context) {
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return cls_;
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv) 
                                 : object_(std::move(object))
                                 , field_name_(std::move(field_name))
                                 , rv_(std::move(rv))
                                 {}

void PrintMapKey (Closure& closure) {
    for (const auto& [name, ptr]:closure) {
                        std::cout << name << "-";
                    }
    std::cout << endl;
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    ObjectHolder result;
        auto obj = object_.Execute(closure, context);
        auto obj_holder_closure = (rv_.get()->Execute(closure, context));
        (closure)[field_name_] =  obj_holder_closure;   
        result = obj_holder_closure;           

        if (obj.TryAs<runtime::ClassInstance>()) {
            runtime::ClassInstance* cl_in = obj.TryAs<runtime::ClassInstance>();
            runtime::Closure* closure_map = &cl_in->Fields();
             
            (*closure_map)[field_name_] = obj_holder_closure;
            if (!((*closure_map)[field_name_]))  {
                cout << " ERRRRRRRRRRRoRRRRRRRR";
            }
        }

    return result;
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body)
:condition_(std::move(condition))
, if_body_(std::move(if_body))
, else_body_(std::move(else_body)) {}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    auto holder = condition_->Execute(closure, context);
    if (holder.TryAs<runtime::Bool>()) {
        if (holder.TryAs<runtime::Bool>()->GetValue() == true){
            return if_body_->Execute(closure, context);
        }
        else {
            if (else_body_.get()) {
                return else_body_->Execute(closure, context);
            } 
            return {};  
        }
    }
    else {
        
        throw runtime_error ("IfElse error");
    }
    
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    auto holder_lhs = GetArgumentLhs ()->Execute(closure, context);
    auto holder_rhs = GetArgumentRhs ()->Execute(closure, context);
    if (auto ptr_l = holder_lhs.TryAs<runtime::Bool>()) {
        if (ptr_l->GetValue() == true) {
            return ObjectHolder::Own(runtime::Bool(true));
        }
        else {
            if (auto ptr_r = holder_rhs.TryAs<runtime::Bool>()) {
                if (ptr_r->GetValue() == true ) {
                    return ObjectHolder::Own(runtime::Bool(true));
                }
                else {
                    return ObjectHolder::Own(runtime::Bool(false));
                }
            }
            else {
                throw std::runtime_error ("OR ERROR");
            }
        }
    }
    else {
        throw std::runtime_error ("OR ERROR");
    }
    return {};
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    auto holder_lhs = GetArgumentLhs ()->Execute(closure, context);
    auto holder_rhs = GetArgumentRhs ()->Execute(closure, context);
    if (auto ptr_l = holder_lhs.TryAs<runtime::Bool>()) {
        if (ptr_l->GetValue() != true) {
            return ObjectHolder::Own(runtime::Bool(false));
        }
        else {
            if (auto ptr_r = holder_rhs.TryAs<runtime::Bool>()) {
                if (ptr_r->GetValue() == true ) {
                    return ObjectHolder::Own(runtime::Bool(true));
                }
                else {
                    return ObjectHolder::Own(runtime::Bool(false));
                }
            }
        }
    }
    else {
        throw std::runtime_error ("AND ERROR");
    }
    return {};
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    auto holder = GetArgument()->Execute(closure, context);
    if (auto ptr_l = holder.TryAs<runtime::Bool>()) { 
        return ObjectHolder::Own(runtime::Bool(!ptr_l->GetValue()));
    }
    else {
        throw std::runtime_error ("NOT ERROR");
    }
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
                        : BinaryOperation(std::move(lhs), std::move(rhs))
                        , cmp_(std::move(cmp))  
                        {}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    auto lhs_holder = GetArgumentLhs()->Execute(closure, context);
    auto rhs_holder = GetArgumentRhs()->Execute(closure, context);
    bool result = cmp_(lhs_holder, rhs_holder, context);

    return ObjectHolder::Own(runtime::Bool(result));
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
                  : class_first_ptr_ (&class_)
                  , class_new_ptr_(make_unique<runtime::ClassInstance> (class_))
                  , args_(std::move(args))
                  {}

NewInstance::NewInstance(const runtime::Class& class_) 
                : class_first_ptr_ (&class_)
                , class_new_ptr_(make_unique<runtime::ClassInstance> (class_))
                {}


ObjectHolder NewInstance::Execute([[maybe_unused]]Closure& closure, Context& context) {
    if (class_new_ptr_) {
        if (class_new_ptr_.get()->HasMethod("__init__", args_.size())) {
            ObjectHolder result;
            std::vector<ObjectHolder> obj_holders {};
            for (auto& arg : args_) {
                auto obj = arg.get()->Execute(closure, context);
                obj_holders.push_back(obj);
            }
            result = class_new_ptr_.get()->Call("__init__", obj_holders, context); 
        }
    }
    return ObjectHolder::Share ( *(class_new_ptr_.release()) );
}

// ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
//     if (class_new_ptr_) {
//         if (class_new_ptr_.get()->HasMethod("__init__", args_.size())) {
//             ObjectHolder result;
//             std::vector<ObjectHolder> obj_holders;
//             for (auto& arg : args_) {
//                 auto obj = arg.get()->Execute(class_new_ptr_.get()->Fields(), context);
//                 obj_holders.push_back(obj);
//             }
//             result = class_new_ptr_.get()->Call("__init__", obj_holders, context);    
//         }
//     }
//     //runtime::ClassInstance* ptr = class_new_ptr_.release();
//     //result = ObjectHolder::Own (*(class_new_ptr_.release()));
//     return ObjectHolder::Share ( *(class_new_ptr_.release()) );
// }

MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
                        : body_(std::move(body))
                        {}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_.get()->Execute(closure, context);
    } catch (const ObjectHolder result) { //} catch (const ObjectHolder* result) {
        return result;
    }
    return {};
}

}  // namespace ast