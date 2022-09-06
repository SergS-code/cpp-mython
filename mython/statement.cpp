#include "statement.h"
#include "test_runner_p.h"
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

ObjectHolder Assignment::Execute([[maybe_unused]] Closure& closure,[[maybe_unused]] Context& context) {
    ObjectHolder temp=rv_->Execute(closure,context);
    closure[var_]=temp;
    return temp;
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv):var_(var),rv_(std::move(rv)) {
}

VariableValue::VariableValue(const std::string& var_name):var_name_(var_name) {
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids):dotted_ids_(dotted_ids) {

}

ObjectHolder VariableValue::Execute(Closure& closure,[[maybe_unused]] Context& context) {

    size_t sizeDotteds = dotted_ids_.size();
    Closure* newClosure = &closure;
    if(sizeDotteds > 0) {
        std::string last_name = dotted_ids_[sizeDotteds - 1];
        for (auto& name : dotted_ids_) {
            if (newClosure->find(name) != newClosure->end()) {
                if(name == last_name) {
                    return newClosure->at(name);
                }
                newClosure = &newClosure->find(name)->second.TryAs<runtime::ClassInstance>()->Fields();
            }
        }
    }
    else if(closure.count(var_name_)) {
        return runtime::ObjectHolder::Share(*closure.at(var_name_).Get());
    }
    throw std::runtime_error("Var not found"s);
    return ObjectHolder::None();


}

unique_ptr<Print> Print::Variable(const std::string& name) {
    auto it=make_unique<VariableValue>(name);
    auto p = std::make_unique<Print>(std::move(it));
    return p;
}

Print::Print(unique_ptr<Statement> argument):argument_(std::move(argument)){

}

Print::Print(vector<unique_ptr<Statement>> args):args_(std::move(args)) {

}

