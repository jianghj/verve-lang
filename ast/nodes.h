#include "utils/macros.h"

#include "parser/environment.h"
#include "parser/token.h"
#include "parser/type.h"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#pragma once

#define DECLARE_TYPE(__class) \
  struct __class; \
  using  __class##Ptr = std::shared_ptr<__class>;

#define DECLARE_CONVERTER(__class) \
  __unused static __class##Ptr as##__class(NodePtr __n) { \
    return std::dynamic_pointer_cast<__class>(__n); \
  }

#define DECLARE_CTOR(__class)  \
    static inline __class##Ptr create##__class(Loc loc) { \
       auto node = std::make_shared<__class>(loc); \
      node->m_loc = loc; \
      return node; \
    } \

#define CLONE_AST(__type) \
  NodePtr clone() const { \
    return std::make_shared<__type>(*this); \
  }


#define AST_TYPES \
      Program, \
      Block, \
      Call, \
      Number, \
      Identifier, \
      String, \
      Function, \
      FunctionParameter, \
      If, \
      BinaryOperation, \
      UnaryOperation, \
      List, \
      Match, \
      Case, \
      Pattern, \
      Let, \
      Constructor, \
      Assignment, \
      Interface, \
      Implementation, \
      AbstractType, \
      BasicType, \
      FunctionType, \
      DataType, \
      EnumType, \
      TypeConstructor, \
      Prototype

namespace Verve {
namespace AST {
  class Visitor;

  EVAL(MAP(DECLARE_TYPE, AST_TYPES))

  using NodePtr = std::shared_ptr<NodeInterface>;

  struct NodeInterface {
    virtual Type *typeof(EnvPtr env) = 0;
    virtual NodePtr clone() const = 0;
    virtual const Loc &loc() const = 0;
    virtual void visit(Visitor *) = 0;
  };

  struct FunctionInterface : virtual public NodeInterface {
    virtual std::string &getName() = 0;
  };

  struct Node : virtual public NodeInterface {
    Node(Loc l): m_loc(l) {  }

    virtual const Loc &loc() const {
      return m_loc;
    }

    virtual Type *typeof(__unused EnvPtr env) {
      throw std::runtime_error("Trying to get type for virtual node");
    }

    virtual void visit(__unused Visitor *_) {
      throw std::runtime_error("Trying to visit virtual node");
    }
    virtual NodePtr clone() const;

    Loc m_loc;
  };

  struct Block : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::vector<NodePtr> nodes;
    unsigned stackSlots = 0;
    EnvPtr env;
  };

  struct Program : public Block {
    using Block::Block;

    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::vector<ProgramPtr> imports;
  };

  struct Number : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual void visit(Visitor *);
    CLONE_AST(Number)

    double value;
    bool isFloat = false;
  };

  struct Identifier : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual void visit(Visitor *);
    CLONE_AST(Identifier)

    std::string name;
    std::string ns;
    bool isCaptured;
    bool isFunctionParameter = false;
    unsigned index;
  };

  struct String : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual void visit(Visitor *);
    CLONE_AST(String)

    std::string value;
  };

  struct FunctionParameter : public Identifier {
    using Identifier::Identifier;

    virtual void visit(Visitor *);
    CLONE_AST(FunctionParameter)
  };

  struct Call : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    NodePtr callee;
    std::vector<NodePtr> arguments;
  };

  struct If : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    NodePtr condition;
    BlockPtr ifBody;
    BlockPtr elseBody = nullptr;
  };

  struct BinaryOperation : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    unsigned op;
    NodePtr lhs;
    NodePtr rhs;
  };

  struct UnaryOperation : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    unsigned op;
    NodePtr operand;
  };

  struct List : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::vector<NodePtr> items;
  };

  struct Pattern : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    unsigned tag;
    std::string constructorName;
    std::vector<IdentifierPtr> values;
    NodePtr value;
  };

  struct Case : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    PatternPtr pattern;
    BlockPtr body;
  };

  struct Match : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    NodePtr value;
    std::vector<CasePtr> cases;
  };

  struct Assignment : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    enum {
      Pattern,
      Identifier,
    } kind;

    union Left {
      Left() :
        pattern(nullptr) {}

      PatternPtr pattern;
      IdentifierPtr ident;

      ~Left() {
        pattern = nullptr;
        ident = nullptr;
      }
    } left;
    NodePtr value;
  };

  struct Let : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::vector<AssignmentPtr> assignments;
    BlockPtr block;
  };

  struct Constructor : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::string name;
    std::vector<NodePtr> arguments;
    unsigned tag;
    unsigned size;
  };

  struct Interface : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::string name;
    std::string genericTypeName;
    std::vector<std::string> virtualFunctions;
    std::vector<std::string> concreteFunctions;
    std::vector<std::shared_ptr<FunctionInterface>> functions;
    EnvPtr env;
  };

  struct Implementation : public Node {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::string interfaceName;
    AbstractTypePtr type;
    std::vector<std::shared_ptr<FunctionInterface>> functions;
    EnvPtr env;
  };

  struct AbstractType : public Node {
    using Node::Node;

  };

  struct BasicType : public AbstractType {
    using AbstractType::AbstractType;

    virtual Type *typeof(EnvPtr env);
    virtual void visit(Visitor *);
    CLONE_AST(BasicType)

    std::string name;
  };

  struct FunctionType : public AbstractType {
    using AbstractType::AbstractType;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::vector<std::string> generics;
    std::vector<AbstractTypePtr> params;
    AbstractTypePtr returnType;
  };

  struct DataType : public AbstractType {
    using AbstractType::AbstractType;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::string name;
    std::vector<AbstractTypePtr> params;
  };

  struct EnumType : public AbstractType {
    using AbstractType::AbstractType;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::string name;
    std::vector<std::string> generics;
    std::vector<TypeConstructorPtr> constructors;
  };

  struct TypeConstructor : public AbstractType {
    using AbstractType::AbstractType;

    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    std::string name;
    std::vector<AbstractTypePtr> types;
  };

  struct Prototype : public FunctionType, public FunctionInterface {
    using FunctionType::FunctionType;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    virtual std::string &getName() {
      return name;
    }

    std::string name;
    struct {
      bool isExternal: 1;
      bool isVirtual: 1;
    };
  };

  struct Function : public Node, public FunctionInterface {
    using Node::Node;

    virtual Type *typeof(EnvPtr env);
    virtual NodePtr clone() const;
    virtual void visit(Visitor *);

    virtual std::string &getName() {
      return name;
    }

    PrototypePtr type = nullptr;
    std::string ns;
    std::string name;
    std::vector<FunctionParameterPtr> parameters;
    BlockPtr body;
    bool needsScope;
    bool capturesScope;
    std::unordered_map<std::string, FunctionPtr> instances;
  };


  EVAL(MAP(DECLARE_CONVERTER, AST_TYPES))
  EVAL(MAP(DECLARE_CTOR, AST_TYPES))
}
}
