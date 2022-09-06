#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

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
    // Проверяет, содержится ли в object значение, приводимое к True
    // Для отличных от нуля чисел, True и непустых строк возвращается true. В остальных случаях - false.

//    using String = ValueObject<std::string>;
//    // Числовое значение
//    using Number = ValueObject<int>;

//    // Логическое значение
//    class Bool : public ValueObject<bool> {



    // нет проверки на None

    if(object.TryAs<String>()){
       if(object.TryAs<String>()->GetValue().size()>0){
           return true;
       }
    }
    if(object.TryAs<Number>()){
       if(object.TryAs<Number>()->GetValue()!=0){
           return true;
       }
    }
    if(object.TryAs<Bool>()){
       if(object.TryAs<Bool>()->GetValue()){
           return true;
       }
    }
    return false;
}

void ClassInstance::Print(std::ostream& os,Context& context) {
    if(cls_.GetMethod("__str__")){
       cls_.GetMethod("__str__")->body->Execute(this->Fields(),context).Get()->Print(os,context);
    }else{
        os<<this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    if(cls_.GetMethod(method) && cls_.GetMethod(method)->formal_params.size()==argument_count){
        return true;
    }
    return false;
}

Closure& ClassInstance::Fields() {
    return table_;
    //throw std::logic_error("Not implemented"s);
}

const Closure& ClassInstance::Fields() const {
    return table_;
}

ClassInstance::ClassInstance(const Class& cls):cls_(cls){


}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) {
    Closure temp;
    if(HasMethod(method,actual_args.size())){
        size_t i=0;
        for (const auto &it: cls_.GetMethod(method)->formal_params){
            temp.operator[](it)=actual_args[i];
            ++i;
        }
        temp.operator[]("self")=ObjectHolder::Share(*this);
    }else{

        throw std::runtime_error("error Call");
    }

    return cls_.GetMethod(method)->body->Execute(temp,context);

}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent):methods_(std::move(methods)),name_(name),parent_(parent) {


}

const Method* Class::GetMethod(const std::string& name) const {
    if(parent_){
        for(auto& it: methods_){
            if(name==it.name){
                return &it;
            }
        }
        return parent_->GetMethod(name);
    }else{
        for(auto& it: methods_){
            if(name==it.name){
                return &it;
            }
        }
    }
    return nullptr;
}

[[nodiscard]]  const std::string& Class::GetName() const {
    return name_;
}



void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
    os <<"Class "<<this->GetName();
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if(lhs.TryAs<Number>() && rhs.TryAs<Number>()){
        return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
    }
    if(lhs.TryAs<String>() && rhs.TryAs<String>()){
        return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
    }
    if(lhs.TryAs<Bool>() && rhs.TryAs<Bool>()){
        return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
    }

    if(lhs.Get()==nullptr && rhs.Get()==nullptr){
         return true;
    }

    if(lhs.Get())
    if(lhs.TryAs<ClassInstance>())
    if(lhs.TryAs<ClassInstance>()->HasMethod("__eq__",1)){
        std::vector arg{rhs};
        bool T=IsTrue(lhs.TryAs<ClassInstance>()->Call("__eq__",arg,context));
        return T;
    }
    throw std::runtime_error("Cannot compare objects for eq"s);

}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if(lhs.TryAs<Number>() && rhs.TryAs<Number>()){
        return lhs.TryAs<Number>()->GetValue()< rhs.TryAs<Number>()->GetValue();
    }
    if(lhs.TryAs<String>() && rhs.TryAs<String>()){
        return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
    }
    if(lhs.TryAs<Bool>() && rhs.TryAs<Bool>()){
        return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
    }

    if(lhs.Get()==nullptr || rhs.Get()==nullptr){
         throw std::runtime_error("Cannot compare objects for less"s);
    }
    if(lhs.Get())
    if(lhs.TryAs<ClassInstance>())
    if(lhs.TryAs<ClassInstance>()->HasMethod("__lt__",1)){
        std::vector arg{rhs};
        bool T=IsTrue(lhs.TryAs<ClassInstance>()->Call("__lt__",arg,context));
        return T;
    }
    throw std::runtime_error("Cannot compare objects for less"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {

    return !Equal(lhs,rhs,context);
    throw std::runtime_error("Cannot compare objects for equality"s);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if(lhs.Get()==nullptr && rhs.Get()==nullptr){
         throw std::runtime_error("Cannot compare objects for less"s);
    }
    if(NotEqual(lhs,rhs,context)){
        if(!Less(lhs,rhs,context)){
            return true;
        }
    }
    return false;
    throw std::runtime_error("Cannot compare objects for equality"s);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if(lhs.Get()==nullptr && rhs.Get()==nullptr){
         throw std::runtime_error("Cannot compare objects for less"s);
    }
    if(Equal(lhs,rhs,context) || Less(lhs,rhs,context) )
        return true;
    return false;
    throw std::runtime_error("Cannot compare objects for equality"s);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if(lhs.Get()==nullptr && rhs.Get()==nullptr){
         throw std::runtime_error("Cannot compare objects for less"s);
    }
    return !Less(lhs,rhs,context);
    throw std::runtime_error("Cannot compare objects for equality"s);
}

}  // namespace runtime
