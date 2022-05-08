#include <string>
#include <map>
#include <vector>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
// 
// jsonChomper: Bunny json parsing.
//      https://github.com/L3pu5/jsonChomper
//      By L3pu5, L3pu5_Hare, Lepus Hare.
//

namespace jsonChomper {

    enum JSON_VALUE_TYPE {STRINGTYPE, NUMBERTYPE, OBJECTTYPE, ARRAYTYPE, BOOLTYPE, NULLTYPE};
    enum EXPRESSION_TYPE {LITERAL, CONCAT, OBJECT, ASSIGN};
    
    //In JSON a value can be: string, number, object, array, true, false, or null
    struct Value {
        JSON_VALUE_TYPE type;
        int size = 0;
        intptr_t ptr = 0;
        
        public:
            Value(){};


            Value(JSON_VALUE_TYPE _type, int _size, intptr_t _ptr){
                type = _type; 
                size = _size;
                ptr = _ptr;
            }
    };

    struct Key{
        //This might throw
        const char* key;

        public:
            Key(){};

            Key(char* _key){
                key = _key;
            }
    };



    struct JsonElement {
        Key key;
        Value value;

        public:
            JsonElement(){}
    };

    struct JsonBody {
        //The child elements inside the JsonBody
        std::vector<JsonElement> data;

        void AddElement(JsonElement _element){
            data.push_back(_element);
        }
    };

    //JSON tokens
    enum TOKEN_TYPE {
        // Symbol literals
        LEFT_BRACKET, RIGHT_BRACKET, LEFT_BRACE, RIGHT_BRACE, COLON, COMMA,
        // Literals
        STRING, INT, BOOL,
        //Buffer
        END,
    };

    struct Token {
        TOKEN_TYPE type;
        intptr_t ptr;

        public:
        Token(){}

        Token(TOKEN_TYPE _type){
            type = _type;
            ptr = 0;
        }

        Token(TOKEN_TYPE _type, intptr_t _ptr){
            type = _type;
            ptr = _ptr;
            }
    };

    struct Expression{
        EXPRESSION_TYPE type;
        public:
            Expression() {};
            Expression(EXPRESSION_TYPE _type){
                type = _type;
            }
    };


    // K -> V ; {v e O, A, S, N, bool}
    struct AssignExpression : Expression
    {
        Expression* left;
        Expression* right;

    public:
        AssignExpression(Expression* _left, Expression* _right) : Expression(ASSIGN){
            left = _left;
            right = _right;
        }
    };

    struct ObjectExpression : Expression {
        std::vector<Expression*> expressions;
        void AcceptExpression(Expression* _expression){
            expressions.push_back(_expression);
        }
        public:
            ObjectExpression(){};

            ObjectExpression(std::vector<Expression*> _expressions) : Expression(OBJECT){
                expressions = _expressions;
            } 

    };

    //NUMBER, STRING, BOOL
    struct LiteralExpression : Expression{
        Token token;
        public:
            LiteralExpression(Token _token) : Expression(LITERAL) { token = _token;};
    };

    // jsonString
    struct JsonString {
        std::string input;
        int currentCharacter = 0;
        std::vector<Token> tokens;
        
        void addToken(Token _token){
            tokens.push_back(_token);
            advance();
        }

        void advance(){
            currentCharacter++;
        }

        char peek(){
            if(currentCharacter == input.length())
                return -1;
            return input[currentCharacter+1];
        }

        void string(){
            std::string _thisString;
            advance(); 
            _thisString += input[currentCharacter];
            while(peek() != -1 && peek() != '"'){
                advance();
                _thisString += input[currentCharacter];
                //_thisString += 'd';
            }

            if(peek() == '"') {
                //
                char * _ptr = (char *) malloc(sizeof(_thisString.c_str()));
                strcpy_s(_ptr, sizeof(_thisString.c_str()), _thisString.c_str());
                addToken( Token(STRING, reinterpret_cast<intptr_t>(_ptr)));
                advance();
                return;
            }

            throw "Unexpected character or EOF for string literal";

            //Read all characters until you reach the next character being \" 
            //At that time, push the STRING literal token with a str pointer;
        }

