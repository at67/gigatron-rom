#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "expression.h"


namespace Expression
{
    char* _expressionToParse;
    char* _expression;

    int _lineNumber = 0;


    bool _binaryChars[256]      = {false};
    bool _octalChars[256]       = {false};
    bool _decimalChars[256]     = {false};
    bool _hexaDecimalChars[256] = {false};

    unaryOpFuncPtr  _negFunc;
    binaryOpFuncPtr _addFunc;
    binaryOpFuncPtr _subFunc;
    binaryOpFuncPtr _mulFunc;
    binaryOpFuncPtr _divFunc;

    // Default operators
    Numeric neg(const Numeric& numeric)
    {
        Numeric result;
        result._value = -numeric._value;
        return result;
    }
    Numeric add(Numeric& result, const Numeric& numeric)
    {
        result._value += numeric._value;
        return result;
    }
    Numeric sub(Numeric& result, const Numeric& numeric)
    {
        result._value -= numeric._value;
        return result;
    }
    Numeric mul(Numeric& result, const Numeric& numeric)
    {
        result._value *= numeric._value;
        return result;
    }
    Numeric div(Numeric& result, const Numeric& numeric)
    {
        result._value = (numeric._value == 0) ? 0 : result._value / numeric._value;
        return result;
    }

    // Set operators
    void setNegFunc(unaryOpFuncPtr  negFunc) {_negFunc = negFunc;}
    void setAddFunc(binaryOpFuncPtr addFunc) {_addFunc = addFunc;}
    void setSubFunc(binaryOpFuncPtr subFunc) {_subFunc = subFunc;}
    void setMulFunc(binaryOpFuncPtr mulFunc) {_mulFunc = mulFunc;}
    void setDivFunc(binaryOpFuncPtr divFunc) {_divFunc = divFunc;}

    void setDefaultOperatorFuncs(void)
    {
        setNegFunc(neg);
        setAddFunc(add);
        setSubFunc(sub);
        setMulFunc(mul);
        setDivFunc(div);
    }


    Numeric expression(void); // forward declaration


    void initialise(void)
    {
        bool* b = _binaryChars;      b['0']=1; b['1']=1;
        bool* o = _octalChars;       o['0']=1; o['1']=1; o['2']=1; o['3']=1; o['4']=1; o['5']=1; o['6']=1; o['7']=1;
        bool* d = _decimalChars;     d['0']=1; d['1']=1; d['2']=1; d['3']=1; d['4']=1; d['5']=1; d['6']=1; d['7']=1; d['8']=1; d['9']=1;
        bool* h = _hexaDecimalChars; h['0']=1; h['1']=1; h['2']=1; h['3']=1; h['4']=1; h['5']=1; h['6']=1; h['7']=1; h['8']=1; h['9']=1; h['A']=1; h['B']=1; h['C']=1; h['D']=1; h['E']=1; h['F']=1;

        setDefaultOperatorFuncs();
    }

    ExpressionType isExpression(const std::string& input)
    {
        if(find_if(input.begin(), input.end(), isalpha) != input.end()) return HasAlpha;
        if(input.find_first_of("[]") != std::string::npos) return Invalid;
        if(input.find("++") != std::string::npos) return Invalid;
        if(input.find("--") != std::string::npos) return Invalid;
        if(input.find_first_of("+-*/()") != std::string::npos) return Valid;
        return None;
    }

    void padString(std::string &str, int num, char pad)
    {
        if(num > str.size()) str.insert(0, num - str.size(), pad);
    }

    void addString(std::string &str, int num, char add)
    {
        if(num > 0) str.append(num, add);
    }

    void stripWhitespace(std::string& input)
    {
        input.erase(remove_if(input.begin(), input.end(), isspace), input.end());
    }

