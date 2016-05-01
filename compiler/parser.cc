#include "parser.h"

#include "lexer.h"
#include "token.h"

namespace ceos {

  std::shared_ptr<AST::Program> Parser::parse(void) {
    m_lexer.nextToken();
    m_ast = std::make_shared<AST::Program>();
    m_ast->loc.start = m_lexer.token()->loc.start;
    parseBody(m_ast->body, Token::Type::END);
    return m_ast;
  }

  void Parser::parseBody(std::vector<std::shared_ptr<AST>> &body, Token::Type delim) {
    while (m_lexer.token()->type != delim) {
      std::shared_ptr<AST> node = parseFactor();
      body.push_back(node);
    }
  }

  std::shared_ptr<AST> Parser::parseFactor() {
    switch (m_lexer.token()->type) {
      case Token::Type::NUMBER:
        return parseNumber();
      case Token::Type::ID:
        return parseID();
      case Token::Type::STRING:
        return parseString();
      default:
        m_lexer.invalidType();
    }
  }

  std::shared_ptr<AST> Parser::parseIf() {
    auto _if = std::make_shared<AST::If>();

    m_lexer.ensure(Token::Type::L_PAREN);
    _if->condition = parseFactor();
    m_lexer.ensure(Token::Type::R_PAREN);

    if (m_lexer.token()->type == Token::Type::L_BRACE) {
      m_lexer.ensure(Token::Type::L_BRACE);
      parseBody(_if->ifBody, Token::Type::R_BRACE);
      m_lexer.ensure(Token::Type::R_BRACE);
    } else {
      _if->ifBody.push_back(parseFactor());
    }

    if (m_lexer.token()->type == Token::Type::ID) {
      auto maybeElse = std::static_pointer_cast<Token::ID>(m_lexer.token());
      if (maybeElse->name == "else") {
        m_lexer.ensure(Token::Type::ID);

        if (m_lexer.token()->type == Token::Type::L_BRACE) {
          m_lexer.ensure(Token::Type::L_BRACE);
          parseBody(_if->elseBody, Token::Type::R_BRACE);
          m_lexer.ensure(Token::Type::R_BRACE);
        } else {
          _if->elseBody.push_back(parseFactor());
        }
      }
    }

    return _if;
  }

  std::shared_ptr<AST::Number> Parser::parseNumber() {
    auto number = std::static_pointer_cast<Token::Number>(m_lexer.token(Token::Type::NUMBER));
    auto ast = std::make_shared<AST::Number>(number->value);
    ast->loc = number->loc;
    return ast;
  }

  std::shared_ptr<AST> Parser::parseID() {
    auto id = std::static_pointer_cast<Token::ID>(m_lexer.token(Token::Type::ID));

    unsigned uid;
    auto it = std::find(m_ast->strings.begin(), m_ast->strings.end(), id->name);
    if (it != m_ast->strings.end()) {
      uid = it - m_ast->strings.begin();
    } else {
      uid = str_uid++;
      m_ast->strings.push_back(id->name);
    }

    if (id->name == "if") {
      return parseIf();
    }

    std::shared_ptr<AST> ast = std::make_shared<AST::ID>(m_ast->strings[uid], uid);
    ast->loc = id->loc;

    while (true) {
      if (m_lexer.token()->type == Token::Type::TYPE) {
        ast = parseTypeInfo(std::move(ast));
      } else if (m_lexer.token()->type == Token::Type::L_PAREN) {
        ast = parseCall(std::move(ast));
      } else if (m_lexer.token()->type == Token::Type::L_BRACE) {
        assert(ast->type == AST::Type::Call);
        auto call = AST::asCall(ast);
        assert(call->callee->type == AST::Type::ID);
        auto callee = AST::asID(call->callee);
        auto fn = std::make_shared<AST::Function>();
        fn->name = std::move(callee);
        for (auto arg : call->arguments) {
          assert(arg->type == AST::Type::ID);
        }
        fn->arguments = std::move(call->arguments);
        m_lexer.ensure(Token::Type::L_BRACE);
        parseBody(fn->body, Token::Type::R_BRACE);
        fn->loc.start = fn->name->loc.start;
        fn->loc.end = m_lexer.token(Token::Type::R_BRACE)->loc.end;
        ast = fn;
      } else {
        break;
      }
    }

    return ast;
  }

  std::shared_ptr<AST::String> Parser::parseString() {
    auto string = std::static_pointer_cast<Token::String>(m_lexer.token(Token::Type::STRING));

    int uid;
    auto it = std::find(m_ast->strings.begin(), m_ast->strings.end(), string->value);
    if (it != m_ast->strings.end()) {
      uid = it - m_ast->strings.begin();
    } else {
      uid = str_uid++;
      m_ast->strings.push_back(string->value);
    }

    auto ast =  std::make_shared<AST::String>(m_ast->strings[uid], uid);
    ast->loc = string->loc;
    return ast;
  }

  std::shared_ptr<AST::Call> Parser::parseCall(std::shared_ptr<AST> &&callee) {
    auto start = callee->loc.start;

    m_lexer.ensure(Token::Type::L_PAREN);

    auto call = std::make_shared<AST::Call>();
    call->callee = callee;

    while (m_lexer.token()->type != Token::Type::R_PAREN) {
      call->arguments.push_back(parseFactor());
      if (m_lexer.token()->type != Token::Type::R_PAREN) {
        m_lexer.ensure(Token::Type::COMMA);
      }
    }

    auto end = m_lexer.token(Token::Type::R_PAREN)->loc.end;
    call->loc = { start, end };

    return call;
  }

  std::shared_ptr<AST::TypeInfo> Parser::parseTypeInfo(std::shared_ptr<AST> &&target) {
    m_lexer.ensure(Token::Type::TYPE);

    auto typeInfo = std::make_shared<AST::TypeInfo>();
    typeInfo->target = std::move(target);
    typeInfo->type = parseID();
    return typeInfo;
  }

}