        bool isDigit(char _character){
            std::cout << "IsDigit()";
            if (_character > '0' && _character < '9') return true;
            return false;
        }

        void integer(){
            // std::cout << "DIGIT";
            // int _start = currentCharacter;
            // while( isDigit(peek())) advance();
            // printf("%d, %d\n", _start, currentCharacter);
            // //std::string _numbers = input.substr(_start, (currentCharacter - _start));
            // printf("%s", _numbers.c_str());
            // int i = std::stoi(_numbers);
            // int * _ptr = (int *) malloc (sizeof(i));
            // *_ptr = i;
            // addToken( Token(STRING, reinterpret_cast<intptr_t>(_ptr)));
        }
    
        void ReadTokens() {
            char _current;
            while(currentCharacter < input.length()){
                //Read the character under the cursor
                _current = input[currentCharacter];
                switch (_current){
                    case '{':
                        addToken(Token(LEFT_BRACE));
                        break;
                    case '}':
                        addToken(Token(RIGHT_BRACE));
                        break;
                    case '"':
                        string();
                        break;
                    case '[':
                        addToken(Token(LEFT_BRACKET));
                        break;
                    case ']':
                        addToken(Token(RIGHT_BRACKET));
                        break;
                    case ',':
                        addToken(Token(COMMA));
                        break;
                    case ':':
                        addToken(Token(COLON));
                        break;
                    default:
                        advance();
                        // if(isDigit(_current))
                        // {
                        //     //std::cout << "Integer";
                        //     //integer();
                        //     advance();
                        // }
                        // else{
                        //     advance();
                        // }
                    break;
                }
            }
            std::cout << "END TOKEN";
            addToken(Token(END));
            std::cout << "TOKENS OVER";
        }
    };
    //-End JsonString

    //TokenReader
    struct TokenConsumer{
        std::vector<Token> tokens;
        std::vector<JsonElement> stack;
        ObjectExpression base_expression;
        int index = 0;
        JsonBody object = JsonBody();


        void RootObject() {
            //ASSUME THAT THE FIRST CHAR WILL  BE ETHIER [] or {}
            if(tokens[0].type == LEFT_BRACKET){
                advance();
                base_expression = ParseArray();
            }
            else{
                advance();
                base_expression = ParseObject();
            }

            std::cout << base_expression.expressions.size() <<" " <<  base_expression.expressions[0] << "\n";
            AssignExpression _object = *(AssignExpression*) base_expression.expressions[0]; 
            ObjectExpression _right = *(ObjectExpression*) _object.right;
            std::cout << _object.right->type << " " << _right.expressions.size();
        }

        ObjectExpression ParseObject(){
            ObjectExpression _objectExpression = ObjectExpression();
            _objectExpression.type = OBJECT;
            while (index < tokens.size()){
                Token _t = tokens[index];
                switch(_t.type){
                    case RIGHT_BRACE:
                        //KILL THIS FUNCITON.
                        return _objectExpression;
                    case LEFT_BRACKET:
                    case RIGHT_BRACKET:
                        printf("Unexpected [] in ParseObject");
                        throw "Unexpected [] in ParseObject";
                    case COMMA: 
                        advance();
                        break;
                    case STRING:
                        if(peek().type == COLON){
                            advance();
                            //{}
                            if(peek().type == LEFT_BRACE){
                                advance();
                                advance();
                                ObjectExpression* _expr = new ObjectExpression;
                                *_expr = ParseObject();
                                _objectExpression.AcceptExpression(new AssignExpression(new LiteralExpression(_t), _expr));
                            }
                            //[]
                            else if(peek().type == LEFT_BRACKET){
                                advance(); 
                                advance(); 
                                ObjectExpression* _expr = new ObjectExpression;
                                *_expr = ParseArray();
                                _objectExpression.AcceptExpression(new AssignExpression(new LiteralExpression(_t), _expr));
                            }
                            // k : V
                            else if (peek().type == STRING || peek().type == INT || peek().type == BOOL){
                                advance();
                                _objectExpression.AcceptExpression(new AssignExpression(new LiteralExpression(_t), new LiteralExpression(tokens[index])));
                                advance();
                            }
                        }
                        break;
                }
            }
            return _objectExpression;
        }

