#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <map>
using namespace std;

map<string, pair<vector<string>, map<string, pair<string, int>>>> table;
map<string, map<string, string>> params;
vector<string> arguments;
string currproc;
int offset = 0;
int looplabel = 0;
int iflabel = 0;

class Failure {
    string message;
  public:
    Failure(string message): message(move(message)) {};

    // Returns the message associated with the exception.
    const std::string &what() const {return message; }
};

class Node {
    public:
    string lhs;
    string rule;
    vector<string> tokens;
    vector<shared_ptr<Node>> children;
    virtual ~Node() {};
};

class Terminal: public Node {
    public:
    Terminal() {
        string input;
        string rhs;
        getline(cin, input);
        rule = input;
        istringstream ss{input};
        ss >> lhs >> rhs;
        tokens.push_back(rhs);
    }
};

bool isLeaf(string lhs) {
    return lhs[0] <= 'Z' && lhs[0] >= 'A';
}

class NonTerminal: public Node {
    public:
    NonTerminal() {
        string token;
        getline(cin, rule);
        istringstream ss{rule};
        ss >> lhs;
        while (ss >> token) {
            tokens.push_back(token);
            if (isLeaf(token)) {
                children.emplace_back(shared_ptr<Node>(new Terminal()));
            } else {
                children.emplace_back(shared_ptr<Node>(new NonTerminal()));
            }
        }
    }
};

bool exist(string target, string which) {
    if (which == "proc" && table.find(target) != table.end()) return true;
    else if (table[currproc].second.find(target) != table[currproc].second.end()) return true;
    return false;
}

void buildTable(shared_ptr<Node> root) {
    if (root->rule == "dcl type ID") {
        string type;
        string name = root->children[1]->tokens[0];
        if (root->children[0]->rule == "type INT") {
            type = "int";
        } else {
            type = "int*";
        }
        if (exist(name, "symbol")) {
            throw Failure("ERROR at type ID");
            return;
        }
        table[currproc].second[name].first = type;
        table[currproc].second[name].second = offset;
        // cout << name << ": " << offset << endl;
        offset -= 4;
    } else if (root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
        currproc = root->children[1]->tokens[0];
        offset = 0;
        if (exist(currproc, "proc")) {
            throw Failure("ERROR at main");
            return;
        }
        table[currproc];
        params[currproc];
        string parameter1 = "";
        string parameter2 = "";
        if (root->children[3]->children[0]->rule == "type INT") {
            parameter1 = "int";
        } else if (root->children[3]->children[0]->rule == "type INT STAR") {
            parameter1 = "int*";
        }
        if (root->children[5]->children[0]->rule != "type INT") {
            throw Failure("ERROR");
            return;
        } else if (root->children[5]->children[0]->rule == "type INT") {
            parameter2 = "int";
        }
        if (parameter1 == "" || parameter2 == "") {
            throw Failure("ERROR at param");
            return;
        }
        table[currproc].first.push_back(parameter1);
        table[currproc].first.push_back(parameter2);
    } else if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
        currproc = root->children[1]->tokens[0];
        if (exist(currproc, "proc")) {
            throw Failure("ERROR");
            return;
        }
        offset = 0;
        table[currproc];
        params[currproc];
    } else if (root->rule == "factor ID" || root->rule == "lvalue ID") {
        string name = root->children[0]->tokens[0];
        if (!exist(name, "symbol")) {
            throw Failure("ERROR");
            return;
        };
    } else if (root->rule == "paramlist dcl" || root->rule == "paramlist dcl COMMA paramlist") {
        string type;
        string name = root->children[0]->children[1]->tokens[0];
        if (root->children[0]->children[0]->rule == "type INT") {
            type = "int";
        } else {
            type = "int*";
        }
        table[currproc].first.push_back(type);
        if (params[currproc].find(name) != params[currproc].end()) {
            throw Failure("ERROR");
        } else {
            params[currproc].insert({name, type});
        }
    } else if (root->rule == "factor ID LPAREN RPAREN" || root->rule == "factor ID LPAREN arglist RPAREN") {
        string procedure = root->children[0]->tokens[0];
        if (!exist(procedure, "proc")) {
            throw Failure("ERROR");
            return;
        }
        if (table[currproc].second.find(procedure) != table[currproc].second.end()) {
            throw Failure("ERROR");
            return;
        }
    }
    for (int i = 0; i < root->children.size(); i++) {
        buildTable(root->children[i]);
    }
}

