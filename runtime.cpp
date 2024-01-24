#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <algorithm>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}


bool IsTrue(const ObjectHolder& object) {
    if (auto ptr = object.TryAs<Bool>()) {
        if (ptr->GetValue() != 0) {return true;}
        else {return false;}
    }
    if (auto ptr = object.TryAs<String>()) {
        if (ptr->GetValue().size() != 0) {return true;}
        else {return false;}
    }
    if (auto ptr = object.TryAs<Number>()) {
        if (ptr->GetValue() != 0) {return true;}
        else {return false;}
    }
    
    return false;
}

void ClassInstance::Print(std::ostream& os, Context& context) {
    if (HasMethod("__str__", 0)){
        auto temp = ptr_cls_->GetMethod("__str__")->body.get()->Execute(map_, context);
        temp.Get()->Print(os, context);
    }
    else {
        
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    if (ptr_cls_) {
        if ( auto met = ptr_cls_->GetMethod(method)) {
            return met->formal_params.size() == argument_count;
        }
    }
    return false;
}

Closure& ClassInstance::Fields() {
    return map_;
}

const Closure& ClassInstance::Fields() const {
    return map_;
}

ClassInstance::ClassInstance(const Class& cls)
                            : ptr_cls_(&cls)
                            {
                               map_["self"s] = ObjectHolder::Share(*this);
                               
                            }

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) {
    //auto hold_this = ObjectHolder::Share(*this);
    if (HasMethod(method, actual_args.size())) {
        auto met = ptr_cls_->GetMethod(method);
        Closure map_for_method;
        map_for_method["self"] = map_["self"];
        for (size_t i = 0; i < actual_args.size(); ++i) { 
            map_[met->formal_params[i]] = actual_args[i];
            map_for_method[met->formal_params[i]] = actual_args[i];
        }
        
        return met->body->Execute(map_for_method, context);
    } 
    else {                              
        throw std::runtime_error("Not Call"s);
    } 
    return {}; 
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent) 
            : name_(std::move(name))
            , methods_(std::move(methods))
            , parent_(parent)
            {
                // GetHoldThis ();
            }
Class::Class(const Class& other)
            : name_(other.name_)
            , methods_(std::move(other.methods_))
            , parent_(other.parent_)  
             {

                // for (const auto& met : other.methods_) {
                //     runtime::Method method;
                //     method.body = make_unique <runtime::Executable> (*(met.body.get()));
                //     method.name = met.name;
                //     method.formal_params = met.formal_params;
                // }
             }

// template<typename T>
// bool CheckPtrValidate(T * ptr)
// {
//     bool result;
//     try
//     {
//         if(NULL != dynamic_cast<T*>(ptr))
//             result = true;
//         else
//             result = false;
//     }
//     catch (...)
//     {
//         result = false;
//     }
//     return result;
// }

const Method* Class::GetMethod(const std::string& name) const {


    Method find_met;
    find_met.name = name;
    auto iter = std::find (methods_.begin(), methods_.end(), find_met );
   if (iter != methods_.end()) {
        return &(*iter);
   }

    else {
        if (parent_) {
            //if (CheckPtrValidate (parent_)) {
                return parent_->GetMethod(name);
            //}
        }
    }

    return nullptr;
    
}

// [[nodiscard]] inline const std::string& Class::GetName() const { //inline 
//     return name_;
// }

void Class::Print(ostream& os, Context& /*context*/) {
    // Выводит в os строку "Class <имя класса>", например "Class cat"
    os << "Class "s << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    //if (lhs.Get() != nullptr && rhs.Get() != nullptr) {
        if (!lhs && !rhs) {
            return true;
        }
        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
            return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
        }
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
            return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
        }
        if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
            return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
        }
        if (lhs.TryAs<ClassInstance>()) {
            runtime::ClassInstance* class_ptr = lhs.TryAs<ClassInstance>();
            if (class_ptr->HasMethod("__eq__"s, 1)) {
                runtime::ObjectHolder temp = lhs.TryAs<ClassInstance>()->Call("__eq__", (std::vector{rhs}), context) ;
                runtime::Bool* temp_ptr = temp.TryAs<Bool>();
                return temp_ptr->GetValue();  
            }
        }
        
   // }
    throw std::runtime_error("Not Equal"s);
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
            return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
    }
    else if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
        return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
    }
    else if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
        return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
    }
    else if (lhs.TryAs<ClassInstance>()) {
        runtime::ClassInstance* class_ptr = lhs.TryAs<ClassInstance>();
        if (class_ptr->HasMethod("__lt__"s, 1)) {
            runtime::ObjectHolder temp = lhs.TryAs<ClassInstance>()->Call("__lt__", (std::vector{rhs}), context) ;
            runtime::Bool* temp_ptr = temp.TryAs<Bool>();
            return temp_ptr->GetValue();  
        }
    }
    else {
        throw std::runtime_error("Cannot compare objects for less"s);
        
    }
    
    return false;
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context) && NotEqual(lhs, rhs, context);
    //throw std::runtime_error("Cannot compare objects for equality"s);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Greater(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime