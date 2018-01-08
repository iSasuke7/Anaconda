#include "ast/node.h"
#include "util/utils.h"
#include "generator/brainfuck.h"

#include <iostream>

AssemblyNode::AssemblyNode(DataTypeBase* datatype, const std::string& assembly, ArgumentListNode* arguments):
    datatype(datatype), assembly(assembly), arguments(arguments) {}

AssemblyNode::~AssemblyNode()
{
    delete this->datatype;
}

void AssemblyNode::print(std::ostream& os, size_t level) const
{
    this->printIndent(os, level);
    os << "assembly statement -> " << *this->datatype << " (" << this->assembly << ")" << std::endl;
    this->arguments->print(os, level+1);
}

void AssemblyNode::checkTypes(BrainfuckWriter& writer)
{
    this->arguments->checkTypes(writer);
}

DataTypeBase* AssemblyNode::getType()
{
    return this->datatype->copy();
}

void AssemblyNode::generate(BrainfuckWriter& writer)
{
    writer.copyAssembly(this->assembly);
}

void AssemblyNode::declareLocals(BrainfuckWriter& writer)
{
    UNUSED(writer);
}