void printTable(map<string, pair<vector<string>, map<string, pair<string,int>>>> table) {
    for (auto& procedure : table) {
        cerr << procedure.first << ": ";
        int size = procedure.second.first.size();
        for (int i = 0; i < size; i++) {
            cerr << procedure.second.first[i];
            if (i != size - 1) cerr << " ";
        }
        cerr << endl;
            for (auto& symbol: procedure.second.second) {
                cerr << symbol.first << " " << symbol.second.first << endl;
            }
    }
}

string typeOf(shared_ptr<Node> root) {
	if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE" || 
        root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
		currproc = root->children[1]->tokens[0];
	}
	vector<string> types;
	for (vector<shared_ptr<Node>>::iterator it = root->children.begin(); it != root->children.end(); it++) {
		types.push_back(typeOf(*it));
	}
	if (root->rule == "factor NUM") return "int";
	else if (root->rule == "factor NULL") return "int*";
	else if (root->rule == "factor ID" || root->rule == "lvalue ID") {
		return table[currproc].second[root->children[0]->tokens[0]].first;
	} else if (root->rule == "factor LPAREN expr RPAREN" || root->rule == "lvalue LPAREN lvalue RPAREN") {
		return types[1];
	} else if (root->rule == "expr term" || root->rule == "term factor") {
		return types[0];
	} else if (root->rule == "factor AMP lvalue") {
		if (types[1] == "int") return "int*";
        throw Failure("ERROR at &");
		return 0;
	} else if (root->rule == "factor STAR factor" || root->rule == "lvalue STAR factor") {
		if (types[1] == "int*") return "int";
		throw Failure("ERROR at star");
		return 0;
	} else if (root->rule == "factor NEW INT LBRACK expr RBRACK") {
		if (types[3] == "int") return "int*";
		throw Failure("ERROR at new");
		return 0;
	} else if (root->rule == "expr expr PLUS term") {
		string type1 = types[0];
		string type2 = types[2];
		if (type1 == "int" && type2 == "int") return "int";
		else if (type1 == "int*" && type2 == "int") return "int*";
		else if (type1 == "int" && type2 == "int*") return "int*";
		throw Failure("ERROR at +");
		return 0;
	} else if (root->rule == "expr expr MINUS term") {
		string type1 = types[0];
		string type2 = types[2];
		if (type1 == "int" && type2 == "int") return "int";
		else if (type1 == "int*" && type2 == "int") return "int*";
		else if (type1 == "int*" && type2 == "int*") return "int";
		throw Failure("ERROR at ()");
		return 0;
	} else if (root->rule == "term term STAR factor" || root->rule == "term term SLASH factor" || root->rule == "term term PCT factor") {
		string type1 = types[0];
		string type2 = types[2];
		if (type1 == "int" && type2 == "int") return "int";
		throw Failure("ERROR at * / %");
		return 0;
	} else if (root->rule == "factor ID LPAREN RPAREN") {
		string procedure = root->children[0]->tokens[0];
        if (!exist(procedure, "proc") || table[procedure].first.size() != 0) {
            throw Failure("ERROR at proc()");
		    return 0;
        }
		return "int";
	} else if (root->rule == "arglist expr" || root->rule == "arglist expr COMMA arglist") {
        arguments.push_back(types[0]);
        return "well";
	} else if (root->rule == "factor ID LPAREN arglist RPAREN") {
		string procedure = root->children[0]->tokens[0];
        if (!exist(procedure, "proc")) {
            throw Failure("ERROR at proc(args)");
		    return 0;
        }
		vector<string> signatures = table[procedure].first;
        int argsize = arguments.size();
		if (arguments.size() == signatures.size()) {
            for (int i = 0; i < arguments.size(); i++) {
                if (arguments[i] != signatures[argsize-1-i]) {
                    throw Failure("ERROR at arguments not same");
                    return 0;
                }
            }
			arguments.clear();
			return "int";
		}
        throw Failure("ERROR at arguments not same");
        return 0;
    }
    else if (root->rule == "test expr EQ expr" || root->rule == "test expr NE expr" || 
             root->rule == "test expr LT expr" || root->rule == "test expr LE expr" || 
             root->rule == "test expr GE expr" || root->rule == "test expr GT expr") {
		if (types[0] == types[2]) return "well";
        throw Failure("ERROR at comparisons");
        return 0;
	} else if (root->rule == "statement WHILE LPAREN test RPAREN LBRACE statements RBRACE") {
		if (types[2] == "well" && types[5] == "well") return "well";
        throw Failure("ERROR at while");
        return 0;
	} else if (root->rule == "statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE") {
		if (types[2] == "well" && types[5] == "well" && types[9] == "well") return "well";
        throw Failure("ERROR at if else");
        return 0;
	} else if (root->rule == "statement DELETE LBRACK RBRACK expr SEMI") {
		if (types[3] == "int*") return "well";
        throw Failure("ERROR at delete");
        return 0;
	} else if (root->rule == "statement PRINTLN LPAREN expr RPAREN SEMI") {
		if (types[2] == "int") return "well";
		throw Failure("ERROR at print");
        return 0;
	} else if (root->rule == "statement lvalue BECOMES expr SEMI") {
		if (types[0] == types[2]) return "well";
		throw Failure("ERROR at assign");
        return 0;
	} else if (root->rule == "statements") {
		return "well";
	} else if (root->rule == "statements statements statement") {
		if (types[0] == "well" && types[1] == "well") return "well";
		throw Failure("ERROR at statements");
        return 0;
	} else if (root->rule == "dcls") {
		return "well";	
	} else if (root->rule == "dcls dcls dcl BECOMES NUM SEMI") {
		if (root->children[1]->children[0]->rule == "type INT" && types[0] == "well") {
			return "well";
		}
		throw Failure("ERROR at = num");
        return 0;
	} else if (root->rule == "dcls dcls dcl BECOMES NULL SEMI") {
		if (root->children[1]->children[0]->rule == "type INT STAR" && types[0] == "well") {
			return "well";
		}
		throw Failure("ERROR at = pointer");
        return 0;
	} else if (root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
        string decl2 = root->children[5]->children[0]->rule;
        string decls = types[8];
        string s = types[9];
        string e = types[11];
		if (decl2 == "type INT" && decls == "well" && s == "well" && e == "int") {
			return "well";
		}
        throw Failure("ERROR at wain");
        return 0;
	} else if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
		if (types[6] == "well" && types[7] == "well" && types[9] == "int") return "well";
		throw Failure("ERROR at proc");
        return 0;
	}
    return "";
}