ObjectHolder Print::Execute(Closure& closure, Context& context) {

    if(argument_.get()!=nullptr && args_.size()==0){
        args_.push_back(std::move(argument_));
    }

    std::ostream& os = context.GetOutputStream();
    if(args_.size()!=0){
        int i=0;
        for (auto& it : args_){
            auto item=it.get()->Execute(closure, context);
            if(i>0){
                os <<" ";
                if (item.operator bool()!=true){
                    os <<"None";
                }else{
                    item->Print(os,context);
                }
            }else{
                if (item.operator bool()!=true){
                    os <<"None";
                }else{
                    item->Print(os,context);
                }
            }

            ++i;
        }
        os << '\n';
    }
    else{
        os << '\n';
    }
    if(argument_.get()!=nullptr){
        ObjectHolder objectHold = argument_.get()->Execute(closure, context);
        objectHold->Print(os,context);
        os << '\n';
    }
    return {};
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args): object_(std::move(object)),method_(method),args_(std::move(args)){

}
// Вызывает метод object.method со списком параметров args
ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    auto it1= object_.get()->Execute(closure,context);
    auto cl=it1.TryAs<runtime::ClassInstance>();
    if(cl){
         if (cl->HasMethod(method_, args_.size())) {
             std::vector<ObjectHolder> args;
             for (auto&& argument : args_) {
                 args.push_back(argument.get()->Execute(closure, context));
             }
             return cl->Call(method_, args, context);
         }
         else {
             return ObjectHolder::Own(std::move(*cl));
         }
    }else{
        return {};
     }

     throw std::runtime_error("Var  not MethodCall found"s);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {

    if(argument_.get() == nullptr) {
        return runtime::ObjectHolder::None();
    }
    else {
        std::ostringstream os;
        auto it =argument_.get()->Execute(closure, context).Get();

        if(it){
            argument_.get()->Execute(closure, context).Get()->Print(os,context);
            std::string str = os.str();
            runtime::String s(str);
            ObjectHolder t=ObjectHolder::Own(std::move(s));
            ASSERT(t.TryAs<runtime::String>());
            return t;

        }else{
            std::string str = "None";
            runtime::String s(str);
            ObjectHolder t=ObjectHolder::Own(std::move(s));
            ASSERT(t.TryAs<runtime::String>());
            return t;

        }


    }
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    auto it1 =lhs_.get()->Execute(closure, context).Get();
    auto it2 =rhs_.get()->Execute(closure, context).Get();

    if(it1 == nullptr || it2==nullptr) {
        throw runtime_error("Add");
    }

    if(lhs_.get() == nullptr || rhs_.get()==nullptr) {
        throw runtime_error("Add");
    }
    else {
        ObjectHolder left=lhs_.get()->Execute(closure, context);
        ObjectHolder right=rhs_.get()->Execute(closure, context);

        if(left.TryAs<runtime::Number>()){
            if(right.TryAs<runtime::Number>()){
                auto l=left.TryAs<runtime::Number>();
                auto r=right.TryAs<runtime::Number>();
                runtime::Number s(l->GetValue() + r->GetValue());
                ObjectHolder rez=ObjectHolder::Own(std::move(s));
                return rez;
            }
            else{
                throw runtime_error("Add right");
            }
        }
        if(left.TryAs<runtime::String>()){
            if(right.TryAs<runtime::String>()){
                auto l=left.TryAs<runtime::String>();
                auto r=right.TryAs<runtime::String>();
                runtime::String s(l->GetValue() + r->GetValue());
                ObjectHolder rez=ObjectHolder::Own(std::move(s));
                return rez;
            }
            else{
                throw runtime_error("Add right");
            }
        }

        if(left.TryAs<runtime::ClassInstance>()){
            auto leftCls=left.TryAs<runtime::ClassInstance>();
            if (leftCls->HasMethod(ADD_METHOD, 1)) {
                std::vector<ObjectHolder> args;
                args.push_back(right);
                return leftCls->Call(ADD_METHOD, args, context);
            }
        }

    }
    throw runtime_error("Add right");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    auto it1 =lhs_.get()->Execute(closure, context).Get();
    auto it2 =rhs_.get()->Execute(closure, context).Get();

    if(it1 == nullptr || it2==nullptr) {
        throw runtime_error("Add");
    }

    if(lhs_.get() == nullptr || rhs_.get()==nullptr) {
        throw runtime_error("Add");
    }
    else {
        ObjectHolder left=lhs_.get()->Execute(closure, context);
        ObjectHolder right=rhs_.get()->Execute(closure, context);

        if(left.TryAs<runtime::Number>()){
            if(right.TryAs<runtime::Number>()){
                auto l=left.TryAs<runtime::Number>();
                auto r=right.TryAs<runtime::Number>();
                runtime::Number s(l->GetValue() - r->GetValue());
                ObjectHolder rez=ObjectHolder::Own(std::move(s));
                return rez;
            }
            else{
                throw runtime_error("Add right");
            }
        }
    }
    throw runtime_error("Add right");
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    auto it1 =lhs_.get()->Execute(closure, context).Get();
    auto it2 =rhs_.get()->Execute(closure, context).Get();

    if(it1 == nullptr || it2==nullptr) {
        throw runtime_error("Add");
    }

    if(lhs_.get() == nullptr || rhs_.get()==nullptr) {
        throw runtime_error("Add");
    }
    else {
        ObjectHolder left=lhs_.get()->Execute(closure, context);
        ObjectHolder right=rhs_.get()->Execute(closure, context);

        if(left.TryAs<runtime::Number>()){
            if(right.TryAs<runtime::Number>()){
                auto l=left.TryAs<runtime::Number>();
                auto r=right.TryAs<runtime::Number>();
                runtime::Number s(l->GetValue() * r->GetValue());
                ObjectHolder rez=ObjectHolder::Own(std::move(s));
                return rez;
            }
            else{
                throw runtime_error("Add right");
            }
        }
    }
    throw runtime_error("Add right");
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    auto it1 =lhs_.get()->Execute(closure, context).Get();
    auto it2 =rhs_.get()->Execute(closure, context).Get();

    if(it1 == nullptr || it2==nullptr) {
        throw runtime_error("Add");
    }

    if(lhs_.get() == nullptr || rhs_.get()==nullptr) {
        throw runtime_error("Add");
    }
    else {
        ObjectHolder left=lhs_.get()->Execute(closure, context);
        ObjectHolder right=rhs_.get()->Execute(closure, context);

        if(left.TryAs<runtime::Number>()){
            if(right.TryAs<runtime::Number>()){
                auto l=left.TryAs<runtime::Number>();
                auto r=right.TryAs<runtime::Number>();
                if(r->GetValue()==0){
                    throw runtime_error("Add right");
                }
                runtime::Number s(l->GetValue() / r->GetValue());
                ObjectHolder rez=ObjectHolder::Own(std::move(s));
                return rez;
            }
            else{
                throw runtime_error("Add right");
            }
        }
    }
    throw runtime_error("Add right");
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
   for(auto& it: args_){
       it.get()->Execute(closure,context);
   }
   return {};
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_.get()->Execute(closure,context);

}

