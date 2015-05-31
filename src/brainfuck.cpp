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
    OUTPUT // .
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
        CommandNode(char c) {
            switch(c) {
                case '+': command = INCREMENT; break;
                case '-': command = DECREMENT; break;
                case '<': command = SHIFT_LEFT; break;
                case '>': command = SHIFT_RIGHT; break;
                case ',': command = INPUT; break;
                case '.': command = OUTPUT; break;
                default: throw new CommandNotValidException();
            }
        }
        void accept (Visitor * v) {
            v->visit(this);
        }

        // returns if something is a command
        static bool IsCommand(char c) {
            switch (c) {
            case '+':
            case '-':
            case '<':
            case '>':
            case ',':
            case '.': return true;
            default:  return false;
            }
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

        static bool IsStart(char c) {
            return c == '[';
        }

        static bool IsEnd(char c) {
            return c == ']';
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
void parse(fstream & file, Container * root) {
    char c;

    while (file >> c) {
        if (CommandNode::IsCommand(c)) {
            root->children.push_back(new CommandNode(c));
        }
        else if (Loop::IsStart(c)) {
            auto loop = new Loop();
            parse(file, loop);
            root->children.push_back(loop);
        }
        else if (Loop::IsEnd(c)) {
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
                case INCREMENT:   cout << '+'; break;
                case DECREMENT:   cout << '-'; break;
                case SHIFT_LEFT:  cout << '<'; break;
                case SHIFT_RIGHT: cout << '>'; break;
                case INPUT:       cout << ','; break;
                case OUTPUT:      cout << '.'; break;
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

// a custom exception for runtime memory overuse
class EvaluatorRuntimeMemoryUsageException : virtual public exception {
public:
    EvaluatorRuntimeMemoryUsageException() : exception("Runtime used more than Evaluator allotted memory") {}
};

// a custom exception for runtime memory being decreased below zero (TODO: is this allowed? does it loop?)
class EvaluatorRuntimeMemoryDecreaseException : virtual public exception {
public:
    EvaluatorRuntimeMemoryDecreaseException() : exception("Runtime decremented memory below allotted addresses") {}
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
        case INCREMENT:     ++*ptr; break;
        case DECREMENT:     --*ptr; break;
        case SHIFT_RIGHT:   /*if (pos++ < max)*/ ++ptr; /*else throw new EvaluatorRuntimeMemoryUsageException();*/ break;
        case SHIFT_LEFT:    /*if (pos-- >= 0)*/ --ptr; /*else throw new EvaluatorRuntimeMemoryDecreaseException();*/ break;
        case INPUT:         *ptr = getchar(); break;
        case OUTPUT:        putchar(*ptr); break;
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

int main(int argc, char *argv[]) {
    fstream file;
    if (argc == 1) {
        cout << argv[0] << ": No input files." << endl;
    } else if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            Printer printer; // how we write out
            Evaluator eval(1024); // how we evaluate - allocate some space (1024 should be enough for these examples)
            Program program; // what we parse into

            file.open(argv[i], fstream::in);
            parse(file, & program);

            cout << "SRC:\n";
            program.accept(&printer); // print the source
            cout << "EVAL:\n";
            program.accept(&eval); // evaluate the code
            
            file.close();
        }
    }
}