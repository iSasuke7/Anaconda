#include "parser/anacondaparser.h"
#include "ast/datatype.h"

AnacondaParser::AnacondaParser(const std::string& input):
	Parser(input) {}

// <parse> = <unit> <WS>? <EOF>
StatementListNode* AnacondaParser::parse()
{
	StatementListNode *node = unit();
	whitespace();
	if (!atEnd())
	{
		delete node;
		return nullptr;
	}
	return node;
}

std::string AnacondaParser::id()
{
	beginCapture();
	if (!expectLetter())
	{
		endCapture();
		return "";
	}

	while(expectLetter() || expect('_'))
		continue;

	return endCapture();
}

DataTypeBase* AnacondaParser::type() {
	whitespace();

	if (expect("u8"))
		return new DataType<DATATYPE_U8>();
	else
	{
		std::string name = id();
		if (name == "")
			return nullptr;

		return new DataType<DATATYPE_STRUCT_FORWARD>(name);
	}

	return nullptr;
}

// <unit> = <funcdecl>*
StatementListNode* AnacondaParser::unit()
{
	StatementListNode *list = nullptr;

	while (true)
	{
		StatementNode *node = (StatementNode*) funcdecl();
		if (!node)
			break;

		list = new StatementListNode(list, node);
	}

	return list;
}

// <funcdecl> = <WS>? 'func' <WS> <id> <WS>? '(' <funcpar> ')' ('->' <id>)? <block>
FunctionDeclaration* AnacondaParser::funcdecl()
{
	whitespace();
	if (!(expect("func") && whitespace()))
		return nullptr;

	std::string name = id();
	if (name == "")
		return nullptr;

	FunctionParameters* parameters = funcpar();
	if (!parameters)
		return nullptr;

	whitespace();
	DataTypeBase *rtype(expect("->") ? type() : new DataType<DATATYPE_VOID>);

	if (!rtype)
	{
		delete parameters;
		return nullptr;
	}

	BlockNode *list = block();

	if (!list) {
		delete parameters;
		delete rtype;
		return nullptr;
	}

	return new FunctionDeclaration(name, parameters, rtype, list);
}

// <funcpar> = (<WS>? <id> <WS>? ',')* <id>?
FunctionParameters* AnacondaParser::funcpar()
{
	std::map<std::string, DataTypeBase*> parameters;

	whitespace();
	if (!expect('('))
		return nullptr;

	while (true)
	{
		whitespace();

		DataTypeBase *partype(type());

		if (!partype)
			break;

		if (!whitespace())
		{
			delete partype;
			break;
		}

		std::string parname = id();
		if (parname == "")
		{
			delete partype;
			break;
		}

		parameters[parname] = partype;

		whitespace();
		if (expect(')'))
			return new FunctionParameters(parameters);
		else if (!expect(','))
			break;
	}

	for (auto i : parameters)
		delete i.second;
	return nullptr;
}

// <block> = '{' <statlist> '}'
BlockNode* AnacondaParser::block()
{
	whitespace();
	if (!expect('{'))
		return nullptr;

	StatementListNode *list = statlist();

	whitespace();
	if (!expect('}'))
	{
		delete list;
		return nullptr;
	}

	return new BlockNode(list);
}

// <statlist> = <statement>*
StatementListNode* AnacondaParser::statlist()
{
	StatementListNode *list = nullptr;

	while (true)
	{
		StatementNode *node = statement();
		if (!node)
			break;

		list = new StatementListNode(list, node);
	}

	return list;
}

// <statement> = <ifstat>
StatementNode* AnacondaParser::statement()
{
	return ifstat();
}

StatementNode* AnacondaParser::ifstat()
{
	whitespace();
	if (!(expect("if") && !whitespace()))
		return nullptr;

	return nullptr;
}

// <expr> = <sum>
ExpressionNode* AnacondaParser::expr()
{
	return sum();
}

// <sum> = <product> (<WS>? ('+' | '-') <product>)*
ExpressionNode* AnacondaParser::sum()
{
	ExpressionNode *lhs = product();
	if (!lhs)
		return nullptr;

	while (true)
	{
		whitespace();

		char op = peek();
		if (op != '+' && op != '-')
			break;

		ExpressionNode *rhs = product();
		if (!rhs)
		{
			delete lhs;
			return nullptr;
		}

		switch (op)
		{
		case '+':
			lhs = new AddNode(lhs, rhs);
			break;
		case '-':
			lhs = new SubNode(lhs, rhs);
			break;
		}
	}

	return lhs;
}

// <product> = <unary> (<WS>? ('*' | '/' | '%') <unary>)*
ExpressionNode* AnacondaParser::product()
{
	ExpressionNode *lhs = unary();
	if (!lhs)
		return nullptr;

	while (true)
	{
		whitespace();

		char op = peek();
		if (op != '*' && op != '/' && op != '%')
			break;

		ExpressionNode *rhs = unary();
		if (!rhs)
		{
			delete lhs;
			return nullptr;
		}

		switch (op)
		{
		case '*':
			lhs = new MulNode(lhs, rhs);
			break;
		case '/':
			lhs = new DivNode(lhs, rhs);
			break;
		case '%':
			lhs = new ModNode(lhs, rhs);
			break;
		}
	}

	return nullptr;
}

// <unary> = <WS>? ('-' <unary> | <atom>)
ExpressionNode* AnacondaParser::unary()
{
	whitespace();
	if (expect('-'))
	{
		ExpressionNode *node = unary();
		if (!node)
			return new NegateNode(node);
	}

	return atom();
}

// <atom> = <paren> | <funccall> | <variable>
ExpressionNode* AnacondaParser::atom()
{
	state_t state = save();

	ExpressionNode *node = paren();
	if (node)
		return node;
	restore(state);

	node = funccall();
	if (node)
		return node;
	restore(state);

	node = variable();
	if (node)
		return node;

	return nullptr;
}

// <paren> = <WS>? '(' <expr> <WS>? ')'
ExpressionNode* AnacondaParser::paren()
{
	whitespace();
	if (!expect('('))
		return nullptr;

	ExpressionNode *node = expr();
	if (!node)
		return nullptr;

	whitespace();
	if (!expect(')'))
	{
		delete node;
		return nullptr;
	}

	return node;
}

// <funccall> = <WS>? <id> <WS>? '(' <funcargs> <WS>? ')'
FunctionCallNode* AnacondaParser::funccall()
{
	whitespace();
	std::string name = id();
	if (name == "")
		return nullptr;

	whitespace();
	if (!expect('('))
		return nullptr;

	FunctionArguments* args = funcargs();

	whitespace();
	if (!expect(')'))
	{
		delete args;
		return nullptr;
	}

	return new FunctionCallNode(name, args);
}

// <funcargs> = (<WS>? <expr> <WS>? ',')* <expr>?
FunctionArguments* AnacondaParser::funcargs()
{
	std::vector<ExpressionNode*> arguments;

	whitespace();
	if (!expect('('))
		return nullptr;

	while (true)
	{
		whitespace();
		ExpressionNode *arg = expr();
		if (!arg)
			break;

		arguments.push_back(arg);

		whitespace();
		if (expect(')'))
			return new FunctionArguments(arguments);
		else if (!expect(','))
			break;
	}

	for(auto expr : arguments)
		delete expr;
	return nullptr;
}

// <variable> = <id>
VariableNode* AnacondaParser::variable()
{
	whitespace();
	std::string name = id();
	if (name == "")
		return nullptr;

	return new VariableNode(name);
}