ClassDefinition::ClassDefinition(ObjectHolder cls):cls_(cls) {

}

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    auto item=cls_.TryAs<runtime::Class>();
    std::string name=item->GetName();
    closure[name]=cls_;
    return {};
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv):object_(object),field_name_(field_name),rv_(std::move(rv)) {
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {

    size_t sizeDotteds = object_.dotted_ids_.size();
      Closure* newCloser = &closure;
      if(sizeDotteds> 0) {
          std::string last_name = object_.dotted_ids_[sizeDotteds-1];
          for (auto& name : object_.dotted_ids_) {
              if (newCloser->find(name) != newCloser->end()) {
                  newCloser = &newCloser->find(name)->second.TryAs<runtime::ClassInstance>()->Fields();
              }
          }
          newCloser->operator[](field_name_)=rv_->Execute(closure, context);
          return newCloser->at(field_name_);
      }
      else if(newCloser->find(object_.var_name_) != newCloser->end()) {
          newCloser = &newCloser->at(object_.var_name_).TryAs<runtime::ClassInstance>()->Fields();
          newCloser->operator[](field_name_) = rv_->Execute(closure, context);
          return newCloser->at(field_name_);
      }
      std::runtime_error("FieldAssignment ");
      return {};
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body) :condition_(std::move(condition)),if_body_(std::move(if_body)),else_body_(std::move(else_body)){

}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    // Инструкция if <condition> <if_body> else <else_body>
    auto  holdObject = condition_->Execute(closure, context);
        if(holdObject.TryAs<runtime::Bool>()->GetValue()) {
            try {
                if_body_->Execute(closure, context);
            }
            catch(ObjectHolder holdObject) {
                throw holdObject;
            }
        }
        else {
            try {
                if(else_body_.get() != nullptr) {
                    else_body_->Execute(closure, context);
                }
            }
            catch(ObjectHolder holdObject) {
                throw holdObject;
            }
        }
        return {};

}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    auto left =lhs_.get()->Execute(closure, context);
    auto right =rhs_.get()->Execute(closure, context);
    // Значение аргумента rhs вычисляется, только если значение lhs
    // после приведения к Bool равно False

    if(left.TryAs<runtime::Bool>()){
        bool leftB= left.TryAs<runtime::Bool>()->GetValue();
        if(leftB){
            return left;
        }else{
            return right;
        }
    }
    throw runtime_error("Bool or");

}

ObjectHolder And::Execute(Closure& closure, Context& context) {

    auto left =lhs_.get()->Execute(closure, context);
    auto right =rhs_.get()->Execute(closure, context);
    // Значение аргумента rhs вычисляется, только если значение lhs
    // после приведения к Bool равно True

    if(left.TryAs<runtime::Bool>()){
        bool leftB= left.TryAs<runtime::Bool>()->GetValue();
        if(leftB){
            return right;
        }else{
            return left;
        }
    }
    throw runtime_error("Bool And");
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    auto it = argument_.get()->Execute(closure,context);
    if(it.TryAs<runtime::Bool>()){
        bool leftB= it.TryAs<runtime::Bool>()->GetValue();
        if(leftB){
            auto b= runtime::Bool(false);
            ObjectHolder rez(ObjectHolder::Own(std::move(b)));
            return rez;
        }else{
            auto b= runtime::Bool(true);
            ObjectHolder rez(ObjectHolder::Own(std::move(b)));
            return rez;
        }
    }
    throw runtime_error("Bool Not");

}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)) {
    cmp_=cmp;

}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
   auto it1=lhs_.get()->Execute(closure,context);
   auto it2=rhs_.get()->Execute(closure,context);
   auto cm=cmp_.operator()(it1,it2,context);
   runtime::Bool rez(cm);
   ObjectHolder reze(ObjectHolder::Own(std::move(rez)));
   return reze;

}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args):class_p(class_),args_(std::move(args)){



}

NewInstance::NewInstance(const runtime::Class& class_):class_p(class_) {

}


/*
Создаёт новый экземпляр класса class_, передавая его конструктору набор параметров args.
Если в классе отсутствует метод __init__ с заданным количеством аргументов,
то экземпляр класса создаётся без вызова конструктора (поля объекта не будут проинициализированы):

class Person:
  def set_name(name):
    self.name = name

p = Person()
# Поле name будет иметь значение только после вызова метода set_name
p.set_name("Ivan")
*/
ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    ObjectHolder object{ObjectHolder::Own(runtime::ClassInstance{class_p})};
    runtime::ClassInstance* class_instance=object.TryAs<runtime::ClassInstance>();
    if (class_instance->HasMethod(INIT_METHOD, args_.size())) {
        std::vector<ObjectHolder> args;
        for (auto&& argument : args_) {
            args.push_back(argument.get()->Execute(closure, context));
        }
         class_instance->Call(INIT_METHOD, args, context);
    }
 return object;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body):body_(std::move(body)) {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
            ObjectHolder objectHold = body_->Execute(closure, context);
            if(objectHold.Get() != nullptr) {
                return objectHold;
            }
            return ObjectHolder::None();
        }
        catch (ObjectHolder objectHold) {
            return objectHold;
        }
}

}  // namespace ast
