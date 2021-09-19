#include <algorithm>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <stack>
#include <tuple>
#include <vector>

using namespace std;
int elseCounter = 6969;
int endIfCounter = 6969;
int loopCounter = 6969;
int endWhileCounter = 6969;
int deleteCounter = 6969;

class Tree { 
    public:
    string type; // lhs of the rule
	string value; // name/value of the terminal
	vector<string> rule; // output/rule
    vector<Tree> children; // child nodes
}; 

Tree createTree() {
	string rule, word, type;
	getline(cin, rule);
	istringstream terms(rule);
	terms >> type;
	if (isupper(type[0])) { // terminal
		Tree node;
		node.type = type;
		terms >> word;
		node.value = word;
		vector<string> rule{type, word};
		node.rule = rule;
		return node;
	}
	else { // non-terminal, there is a rule
		Tree node;
		node.type = type;
		vector<Tree> children;
		vector<string> rule{type};
		while (terms >> word) {
			Tree child = createTree();
			children.push_back(child);
			rule.push_back(word);
		}
		node.rule = rule;
		node.children = children;
		return node;
	}
}

// prints the tree in accordance with A6P5
void printTree(Tree &t) {
	cout << t.rule[0];
	for (int i = 1; i < t.rule.size(); i++) cout << ' ' << t.rule[i];
	cout << endl;
	for (int i = 0; i < t.children.size(); i++) printTree(t.children[i]);
}

pair<string, string> getDcl(Tree &dcl) {
	string type, id;

	// find type of declaration
	Tree typeNode = dcl.children[0];
	if (typeNode.children.size() == 1) type = "int";
	else if (typeNode.children.size() == 2) type = "int*";
	id = dcl.children[1].value;

	return make_pair(type, id);
}

string getLvalue(Tree &tree) {
	// lvalue
	if (tree.type == "lvalue") {
		// lvalue ID
		if (tree.children[0].type == "ID") {
			return tree.children[0].value;
		}

		// lvalue STAR factor
		// this should never happen as we cover the cases where this rule might be used elsewhere
		else if (tree.children[0].type == "STAR") {
			cerr << "ERROR: getLvalue reached STAR!" << endl; 
		}

		// lvalue LPAREN lvalue RPAREN	
		else if (tree.children[0].type == "LPAREN") {
			return getLvalue(tree.children[1]);
		}
	}
	return "";
}

void printPush(int regNum) {
	cout << "sw $" << regNum << ", -4($30)" << endl;
	cout << "sub $30, $30, $4" << endl;
}

void printPop(int regNum) {
	cout << "add $30, $30, $4" << endl;
	cout << "lw $" << regNum << ", -4($30)" << endl;
}

string findType(Tree &tree, unordered_map<string, pair<string, int>> &symbolTable) {
	string op, left, right, id;

	if (tree.type == "ID") {
		return symbolTable.find(tree.value)->second.first;
	}

	// expr
	else if (tree.type == "expr") {
		// expr term
		if (tree.children.size() == 1) return findType(tree.children[0], symbolTable);

		// expr expr (op) term
		op = tree.children[1].type;
		left = findType(tree.children[0], symbolTable);
		right = findType(tree.children[2], symbolTable);
		if (op == "PLUS") {
			if (left == "int" && right == "int") return "int";
			if (left == "int*" && right == "int") return "int*";
			if (left == "int" && right == "int*") return "int*";
		}
		else if (op == "MINUS") {
			if (left == "int" && right == "int") return "int";
			if (left == "int*" && right == "int") return "int*";
			if (left == "int*" && right == "int*") return "int";
		}
	}

	// term
	else if (tree.type == "term") {
		// term factor
		if (tree.children.size() == 1) return findType(tree.children[0], symbolTable);

		// term term (op) factor
		else return "int";
	}

	// factor (op)
	else if (tree.type == "factor") {
		op = tree.children[0].type;
		if (op == "ID") {
			if (tree.children.size() == 1) return findType(tree.children[0], symbolTable); // factor ID
			else return "int"; // parameter
		}
		else if (op == "NUM") return "int"; // factor NUM
		else if (op == "NULL") return "int*"; // factor NULL
		else if (op == "LPAREN") return findType(tree.children[1], symbolTable); // factor LPAREN expr RPAREN
		else if (op == "AMP") return "int*"; // factor AMP lvalue
		else if (op == "STAR") return "int"; // factor STAR factor
		else if (op == "NEW") return "int*"; // factor NEW INT LBRACK expr RBRACK
	}

	// lvalue
	else if (tree.type == "lvalue") {
		op = tree.children[0].type;
		if (op == "ID") return findType(tree.children[0], symbolTable); // lvalue ID
		else if (op == "STAR") return "int"; // lvalue STAR factor
		else if (op == "LPAREN") return findType(tree.children[1], symbolTable); // lvalue LPAREN lvalue RPAREN
	}
	cerr << "ERROR: Reached end of findType!" << endl;
	return "";
}

