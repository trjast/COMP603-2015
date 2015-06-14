/*
= Brainfuck

If you have gcc:

----
g++ -o brainfuck.exe brainfuck.cpp
brainfuck.exe helloworld.bf
----
*/

#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

/**
 * Primitive Brainfuck commands
 */
typedef enum { 
    INCREMENT, // +
    DECREMENT, // -
    SHIFT_LEFT, // <
    SHIFT_RIGHT, // >
    INPUT, // ,
    OUTPUT, // .
    ZERO
} Command;

// Forward references. Silly C++!
class CommandNode;
class Loop;
class Program;

/**
 * Visits?!? Well, that'd indicate visitors!
 * A visitor is an interface that allows you to walk through a tree and do stuff.
 */
class Visitor {
    public:
        virtual void visit(const CommandNode * leaf) = 0;
        virtual void visit(const Loop * loop) = 0;
        virtual void visit(const Program * program) = 0;
};

/**
 * The Node class (like a Java abstract class) accepts visitors, but since it's pure virtual, we can't use it directly.
 */
class Node {
    public:
        virtual void accept (Visitor *v) = 0;
};

// a custom exception for commands that aren't real commands
class CommandNotValidException : virtual public exception {
public:
    CommandNotValidException() : exception("Tried to create a command from an invalid character") {}
};

/**
 * CommandNode publicly extends Node to accept visitors.
 * CommandNode represents a leaf node with a primitive Brainfuck command in it.
 */
class CommandNode : public Node {
    public:
        Command command;
        int count;
        CommandNode(char c, int count = 1) {
            switch(c) {
                case '+': command = INCREMENT; break;
                case '-': command = DECREMENT; break;
                case '<': command = SHIFT_LEFT; break;
                case '>': command = SHIFT_RIGHT; break;
                case ',': command = INPUT; break;
                case '.': command = OUTPUT; break;
                case '0': command = ZERO; break;
                default: throw new CommandNotValidException();
            }
            this->count = count;
        }
        void accept (Visitor * v) {
            v->visit(this);
        }
};

class Container: public Node {
    public:
        vector<Node*> children;
        virtual void accept (Visitor * v) = 0;
};

/**
 * Loop publicly extends Node to accept visitors.
 * Loop represents a loop in Brainfuck.
 */
class Loop : public Container {
    public:
        void accept (Visitor * v) {
            v->visit(this);
        }
};

/**
 * Program is the root of a Brainfuck program abstract syntax tree.
 * Because Brainfuck is so primitive, the parse tree is the abstract syntax tree.
 */
class Program : public Container {
    public:
        void accept (Visitor * v) {
            v->visit(this);
        }
};

/**
 * Read in the file by recursive descent.
 * Modify as necessary and add whatever functions you need to get things done.
 */
void parse(fstream & file, Container * container) {
    char c = '\0';
    Loop* loop = nullptr;
    int multiples = 0;

    while (file >> c)
    {
        switch (c)
        {
        case '+':
        case '-':
        case '<':
        case '>':
        case ',':
        case '.':
            multiples = 1;
            while (file.peek() == c){
                multiples++;
                file >> c;
            }
            container->children.push_back(new CommandNode(c, multiples));
            break;
        case '[':
            loop = new Loop();
            parse(file, loop);
            if (loop->children.size() == 1)
            {
                CommandNode*leaf = (CommandNode*)loop->children[0];
                if (leaf->command == '+' || leaf->command == '-')
                {
                    container->children.push_back(new CommandNode('0', 1));
                    break;
                }
                else
                {
                    container->children.push_back(loop);
                    break;
                }
            }
            container->children.push_back(loop);
            break;
        case ']':
            return;
        }
    }
}

/**
 * A printer for Brainfuck abstract syntax trees.
 * As a visitor, it will just print out the commands as is.
 * For Loops and the root Program node, it walks trough all the children.
 */
class Printer : public Visitor {
    public:
        void visit(const CommandNode * leaf) {
            switch (leaf->command) {
            case INCREMENT:   for (int i = 0; i < leaf->count; i++){
                cout << '+';
            } break;
            case DECREMENT:   for (int i = 0; i < leaf->count; i++){
                cout << '-';
            } break;
            case SHIFT_LEFT:  for (int i = 0; i < leaf->count; i++){
                cout << '<';
            } break;
            case SHIFT_RIGHT: for (int i = 0; i < leaf->count; i++){
                cout << '>';
            } break;
            case INPUT:       for (int i = 0; i < leaf->count; i++){
                cout << ',';
            } break;
            case OUTPUT:      for (int i = 0; i < leaf->count; i++){
                cout << '.';
            } break;
            case ZERO:        for (int i = 0; i < leaf->count; i++){
                cout << '[+]';
            } break;
            }
        }
        void visit(const Loop * loop) {
            cout << '[';
            for (vector<Node*>::const_iterator it = loop->children.begin(); it != loop->children.end(); ++it) {
                (*it)->accept(this);
            }
            cout << ']';
        }
        void visit(const Program * program) {
            for (vector<Node*>::const_iterator it = program->children.begin(); it != program->children.end(); ++it) {
                (*it)->accept(this);
            }
            cout << '\n';
        }
};

