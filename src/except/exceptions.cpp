#include "except/exceptions.h"

ProgramException::ProgramException(const std::string& msg) : std::runtime_error(msg) {}
ProgramException::ProgramException(const char* msg) : std::runtime_error(msg) {}

ASTException::ASTException(const std::string& msg) : ProgramException(msg) {}
ASTException::ASTException(const char* msg) : ProgramException(msg) {}

TypeCheckException::TypeCheckException(const std::string& msg) : ASTException(msg) {}
TypeCheckException::TypeCheckException(const char* msg) : ASTException(msg) {}