// codes statements
void codeStatement(Tree &tree, unordered_map<string, pair<string, int>> &symbolTable) {
	int offset;
	string id;
	pair<string, int> symbolInfo;

	// statements
	if (tree.type == "statements") {
		// statements statements statement
		if (tree.children.size() > 0) {
			// code the statement first, then move on to the next statement
			codeStatement(tree.children[0], symbolTable);
			codeStatement(tree.children[1], symbolTable);
		}
	}

	// statement
	else if (tree.type == "statement") {
		// statement lvalue BECOMES expr SEMI
		if (tree.children[0].type == "lvalue") {
			// two rules of lvalue calculated differently
			Tree lvalue = tree.children[0];
			// lvalue ID
			if (lvalue.children[0].type == "ID") {
				codeStatement(tree.children[2], symbolTable); // code expr
			
				// find id from lvalue
				id = getLvalue(tree.children[0]);
				symbolInfo = symbolTable.find(id)->second;
				offset = symbolInfo.second;

				cout << "sw $3, " << offset << "($29)" << endl;
			}

			// lvalue STAR factor
			else if (lvalue.children[0].type == "STAR") {
				codeStatement(tree.children[2], symbolTable); // generate code for RHS
				printPush(3); // push($3)
				codeStatement(lvalue.children[1], symbolTable); // generate code for LHS
				printPop(5); // pop($5), $5 contains value to assign
				cout << "sw $5, 0($3)" << endl;
			}
		}

		// statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
		else if (tree.children[0].type == "IF") {
			int elseC = elseCounter;
			int eiC = endIfCounter;
			elseCounter++;
			endIfCounter++;
			codeStatement(tree.children[2], symbolTable); // code test
			cout << "beq $3, $0, elseLabel" << elseC << endl;
			codeStatement(tree.children[5], symbolTable); // code statements 1
			cout << "beq $0, $0, endIfLabel" << eiC << endl;
			cout << "elseLabel" << elseC << ":" << endl;
			codeStatement(tree.children[9], symbolTable); // code statements 2
			cout << "endIfLabel" << eiC << ":" << endl;
		}

		// statement WHILE LPAREN test RPAREN LBRACE statements RBRACE
		else if (tree.children[0].type == "WHILE") {
			int loopC = loopCounter;
			int ewC = endWhileCounter;
			loopCounter++;
			endWhileCounter++;
			cout << "loopLabel" << loopC << ":" << endl;
			codeStatement(tree.children[2], symbolTable); // code test
			cout << "beq $3, $0, endWhileLabel" << ewC << endl;
			codeStatement(tree.children[5], symbolTable); // code statements
			cout << "beq $0, $0, loopLabel" << loopC << endl;
			cout << "endWhileLabel" << ewC << ":" << endl;
		}

		// statement PRINTLN LPAREN expr RPAREN SEMI
		else if (tree.children[0].type == "PRINTLN") {
			printPush(1); // push $1
			codeStatement(tree.children[2], symbolTable); // code expr
			cout << "add $1, $3, $0" << endl;
			printPush(31); // push $31
			cout << "jalr $10" << endl;
			printPop(31); // pop $31
			printPop(1); // pop $1
		}

		// statement DELETE LBRACK RBRACK expr SEMI
		else if (tree.children[0].type == "DELETE") {
			codeStatement(tree.children[3], symbolTable); // code expr
			int deleteC = deleteCounter;
			deleteCounter++;
			cout << "beq $3, $11, skipDelete" << deleteC << endl; // do NOT call delete on NULL
			cout << "add $1, $3, $0 ; delete expects address in $1" << endl;
			printPush(31); // push $31
			cout << "lis $5" << endl;
			cout << ".word delete" << endl;
			cout << "jalr $5" << endl;
			printPop(31); // pop $31
			cout << "skipDelete" << deleteC << ":" << endl;
		}
	}

	// expr
	else if (tree.type == "expr") {
		// expr term
		if (tree.children[0].type == "term") {
			codeStatement(tree.children[0], symbolTable);
			return;
		}
		
		string leftType = findType(tree.children[0], symbolTable);
		string rightType = findType(tree.children[2], symbolTable);
		// expr expr PLUS term
		if (tree.children[1].type == "PLUS") {
			// int PLUS int
			if (leftType == "int" && rightType == "int") {
				codeStatement(tree.children[0], symbolTable); // code expr
				printPush(3); // push $3
				codeStatement(tree.children[2], symbolTable); // code term
				printPop(5); // pop $5
				cout << "add $3, $5, $3" << endl;
			}

			// int* PLUS int
			else if (leftType == "int*" && rightType == "int") {
				codeStatement(tree.children[0], symbolTable); // code expr
				printPush(3); // push $3
				codeStatement(tree.children[2], symbolTable); // code term
				cout << "mult $3, $4" << endl;
				cout << "mflo $3" << endl;
				printPop(5); // pop $5
				cout << "add $3, $5, $3" << endl;
			}

			// int PLUS int*
			else if (leftType == "int" && rightType == "int*") {
				codeStatement(tree.children[2], symbolTable); // code term
				printPush(3); // push $3
				codeStatement(tree.children[0], symbolTable); // code expr
				cout << "mult $3, $4" << endl;
				cout << "mflo $3" << endl;
				printPop(5); // pop $5
				cout << "add $3, $5, $3" << endl;
			}
		}

		// expr expr MINUS term
		else if (tree.children[1].type == "MINUS") {
			// int MINUS int
			if (leftType == "int" && rightType == "int") {
				codeStatement(tree.children[0], symbolTable); // code expr
				printPush(3); // push $3
				codeStatement(tree.children[2], symbolTable); // code term
				printPop(5); // pop $5
				cout << "sub $3, $5, $3" << endl; 
			}

			// int* MINUS int
			else if (leftType == "int*" && rightType == "int") {
				codeStatement(tree.children[0], symbolTable); // code expr
				printPush(3); // push $3
				codeStatement(tree.children[2], symbolTable); // code term
				cout << "mult $3, $4" << endl;
				cout << "mflo $3" << endl;
				printPop(5); // pop $5
				cout << "sub $3, $5, $3" << endl;
			}

			// int* MINUS int*
			else if (leftType == "int*" && rightType == "int*") {
				codeStatement(tree.children[0], symbolTable); // code expr
				printPush(3); // push $3
				codeStatement(tree.children[2], symbolTable); // code term
				printPop(5); // pop $5
				cout << "sub $3, $5, $3" << endl; 
				cout << "div $3, $4" << endl;
				cout << "mflo $3" << endl;
			}
		}
	}
	
	// term
	else if (tree.type == "term") {
		// term factor
		if (tree.children[0].type == "factor") {
			codeStatement(tree.children[0], symbolTable);
		}

		// term term STAR factor
		else if (tree.children[1].type == "STAR") {
			codeStatement(tree.children[0], symbolTable); // code term
			printPush(3); // push $3
			codeStatement(tree.children[2], symbolTable); // code factor
			printPop(5); // pop $5
			cout << "mult $5, $3" << endl;
			cout << "mflo $3" << endl; 
		}

		// term term SLASH factor
		else if (tree.children[1].type == "SLASH") {
			codeStatement(tree.children[0], symbolTable); // code term
			printPush(3); // push $3
			codeStatement(tree.children[2], symbolTable); // code factor
			printPop(5); // pop $5
			cout << "div $5, $3" << endl;
			cout << "mflo $3" << endl; 
		}

		// term term PCT factor
		else if (tree.children[1].type == "PCT") {
			codeStatement(tree.children[0], symbolTable); // code term
			printPush(3); // push $3
			codeStatement(tree.children[2], symbolTable); // code factor
			printPop(5); // pop $5
			cout << "div $5, $3" << endl;
			cout << "mfhi $3" << endl; 
		}
	}

	// factor
	else if (tree.type == "factor") {
		// factor NUM
		if (tree.children[0].type == "NUM") {
			cout << "lis $3" << endl;
			cout << ".word " << tree.children[0].value << endl;
		}
		
		// factor ID
		else if (tree.children[0].type == "ID" && tree.children.size() == 1) {
			id = tree.children[0].value;
			symbolInfo = symbolTable.find(id)->second;
			offset = symbolInfo.second;
			cout << "lw $3, " << offset << "($29)" << endl;
		}

		// factor NULL
		else if (tree.children[0].type == "NULL") {
			cout << "add $3, $0, $11" << endl; // recall $11 is always 1
		}

		// factor LPAREN expr RPAREN
		else if (tree.children[0].type == "LPAREN") {
			codeStatement(tree.children[1], symbolTable); // code expr
		}

		// factor AMP lvalue
		else if (tree.children[0].type == "AMP") {
			// two rules of lvalue calculated differently
			Tree lvalue = tree.children[1];

			// lvalue ID
			if (lvalue.children[0].type == "ID") {
				id = getLvalue(lvalue);
				symbolInfo = symbolTable.find(id)->second;
				offset = symbolInfo.second;
				cout << "lis $3" << endl;
				cout << ".word " << offset << endl;
				cout << "add $3, $3, $29" << endl;
			}

			// lvalue STAR factor
			else if (lvalue.children[0].type == "STAR") {
				codeStatement(lvalue.children[1], symbolTable); // code factor
			}
		}

		// factor STAR factor
		else if (tree.children[0].type == "STAR") {
			codeStatement(tree.children[1], symbolTable);
			cout << "lw $3, 0($3)" << endl;
		}

		// factor NEW INT LBRACK expr RBRACK
		else if (tree.children[0].type == "NEW") {
			codeStatement(tree.children[3], symbolTable);
			cout << "add $1, $3, $0 ; new procedure expects value in $1" << endl;
			printPush(31); // push($31)
			cout << "lis $5" << endl;
			cout << ".word new" << endl;
			cout << "jalr $5" << endl;
			printPop(31); // pop($31)
			cout << "bne $3, $0, 1 ; if call succeeded skip next instruction" << endl;
			cout << "add $3, $11, $0 ; set $3 to NULL if allocation fails" << endl;
		}

		// factor ID LPAREN RPAREN
		else if (tree.children[2].type == "RPAREN") {
			printPush(29); // push($29)
			printPush(31); // push($31)
			// no arguments so no expressions to code
			cout << "lis $5" << endl;
			cout << ".word " << tree.children[0].value << endl;
			cout << "jalr $5" << endl;
			// no arguments, so no need to pop
			printPop(31); // pop($31)
			printPop(29); // pop($29)
		}

		// factor ID LPAREN arglist RPAREN
		else if (tree.children[2].type == "arglist") {
			printPush(29); // push($29)
			printPush(31); // push($31)

			// code arguments list
			int numArguments = 0;
			Tree arglist = tree.children[2];

			// arglist expr COMMA arglist
			while (arglist.children.size() == 3) {
				codeStatement(arglist.children[0], symbolTable);
				printPush(3); // push($3)
				arglist = arglist.children[2];
				numArguments++;
			}

			// arglist expr
			codeStatement(arglist.children[0], symbolTable);
			printPush(3); // push($3)
			numArguments++;
			
			cout << "lis $5" << endl;
			cout << ".word " << tree.children[0].value << endl;
			cout << "jalr $5" << endl;
			
			// pop each argument
			for (int i = 0; i < numArguments; i++) printPop(31); // discard

			printPop(31); // pop($31)
			printPop(29); // pop($29)
		}
	}

	// test
	else if (tree.type == "test") {
		string testType = findType(tree.children[0], symbolTable);
		string sltType;
		if (testType == "int") sltType = "slt";
		else sltType = "sltu";

		// test expr LT expr
		if (tree.children[1].type == "LT") {
			codeStatement(tree.children[0], symbolTable); // code expr 1
			printPush(3); // push 3
			codeStatement(tree.children[2], symbolTable); // code expr 2
			printPop(5); // pop 5
			cout << sltType << " $3, $5, $3" << endl;
		}

		// test expr NE expr
		else if (tree.children[1].type == "NE") {
			codeStatement(tree.children[0], symbolTable); // code expr 1
			printPush(3); // push 3
			codeStatement(tree.children[2], symbolTable); // code expr 2
			printPop(5); // pop 5
			cout << sltType << " $6, $3, $5" << endl; // $6 = $3 < $5
			cout << sltType << " $7, $5, $3" << endl; // $7 = $5 < $3
			cout << "add $3, $6, $7" << endl;
		}

		// test expr EQ expr
		else if (tree.children[1].type == "EQ") {
			codeStatement(tree.children[0], symbolTable); // code expr 1
			printPush(3); // push 3
			codeStatement(tree.children[2], symbolTable); // code expr 2
			printPop(5); // pop 5
			cout << sltType << " $6, $3, $5" << endl; // $6 = $3 < $5
			cout << sltType << " $7, $5, $3" << endl; // $7 = $5 < $3
			cout << "add $3, $6, $7" << endl; // calculated expr NE expr
			cout << "sub $3, $11, $3" << endl; // inverse to EQ
		}
		
		// test expr LE expr
		else if (tree.children[1].type == "LE") {
			codeStatement(tree.children[0], symbolTable); // code expr 1
			printPush(3); // push 3
			codeStatement(tree.children[2], symbolTable); // code expr 2
			printPop(5); // pop 5
			cout << sltType << " $3, $3, $5" << endl; // caluclated expr GT expr
			cout << "sub $3, $11, $3" << endl; // inverse to LE
		}

		// test expr GE expr
		else if (tree.children[1].type == "GE") {
			codeStatement(tree.children[0], symbolTable); // code expr 1
			printPush(3); // push 3
			codeStatement(tree.children[2], symbolTable); // code expr 2
			printPop(5); // pop 5
			cout << sltType << " $3, $5, $3" << endl; // caluclated expr LT expr
			cout << "sub $3, $11, $3" << endl; // inverse to GE
		}

		// test expr GT expr
		else if (tree.children[1].type == "GT") {
			codeStatement(tree.children[0], symbolTable); // code expr 1
			printPush(3); // push 3
			codeStatement(tree.children[2], symbolTable); // code expr 2
			printPop(5); // pop 5
			cout << sltType << " $3, $3, $5" << endl;
		}
	}

	// lvalue is coded in the rules of other statements as the two outputs differ for lvalue's two rules
}