// the evaluator. based on http://en.wikipedia.org/wiki/Brainfuck#Commands
class Evaluator : public Visitor {
public:
    // create an evaluator with a limit of memory (overuse throws)
    Evaluator(int maxMemory) : max(maxMemory), pos(0), arr(new unsigned char[maxMemory])
    {
        memset(arr, 0, sizeof(arr));
        ptr = arr;
    }

    // handle all the ops of the commandnode
    void visit(const CommandNode * leaf) {
        switch (leaf->command) {
        case INCREMENT:     for (int i = 0; i < leaf->count; i++){
            ++*ptr;
        } break;
        case DECREMENT:     for (int i = 0; i < leaf->count; i++){
            --*ptr;
        } break;
        case SHIFT_RIGHT:   for (int i = 0; i < leaf->count; i++){
            ++ptr;
        } break;
        case SHIFT_LEFT:    for (int i = 0; i < leaf->count; i++){
            --ptr;
        } break;
        case INPUT:         for (int i = 0; i < leaf->count; i++){
            *ptr = getchar();
        } break;
        case OUTPUT:        for (int i = 0; i < leaf->count; i++){
            putchar(*ptr);
        } break;
        case ZERO:          for (int i = 0; i < leaf->count; i++){
            *ptr = 0;
        } break;
        }
    }

    // handle a loop
    void visit(const Loop * loop) {
        while (*ptr) {
            for (auto it = loop->children.begin(); it != loop->children.end(); ++it) {
                (*it)->accept(this);
            }
        }
    }

    // handle a program
    void visit(const Program * program) {
        for (auto it = program->children.begin(); it != program->children.end(); ++it) {
            (*it)->accept(this);
        }
        cout << '\n';
    }

private:
    unsigned char* ptr; // the instruction pointer
    int pos; // the instruction pointer position (for memory safety checks)
    int max; // the size of memory we have to work in (for memory safety checks)
    unsigned char* arr; // the actual memory we have to work in
};

// the compiler outputs c code
class Compiler : public Visitor {
public:    
    // handle all the ops of the commandnode
    void visit(const CommandNode * leaf) {
        switch (leaf->command) {
        case INCREMENT:     for (int i = 0; i < leaf->count; i++){
            cout << "++*ptr;" << endl;
        } break;
        case DECREMENT:     for (int i = 0; i < leaf->count; i++){
            cout << "--*ptr;" << endl;
        } break;
        case SHIFT_RIGHT:   for (int i = 0; i < leaf->count; i++){
            cout << "++ptr;" << endl;
        } break;
        case SHIFT_LEFT:    for (int i = 0; i < leaf->count; i++){
            cout << "--ptr;" << endl;
        } break;
        case INPUT:         for (int i = 0; i < leaf->count; i++){
            cout << "*ptr = getchar();" << endl;
        } break;
        case OUTPUT:        for (int i = 0; i < leaf->count; i++){
            cout << "putchar(*ptr);" << endl;
        } break;
        case ZERO:          for (int i = 0; i < leaf->count; i++){
            cout << "*ptr = 0;" << endl;
        } break;
        }
    }

    // handle a loop
    void visit(const Loop * loop) {
        cout << "while (*ptr) {" << endl;
        for (auto it = loop->children.begin(); it != loop->children.end(); ++it) {
            (*it)->accept(this);
        }
        cout << "}" << endl;
    }

    // handle a program
    void visit(const Program * program) {
        cout << "#include <stdio.h>" << endl;
        cout << "int main(int argc, char** argv) {" << endl;
        for (auto it = program->children.begin(); it != program->children.end(); ++it) {
            (*it)->accept(this);
        }
        cout << '}' << endl;
    }
};

int main(int argc, char *argv[]) {
    fstream file;
    if (argc == 1) {
        cout << argv[0] << ": No input files." << endl;
    } else if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            Printer printer; // how we write out
            //Compiler compile; // how we compile out
            //Evaluator eval(30000); // how we evaluate - allocate some space
            Program program; // what we parse into

            file.open(argv[i], fstream::in);
            parse(file, & program);

            cout << "SRC:\n";
            program.accept(&printer); // print the source
            //cout << "C CODE:\n";
            //program.accept(&compile);
            //cout << "EVAL:\n";
            //program.accept(&eval); // evaluate the code
            
            file.close();
        }
    }
}