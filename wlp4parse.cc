#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <tuple>
#include <vector>

using namespace std;

class Node { 
    public:
    string kind;
	vector<string> output;
    vector<Node> children;
}; 

// finds a reduce action, returns the rule number accordingly
int findReduce(vector<tuple<int, string, string, int>> actions, int state, string symbol) {
	for (auto & action : actions) {
		if (state == get<0>(action) && get<1>(action) == symbol && get<2>(action) == "reduce") return get<3>(action);
	}
	return -1;
}

// finds a shift action, returns the correct state
int findShift(vector<tuple<int, string, string, int>> actions, int state, string symbol) {
	for (auto & action : actions) {
		if (state == get<0>(action) && get<1>(action) == symbol && get<2>(action) == "shift") return get<3>(action);
	}
	return -1;
}

void printNode(Node n) {
	cout << n.output[0];
	for (int i = 1; i < n.output.size(); i++) cout << ' ' << n.output[i];
	cout << endl;
	for (int i = 0; i < n.children.size(); i++) printNode(n.children[i]);
}

int main() {
	ifstream WLP4("wlp4.txt");

	// read terminal symbols
	int numTerminals;
	WLP4 >> numTerminals;
	vector<string> terminals;
	terminals.resize(numTerminals);
	for (int i = 0; i < numTerminals; i++) WLP4 >> terminals[i];

	// read non-terminal symbols
	int numNonTerminals;
	WLP4 >> numNonTerminals;
	vector<string> nonTerminals;
	nonTerminals.resize(numNonTerminals);
	for (int i = 0; i < numNonTerminals; i++) WLP4 >> nonTerminals[i];

	// read start symbol
	string start;
	WLP4 >> start;

	// read production rules into vectors
	int numRules;
	int numStart = 0;
	vector<vector<string>> rules;
	vector<string> startRule;
	WLP4 >> numRules;
	string ignore, lineStr, firstWord, temp;
	getline(WLP4, ignore);
	for (int i = 0; i < numRules; i++) {
		getline(WLP4, lineStr);
		istringstream line(lineStr);
		string word;
		vector<string> rule;
		line >> firstWord;
		rule.push_back(firstWord);
		while (line >> word) rule.push_back(word);
		if (firstWord == start) {
			numStart++;
			for (int i = 0; i < rule.size(); i++) startRule.push_back(rule[i]);
			if (numStart > 1) {
				cerr << "ERROR: start symbol occurs on LHS of more than one rule" << word << endl;
				return 1;
			}
		}
		rules.push_back(rule);
	} // CFG has been intialized

	// read LR(1) DFA
	int numStates, numActions, a1, a4;
	string a2, a3;
	WLP4 >> numStates;
	WLP4 >> numActions;
	getline(WLP4, ignore);
	vector<tuple<int, string, string, int>> actions;
	for (int i = 0; i < numActions; i++) {
		getline(WLP4, lineStr);
		istringstream line(lineStr);
		line >> a1;
		line >> a2;
		line >> a3;
		line >> a4;
		actions.push_back(make_tuple(a1, a2, a3, a4));
	}

	// sequence
	vector<pair<string, string>> sequence;
	sequence.push_back(make_pair("BOF", "BOF"));
	string kind, lexeme;
	while(cin >> kind) {
		cin >> lexeme;
		sequence.push_back(make_pair(kind, lexeme));
	}
	sequence.push_back(make_pair("EOF", "EOF"));

	// create stacks
	stack<int> stateStack;
	stack<Node> symStack;
	stateStack.push(0);

	// LR(1) algorithm
	int k = 0;
	for (auto & symbol : sequence) {
		k++;
		while (findReduce(actions, stateStack.top(), symbol.first) > 0) {
			int ruleNum = findReduce(actions, stateStack.top(), symbol.first);
			vector<string> rule = rules[ruleNum];
			int sizeRHS = rule.size() - 1;
			Node newSym;
			newSym.kind = rule[0];
			newSym.output = rule;
			for (int i = 0; i < sizeRHS; i++) {
				stateStack.pop();
				auto it = newSym.children.begin();
				Node popped = symStack.top();
				symStack.pop();
				newSym.children.insert(it, popped);
			}
			symStack.push(newSym);
			stateStack.push(findShift(actions, stateStack.top(), rule[0]));
		}
		Node newSym;
		newSym.kind = symbol.first;
		vector<string> shift{symbol.first, symbol.second};
		newSym.output = shift;
		symStack.push(newSym);
		if (findShift(actions, stateStack.top(), symbol.first) < 0) {
			cerr << "ERROR at " << k - 1 << endl;
			return 1;
		}
		stateStack.push(findShift(actions, stateStack.top(), symbol.first));
	}
	Node newSym;
	newSym.kind = start;
	newSym.output = startRule;
	for (int i = 0; i < 3; i++) {
		auto it = newSym.children.begin();
		Node popped = symStack.top();
		symStack.pop();
		newSym.children.insert(it, popped);
	}
	
	// print outputs
	printNode(newSym);
	return 0;
}