void findDcls (Tree &dcls, unordered_map<string, pair<string, int>> &symbolTable, int &frameOffset, int &offsetCounter) {
	if (dcls.children.size() > 0) {
		findDcls(dcls.children[0], symbolTable, frameOffset, offsetCounter);

		pair<string, string> dclInfo;
		dclInfo = getDcl(dcls.children[1]); // (type, id)
		symbolTable.insert(make_pair(dclInfo.second, make_pair(dclInfo.first, frameOffset)));

		// dcls dcls dcl BECOMES NUM SEMI
		if (dclInfo.first == "int") {
			string value = dcls.children[3].value;
			cout << "lis $3" << endl;
			cout << ".word " << value << endl;
			cout << "sw $3, " << frameOffset << "($29) ; push variable " << dclInfo.second << endl;
		}

		// dcls dcls dcl BECOMES NULL SEMI
		else {
			cout << "sw $11, " << frameOffset << "($29) ; push variable " << dclInfo.second << endl;
		}
		
		cout << "sub $30, $30, $4 ; update stack pointer" << endl;
		frameOffset = frameOffset - 4;
		offsetCounter++;
	}
}

// codes procedures
void codeProcedures(Tree &tree) {
	// procedure
	if (tree.type == "procedure") {
		// procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
		cout << tree.children[1].value << ": ; label same as name of WLP4 procedure" << endl;
		cout << "sub $29, $30, $4 ; assuming caller saves old frame pointer" << endl;

		// push registers to save
		printPush(5); // push($5)

		// create symbol table
		unordered_map<string, pair<string, int>> symbolTable;

		cout << "; params below" << endl;

		// code parameters, if there are any
		int numParams = 0;

		if (tree.children[3].children.size() > 0) {
			Tree paramlist = tree.children[3].children[0];

			// find number of parameters
			while (paramlist.children.size() == 3) {
				numParams++;
				paramlist = paramlist.children[2];
			}
			numParams++;

			// paramlist dcl COMMA paramlist
			int paramNumber = 1;
			paramlist = tree.children[3].children[0];
			while (paramlist.children.size() == 3) {
				pair<string, string> dclInfo;
				dclInfo = getDcl(paramlist.children[0]); // (type, id)
				int paramOffset = 4 * (numParams - paramNumber + 1);
				symbolTable.insert(make_pair(dclInfo.second, make_pair(dclInfo.first, paramOffset)));
				paramNumber++;
				paramlist = paramlist.children[2];
			}

			// paramlist dcl
			pair<string, string> dclInfo;
			dclInfo = getDcl(paramlist.children[0]); // (type, id)
			symbolTable.insert(make_pair(dclInfo.second, make_pair(dclInfo.first, 4)));
			paramNumber++;
		}

		cout << "; dcls below" << endl;

		// code dcls
		int frameOffset = -4; // accounts for register 5 being saved
		int offsetCounter = 0;
		findDcls(tree.children[6], symbolTable, frameOffset, offsetCounter);

		cout << "; statements below" << endl;

		// code statements
		codeStatement(tree.children[7], symbolTable);

		// code return expression
		codeStatement(tree.children[9], symbolTable);

		cout << "; offsets below" << endl;

		for (int i = 0; i < offsetCounter; i++) cout << "add $30, $30, $4" << endl;
		// pop saved registers
		printPop(5); // pop($5)
		cout << "add $30, $29, $4 ; reset stack frame" << endl;
		cout << "jr $31" << endl;
	}

	// main 
	else if (tree.type == "main") {
		// main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE

		// print prologue
		cout << "; begin prologue" << endl;
		cout << ".import print" << endl;
		cout << ".import init" << endl;
		cout << ".import new" << endl;
		cout << ".import delete" << endl;
		cout << "lis $4 ; convention: $4 always contains 4" << endl;
		cout << ".word 4" << endl;
		cout << "lis $10 ; convention: $10 always contains print" << endl;
		cout << ".word print" << endl;
		cout << "lis $11 ; convention: $11 always contains 1" << endl;
		cout << ".word 1" << endl;
		cout << "sub $29, $30, $4 ; setup frame pointer" << endl;
		cout << "; end prologue" << endl;

		// create symbol table, unordered_map<id, pair<type, offset (from $29)>>
		unordered_map<string, pair<string, int>> symbolTable;
		int frameOffset = 0;
		int offsetCounter = 0;
		cout << "; reserve space for wain's variables" << endl;
		
		// add wain's two parameters to symbol table, print parameters
		pair<string, string> dclInfo;

		// first parameter
		dclInfo = getDcl(tree.children[3]);
		symbolTable.insert(make_pair(dclInfo.second, make_pair(dclInfo.first, frameOffset)));
		cout << "sw $1, " << frameOffset << "($29) ; push variable a" << endl;
		cout << "sub $30, $30, $4 ; update stack pointer" << endl;
		frameOffset = frameOffset - 4;
		offsetCounter++;

		// check if pointer for heap allocator initalization
		bool initArray = false;
		if (dclInfo.first == "int*") initArray = true;

		// second parameter
		dclInfo = getDcl(tree.children[5]);
		symbolTable.insert(make_pair(dclInfo.second, make_pair(dclInfo.first, frameOffset)));
		cout << "sw $2, " << frameOffset << "($29) ; push variable b" << endl;
		cout << "sub $30, $30, $4 ; update stack pointer" << endl;
		frameOffset = frameOffset - 4;
		offsetCounter++;

		// initalize heap allocator's internal data structures
		if (initArray) {
			printPush(2); // push $2
			printPush(31); // push $31
			cout << "lis $5" << endl;
			cout << ".word init" << endl;
			cout << "jalr $5" << endl;
			printPop(31); // pop $31
			printPop(2); // pop $2
		}
		else {
			printPush(2); // push $2
			cout << "add $2, $0, $0 ; reset $2 to 0" << endl;
			printPush(31); // push $31
			cout << "lis $5" << endl;
			cout << ".word init" << endl;
			cout << "jalr $5" << endl;
			printPop(31); // pop $31
			printPop(2); // pop $2
		}

		// code dcls
		findDcls(tree.children[8], symbolTable, frameOffset, offsetCounter);

		// code statements
		cout << "; translated WLP4 code" << endl;
		codeStatement(tree.children[9], symbolTable);

		// code return
		codeStatement(tree.children[11], symbolTable);

		// print epilogue
		cout << "; begin epilogue" << endl;
		for (int i = 0; i < offsetCounter; i++) cout << "add $30, $30, $4" << endl;
		cout << "jr $31" << endl;
	}
	// procedures
	else {
		if (tree.children.size() > 1) codeProcedures(tree.children[1]); // code procedures first so wain is printed first
		codeProcedures(tree.children[0]); // procedures will be printed in reverse but this does not really matter
	} 
}

int main() {
	// create tree from WLP4I input
	Tree root = createTree();
	
	// root has children {BOF, procedures, EOF}, program is contained within procedures, children[1]
	Tree procedures = root.children[1];

	codeProcedures(procedures);
	return 0;
}