    std::string byteToHexString(uint8_t n)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)n;
        return "0x" + ss.str();
    }

    std::string wordToHexString(uint16_t n)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(4) << n;
        return "0x" + ss.str();
    }

    std::string& strToUpper(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {return toupper(c);} );
        return s;
    }

    NumericType getBase(const std::string& input, long& result)
    {
        bool success = true;
        std::string token = input;
        strToUpper(token);
        
        // Hex
        if(token.size() >= 2  &&  token.c_str()[0] == '$')
        {
            for(int i=1; i<token.size(); i++) success &= _hexaDecimalChars[token.c_str()[i]];
            if(success)
            {
                result = strtol(&token.c_str()[1], NULL, 16);
                return HexaDecimal; 
            }
        }
        // Hex
        else if(token.size() >= 3  &&  token.c_str()[0] == '0'  &&  token.c_str()[1] == 'X')
        {
            for(int i=2; i<token.size(); i++) success &= _hexaDecimalChars[token.c_str()[i]];
            if(success)
            {
                result = strtol(&token.c_str()[2], NULL, 16);
                return HexaDecimal; 
            }
        }
        // Octal
        else if(token.size() >= 3  &&  token.c_str()[0] == '0'  &&  (token.c_str()[1] == 'O' || token.c_str()[1] == 'Q'))
        {
            for(int i=2; i<token.size(); i++) success &= _octalChars[token.c_str()[i]];
            if(success)
            {
                result = strtol(&token.c_str()[2], NULL, 8);
                return Octal; 
            }
        }
        // Binary
        else if(token.size() >= 3  &&  token.c_str()[0] == '0'  &&  token.c_str()[1] == 'B')
        {
            for(int i=2; i<token.size(); i++) success &= _binaryChars[token.c_str()[i]];
            if(success)
            {
                result = strtol(&token.c_str()[2], NULL, 2);
                return Binary; 
            }
        }
        // Decimal
        else
        {
            for(int i=0; i<token.size(); i++) success &= _decimalChars[token.c_str()[i]];
            if(success)
            {
                result = strtol(&token.c_str()[0], NULL, 10);
                return Decimal; 
            }
        }

        return BadBase;
    }

    bool stringToU8(const std::string& token, uint8_t& result)
    {
        if(token.size() < 1  ||  token.size() > 10) return false;

        long lResult;
        NumericType base = getBase(token, lResult);
        if(base == BadBase) return false;

        result = uint8_t(lResult);
        return true;
    }

    bool stringToU16(const std::string& token, uint16_t& result)
    {
        if(token.size() < 1  ||  token.size() > 18) return false;

        long lResult;
        NumericType base = getBase(token, lResult);
        if(base == BadBase) return false;

        result = uint16_t(lResult);
        return true;
    }

    char peek(void)
    {
        return *_expression;
    }

    char get(void)
    {
        return *_expression++;
    }

    bool number(uint16_t& value)
    {
        char uchr;

        std::string valueStr;
        uchr = toupper(peek());
        valueStr.push_back(uchr); get();
        uchr = toupper(peek());
        if((uchr >= '0'  &&  uchr <= '9')  ||  uchr == 'X'  ||  uchr == 'B'  ||  uchr == 'O'  ||  uchr == 'Q')
        {
            valueStr.push_back(uchr); get();
            while((peek() >= '0'  &&  peek() <= '9')  ||  (peek() >= 'A'  &&  peek() <= 'F'))
            {
                valueStr.push_back(get());
            }
        }

        return stringToU16(valueStr, value);
    }

    Numeric factor(uint16_t defaultValue)
    {
        uint16_t value = 0;
        if((peek() >= '0'  &&  peek() <= '9')  ||  peek() == '$')
        {
            if(!number(value))
            {
                fprintf(stderr, "Expression::factor() : Bad numeric data in '%s' on line %d\n", _expressionToParse, _lineNumber + 1);
                value = 0;
            }
            return Numeric(value, false, nullptr);
        }
        else if(peek() == '(')
        {
            get();
            Numeric numeric = expression();
            //if(peek() != ')')
            //{
            //    fprintf(stderr, "Expression::factor() : Expecting ')' : found '%c' in '%s' on line %d\n", peek(), _expressionToParse, _lineNumber + 1);
            //    value = 0;
            //}
            get();
            return numeric;
        }
        else if(peek() == '-')
        {
            get();
            return _negFunc(factor(0));
        }

        //fprintf(stderr, "Expression::factor() : Unknown character '%c' in '%s' on line %d : returning default %d\n", peek(), _expressionToParse, _lineNumber + 1, defaultValue);
        Numeric numeric = Numeric(defaultValue, true, _expression);
        while(isalpha(peek())) get();
        return numeric;
    }

    Numeric term(void)
    {
        Numeric result = factor(0);
        while(peek() == '*'  ||  peek() == '/')
        {
            if(get() == '*')
            {
                result = _mulFunc(result, factor(0));
            }
            else
            {
                Numeric f = factor(0);
                if(f._value == 0)
                {
                    result = _mulFunc(result, f);
                }
                else
                {
                    result = _divFunc(result, f);
                }
            }
        }

        return result;
    }

    Numeric expression(void)
    {
        Numeric result = term();
        while(peek() == '+' || peek() == '-')
        {
            if(get() == '+')
            {
                result = _addFunc(result, term());
            }
            else
            {
                result = _subFunc(result, term());
            }
        }

        return result;
    }

    uint16_t parse(char* expressionToParse, int lineNumber)
    {
        _expressionToParse = expressionToParse;
        _expression = expressionToParse;
        _lineNumber = lineNumber;

        return expression()._value;
    }
}