        ObjectExpression ParseArray(){
            ObjectExpression _concatExpression = ObjectExpression();
            _concatExpression.type = CONCAT;
            while (index < tokens.size()){
                Token _t = tokens[index];
                switch(_t.type){
                    case RIGHT_BRACE:
                         printf("Unexpected ] in RIGHT_BRACE");

                        throw "Unexpected RIGHT_BRACE in array";
                    case LEFT_BRACKET:
                        printf("Unexpected ] in LEFT_BRAKCET");
                        throw "Unexpected LEFT_BRACKET in array";
                    case RIGHT_BRACKET:
                        advance();
                        return _concatExpression; 
                    case COMMA: 
                        advance();
                        break;
                    case STRING:
                        if(peek().type == COLON){
                            advance();
                            //{}
                            if(peek().type == LEFT_BRACE){
                                advance();
                                advance();
                                ObjectExpression* _expr = new ObjectExpression;
                                *_expr = ParseObject();
                                _concatExpression.AcceptExpression(new AssignExpression(new LiteralExpression(_t), _expr));
                            }
                            //[]
                            else if(peek().type == LEFT_BRACKET){
                                advance(); 
                                advance(); 
                                ObjectExpression* _expr = new ObjectExpression;
                                *_expr = ParseArray();
                                _concatExpression.AcceptExpression(new AssignExpression(new LiteralExpression(_t), _expr));
                            }
                            // k : V
                            else if (peek().type == STRING || peek().type == INT || peek().type == BOOL){
                                advance();
                                _concatExpression.AcceptExpression(new AssignExpression(new LiteralExpression(_t), new LiteralExpression(tokens[index])));
                                advance();
                            }
                        }
                        //[ A, B, C, D: "asd", E ] 
                        else if (peek().type == COMMA){
                            advance();
                            if (peek().type == STRING || peek().type == INT || peek().type == BOOL){
                                advance();
                                _concatExpression.AcceptExpression(new LiteralExpression(tokens[index]));
                                advance();
                            }
                        }
                        break;
                }
            }
            return _concatExpression;
        }
        void advance(){
            index++;
        }   

        Token peek(){
            if(index+1 > tokens.size()) throw "Token Peek() out of range in Token parser";
            return tokens[index+1];
        }

        public:
            TokenConsumer(std::vector<Token> _tokens){ tokens = _tokens;};
    };
    //-End TokenReader



    std::string read_file(const char* _filePath){
        std::ifstream _stream(_filePath);
        std::stringstream _buffer;
        _buffer << _stream.rdbuf();
        return _buffer.str();
    }

    std::string _testString;
    int main()
    {
        //Read the test case
        _testString = read_file("TestJson.txt");

        JsonString jString = JsonString();
        jString.input = _testString;
        std::cout << "\n" << jString.tokens.size() << "\n";
        jString.ReadTokens();
        std::cout << "\n AFTER THE TOKEN PARSES:" << jString.tokens.size() << "\n";
        TokenConsumer consumer = TokenConsumer(jString.tokens);
        consumer.RootObject();


        // for (size_t i = 0; i < jString.tokens.size(); i++)
        // {
        //     std::cout << i << ": " << jString.tokens.at(i).type << ", " << jString.tokens.at(i).ptr << "\n";
        //     if(jString.tokens.at(i).type == INT || jString.tokens.at(i).type == STRING){
        //         printf("%s\n", jString.tokens.at(i).ptr);
        //     }
        // }
        return 0;
    }
}

int main()
{ 
    return jsonChomper::main();
}