void push(int t) {
    cout << "sw $" << t << ", -4($30)" << endl;
    cout << "sub $30, $30, $4" << endl;
}

void pop(int t) {
    cout << "add $30, $30, $4" << endl;
    cout << "lw $" << t << ", -4($30)" << endl;
}

void prologue() {
    cout << "; prologue" << endl;
    cout << ".import print" << endl;
    cout << ".import init" << endl;
    cout << ".import new" << endl;
    cout << ".import delete" << endl;
    cout << "lis $4" << endl;
    cout << ".word 4" << endl;
    cout << "lis $10" << endl;
    cout << ".word print" << endl;
    cout << "lis $11" << endl;
    cout << ".word 1" << endl;
    cout << "sub $29, $30, $4" << endl;
}

void epilogue() {
    cout << "; epilogue" << endl;
    cout << "add $30, $29, $4" << endl;
    cout << "jr $31" << endl;
}

void procedureOffset() {
    for (auto& procedure: table) {
        if (procedure.first != "wain") {
            int n = procedure.second.first.size();
            int update = n * 4;
            // cout << procedure.first << endl;
            for (auto& variable: procedure.second.second) {
                variable.second.second += update;
            }
        }
    }
}

void generateCode(shared_ptr<Node> root, string lparent = "") {
    if (root->rule == "start BOF procedures EOF") {
        generateCode(root->children[1]);
    } else if (root->rule == "procedures main") {
        generateCode(root->children[0]);
    } else if (root->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
        currproc = "wain";
        cout << "sw $1, -4($30)" << endl;
        cout << "sub $30, $30, $4" << endl;
        cout << "sw $2, -4($30)" << endl;
        cout << "sub $30, $30, $4" << endl;
        push(31);
        if (table["wain"].first[0] == "int") {
            cout << "add $2, $0, $0" << endl;
        }
        cout << "lis $5" << endl;
        cout << ".word init" << endl;
        cout << "jalr $5" << endl;
        pop(31);
        generateCode(root->children[8]);
        generateCode(root->children[9]);
        generateCode(root->children[11]);
        epilogue();
    } else if (root->rule == "expr term") {
        generateCode(root->children[0], lparent);
    } else if (root->rule == "term factor") {
        generateCode(root->children[0], lparent);
    } else if (root->rule == "factor ID") {
        generateCode(root->children[0]);
        string name = root->children[0]->tokens[0];
        cout << "lw $3, " << table[currproc].second[name].second << "($29)" << endl;
        // if (lparent == "args") {
        //     cout << "sw $3, " << table[currproc].second[name].second << "($29)" << endl;
        // }
    } else if (root->rule == "factor LPAREN expr RPAREN") {
        generateCode(root->children[1]);
    } else if (root->rule == "expr expr PLUS term" || root->rule == "expr expr MINUS term" ||
               root->rule == "term term STAR factor" || root->rule == "term term SLASH factor" ||
               root->rule == "term term PCT factor") {
        generateCode(root->children[0]);
        push(3);
        generateCode(root->children[2]);
        pop(5);
        if (root->rule == "expr expr PLUS term") {
            string exprtype = typeOf(root->children[0]);
            string termtype = typeOf(root->children[2]);
            if (exprtype == "int" && termtype == "int*") {
                cout << "mult $5, $4" << endl;
                cout << "mflo $5" << endl;
            }
            if (exprtype == "int*" && termtype == "int") {
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
            }
            cout << "add $3, $3, $5" << endl;
        } else if (root->rule == "expr expr MINUS term") {
            string exprtype = typeOf(root->children[0]);
            string termtype = typeOf(root->children[2]);
            if (exprtype == "int" && termtype == "int") {
                cout << "sub $3, $5, $3" << endl;
            } else if (exprtype == "int*" && termtype == "int") {
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
                cout << "sub $3, $5, $3" << endl;
            } else if (exprtype == "int*" && termtype == "int*") {
                cout << "sub $5, $5, $3" << endl;
                cout << "div $5, $4" << endl;
                cout << "mflo $3" << endl;
            }
        } else if (root->rule == "term term STAR factor") {
            cout << "mult $3, $5" << endl;
            cout << "mflo $3" << endl;
        } else if (root->rule == "term term SLASH factor") {
            cout << "div $5, $3" << endl;
            cout << "mflo $3" << endl;
        } else if (root->rule == "term term PCT factor") {
            cout << "div $5, $3" << endl;
            cout << "mfhi $3" << endl;
        }
    } else if (root->rule == "factor NUM") {
        cout << "lis $3" << endl;
        cout << ".word " << root->children[0]->tokens[0] << endl;
    } else if (root->rule == "statements statements statement") {
		generateCode(root->children[0]);
		generateCode(root->children[1]);
	} else if (root->rule == "statement PRINTLN LPAREN expr RPAREN SEMI") {
        push(1);
		generateCode(root->children[2]);
        cout << "add $1, $3, $0" << endl;
        push(31);
        cout << "jalr $10" << endl;
        pop(31);
        pop(1);
    } else if (root->rule == "dcls dcls dcl BECOMES NUM SEMI") {
		generateCode(root->children[0]);
        cout << "lis $3" << endl;
        cout << ".word " << root->children[3]->tokens[0] << endl;
        string name = root->children[1]->children[1]->tokens[0];
        cout << "sw $3, " << table[currproc].second[name].second << "($29)" << endl; 
		cout << "sub $30, $30, $4" << endl;
    } else if (root->rule == "statement lvalue BECOMES expr SEMI") {
        generateCode(root->children[2]);
        generateCode(root->children[0], "statement");
    } else if (root->rule == "factor AMP lvalue") {
        generateCode(root->children[1], "AMP");
    } else if (root->rule == "lvalue LPAREN lvalue RPAREN") {
        generateCode(root->children[1], lparent);
    } else if (root->rule == "lvalue ID") {
        string name = root->children[0]->tokens[0];
        if (lparent == "statement") {
            cout << "sw $3, " << table[currproc].second[name].second << "($29)" << endl;
        } else if (lparent == "AMP") {
            cout << "lis $3" << endl;
			cout << ".word " << table[currproc].second[name].second << endl;
			cout << "add $3, $3, $29" << endl;
        }
    } else if (root->rule == "factor STAR factor") {
        generateCode(root->children[1]);
        cout << "lw $3, 0($3)" << endl;
    } else if (root->rule == "lvalue STAR factor") {
        if (lparent == "AMP") {
            generateCode(root->children[1]);
        } else if (lparent == "statement") {
            push(3);
            generateCode(root->children[1]);
            pop(5);
            cout << "sw $5, 0($3)" << endl;
        }
    } else if (root->rule == "statement WHILE LPAREN test RPAREN LBRACE statements RBRACE") {
        int curr = looplabel;
        looplabel += 1;
        cout << "loop" << curr  << ":" << endl;
        generateCode(root->children[2]);
        cout << "beq $3, $0, " << "endWhile" << curr << endl;
        generateCode(root->children[5]);
        cout << "beq $0, $0, " << "loop" << curr << endl;
        cout << "endWhile" << curr << ":" << endl;
        looplabel += 1;
    } else if (root->rule == "test expr LT expr" || root->rule == "test expr GE expr") {
        generateCode(root->children[0]);
        push(3);
        generateCode(root->children[2]);
        pop(5);
        if (typeOf(root->children[0]) == "int") {
            cout << "slt $3, $5, $3" << endl;
        } else {
            cout << "sltu $3, $5, $3" << endl;
        }
        if (root->rule == "test expr GE expr") {
            cout << "sub $3, $11, $3" << endl;
        }
    } else if (root->rule == "test expr GT expr" || root->rule == "test expr LE expr") {
        generateCode(root->children[0]);
        push(3);
        generateCode(root->children[2]);
        pop(5);
        if (typeOf(root->children[0]) == "int") {
            cout << "slt $3, $3, $5" << endl;
        } else {
            cout << "sltu $3, $3, $5" << endl;
        }
        if (root->rule == "test expr LE expr") {
            cout << "sub $3, $11, $3" << endl;
        }
    } else if (root->rule == "test expr NE expr" || root->rule == "test expr EQ expr") {
        generateCode(root->children[0]);
        push(3);
        generateCode(root->children[2]);
        pop(5);
        if (typeOf(root->children[0]) == "int") {
            cout << "slt $6, $3, $5" << endl;
            cout << "slt $7, $5, $3" << endl;
        } else {
            cout << "sltu $6, $3, $5" << endl;
            cout << "sltu $7, $5, $3" << endl;
        }
        cout << "add $3, $6, $7" << endl;
        if (root->rule == "test expr EQ expr") {
            cout << "sub $3, $11, $3" << endl;
        }
    } else if (root->rule == "statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE") {
        int curr = iflabel;
        iflabel += 1;
        generateCode(root->children[2]);
        cout << "beq $3, $0, else" << curr << endl;
        generateCode(root->children[5]);
        cout << "beq $0, $0, endif" << curr << endl;
        cout << "else" << curr << ":" << endl;
        generateCode(root->children[9]);
        cout << "endif" << curr << ":" << endl;
    } else if (root->rule == "dcls dcls dcl BECOMES NULL SEMI") {
        generateCode(root->children[0]);
        cout << "add $3, $0, $11" << endl;
        string name = root->children[1]->children[1]->tokens[0];
        cout << "sw $3, " << table[currproc].second[name].second << "($29)" << endl; 
		cout << "sub $30, $30, $4" << endl;
    } else if (root->rule == "factor NULL") {
        cout << "add $3, $0, $11" << endl;
    } else if (root->rule == "factor NEW INT LBRACK expr RBRACK") {
        generateCode(root->children[3]);
        cout << "add $1, $3, $0" << endl;
        push(31);
        cout << "lis $5" << endl;
        cout << ".word new" << endl;
        cout << "jalr $5" << endl;
        pop(31);
        cout << "bne $3, $0, 1" << endl;
        cout << "add $3, $11, $0" << endl;
    } else if (root->rule == "statement DELETE LBRACK RBRACK expr SEMI") {
        generateCode(root->children[3]);
        cout << "beq $3, $11, skipDelete" << iflabel << endl;
        cout << "add $1, $3, $0" << endl;
        push(31);
        cout << "lis $5" << endl;
        cout << ".word delete" << endl;
        cout << "jalr $5" << endl;
        pop(31);
        cout << "skipDelete" << iflabel << ":" << endl;
        iflabel += 1;
    } else if (root->rule == "procedures procedure procedures") {
        generateCode(root->children[1]);
        generateCode(root->children[0]);
    } else if (root->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
        string procedure = root->children[1]->tokens[0];
        currproc = procedure;
        cout << "F" << procedure << ":" << endl;
        cout << "sub $29, $30, $4" << endl;
        generateCode(root->children[6]);
        push(1);
        push(2);
        push(5);
        push(6);
        push(7);
        generateCode(root->children[7]);
        generateCode(root->children[9]);
        pop(7);
        pop(6);
        pop(5);
        pop(2);
        pop(1);
        cout << "add $30, $29, $4" << endl;
        cout << "jr $31" << endl;
    } else if (root->rule == "factor ID LPAREN RPAREN") {
        string procedure = root->children[0]->tokens[0];
        push(29);
        push(31);
        cout << "lis $5" << endl;
        cout << ".word F" << procedure << endl; 
        cout << "jalr $5" << endl;
        pop(31);
        pop(29);
    } else if (root->rule == "factor ID LPAREN arglist RPAREN") {
        string procedure = root->children[0]->tokens[0];
        push(29);
        push(31);
        generateCode(root->children[2]);
		cout << "lis $5" << endl;
		cout << ".word F" << procedure << endl;
		cout << "jalr $5" << endl;
        // cout << "-------------------" << endl;
        // cout << procedure << " " << table[procedure].first.size() << endl;
        // cout << "-------------------" << endl;
        int diff = table[procedure].first.size() * 4;
        cout << "lis $5" << endl;
        cout << ".word " << diff << endl;
        cout << "add $30, $30, $5" << endl;
        pop(31);
        pop(29);
    } else if (root->rule == "arglist expr") {
        generateCode(root->children[0]);
        push(3);
    } else if (root->rule == "arglist expr COMMA arglist") {
        generateCode(root->children[0]);
        push(3);
        generateCode(root->children[2]);
    }
}
 
int main() {
    try {
        shared_ptr<NonTerminal> tree = make_shared<NonTerminal> ();
        buildTable(tree);
        // printTable(table);
        // typeOf(tree);
        procedureOffset();
        prologue();
        generateCode(tree);
    } catch (const Failure& e) { 
        std::cerr << e.what() << std::endl; 
    }
}
