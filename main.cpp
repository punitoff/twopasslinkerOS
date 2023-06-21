#include <ctype.h>
#include <string>
#include <string.h>
#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>


using namespace std;

class Token
{
public:
    int lineNumber;
    int lineOffset;
    string tokenContents;

    void setToken(int lineNo, int lineOff, string tokenStr)
    {
        lineNumber = lineNo;
        lineOffset = lineOff;
        tokenContents = tokenStr;
    }
};

class Symbol
{
public:
    string symbolName;
    int address;
    int moduleNumber;
    bool alreadyDefined = false;
    bool used = false;
};

class Parser
{
private:
    vector<Token> tokens;
    vector<Symbol> symbolTable;
    int moduleSize[100];
    int totalInst = 0;

public:
    void parseError(int errcode, Token tokenVar)
    {
        string errstr[] =
                {
                        "NUM_EXPECTED",
                        "SYM_EXPECTED",
                        "MARIE_EXPECTED",
                        "SYM_TOO_LONG",
                        "TOO_MANY_DEF_IN_MODULE",
                        "TOO_MANY_USE_IN_MODULE",
                        "TOO_MANY_INSTR",
                };
        cout << "Parse Error line " << tokenVar.lineNumber << " offset " << tokenVar.lineOffset << ": " << errstr[errcode] << endl;
        exit(0);
    }

    int readInt(Token tokenVar)
    {
        string s = tokenVar.tokenContents;
        int i;
        try
        {
            i = std::stoi(s);
        }
        catch (std::invalid_argument const &e)
        {
            parseError(0, tokenVar);
        }
        catch (std::out_of_range const &e)
        {
            parseError(0, tokenVar);
        }

        return i;
    }

    string readSymbol(Token tokenVar)
    {
        string s = tokenVar.tokenContents;
        int i = 0;
        if (s.length() < 1)
        {
            parseError(1, tokenVar);
        }
        while (s[i])
        {
            if (i == 0)
            {
                if (!isalpha(s[i]))
                    parseError(1, tokenVar);
            }

            if (!isalnum(s[i]))
            {
                parseError(1, tokenVar);
            }

            i++;
        }
        char* k = &s[0];
        if (strlen(k) > 16)
        {
            parseError(3, tokenVar);
        }
        return s;
    }

    string readIAER(Token tokenVar)
    {
        string s = tokenVar.tokenContents;

        if (s.length() > 1)
        {
            parseError(2, tokenVar);
        }
        if (s[0] != 'I' && s[0] != 'A' && s[0] != 'E' && s[0] != 'R')
        {
            parseError(2, tokenVar);
        }

        return s;
    }

    void tokenizeInputFile(string fileName)
    {
        ifstream file(fileName);
        string line;
        int lineNo = 1;

        if (!file)
        {
            cout << "Unable to open file " << fileName << endl;
            exit(0);
        }

        while (getline(file, line))
        {
            int lineOffset = 1;
            char* str = &line[0];
            char* tok;
            char* remainingLine;
            tok = strtok(str, " \t");

            while (tok != NULL)
            {
                int curTokLength = strlen(tok);
                Token t;
                t.setToken(lineNo, lineOffset, tok);

                tokens.push_back(t);

                remainingLine = strtok(NULL, "");
                tok = strtok(remainingLine, " \t");
                if (tok != NULL)
                {
                    char* pch;
                    pch = strstr(remainingLine, tok);
                    lineOffset = lineOffset + curTokLength + strlen(remainingLine) - strlen(pch) + 1;
                }
            }
            lineNo++;
        }
        Token last;
        if (lineNo == 1 && line.empty()) last.setToken(1, 1, "");
        else last.setToken(lineNo - 1, line.length() + 1, "");
        tokens.push_back(last);

        file.close();
    }

    void parseFirstPass()
    {
        int ct = 0;
        int moduleNo = 0;
        int curBaseAddress = 0;

        while (ct < tokens.size() - 1)
        {
            moduleNo++;
            Token curTok;
            int defcount;
            try
            {
                curTok = tokens.at(ct);
                defcount = readInt(curTok);
                ct++;
            }
            catch (exception E)
            {
                parseError(0, tokens.back());
            }

            if (defcount > 16)
            {
                parseError(4, curTok);
            }

            for (int p = 0; p < defcount; p++)
            {
                Symbol s;

                try
                {
                    Token sym = tokens.at(ct);
                    string k = readSymbol(sym);
                    s.symbolName = k;
                    ct++;
                }
                catch (exception E)
                {
                    parseError(1, tokens.back());
                }

                try
                {
                    Token symAddr = tokens.at(ct);
                    int addr = readInt(symAddr) + curBaseAddress;
                    s.address = addr;
                    s.moduleNumber = moduleNo;
                    ct++;
                }
                catch (exception E)
                {
                    parseError(0, tokens.back());
                }

                int flag = 0;
                for (int d = 0; d < symbolTable.size(); d++)
                {
                    if (symbolTable[d].symbolName.compare(s.symbolName) == 0)
                    {
                        symbolTable[d].alreadyDefined = true;
                        flag = 1;
                        cout << "Warning: Module " << moduleNo << ": " << s.symbolName << " redefinition ignored" << endl;
                        break;
                    }
                }
                if (flag == 0)
                {
                    symbolTable.push_back(s);
                }
            }

            int usecount;
            try
            {
                curTok = tokens.at(ct);
                usecount = readInt(curTok);
                ct++;
            }
            catch (exception E)
            {
                parseError(0, tokens.back());
            }

            if (usecount > 16)
            {
                parseError(5, curTok);
            }

            for (int p = 0; p < usecount; p++)
            {
                try
                {
                    Token sym = tokens.at(ct);
                    string k = readSymbol(sym);
                    ct++;
                }
                catch (exception E)
                {
                    parseError(1, tokens.back());
                }
            }

            int instcount;
            Token insTok;
            try
            {
                insTok = tokens.at(ct);
                instcount = readInt(insTok);
                totalInst += instcount;
                ct++;
            }
            catch (const std::exception& e)
            {
                parseError(0, tokens.back());
            }

            if (totalInst > 512)
            {
                parseError(6, insTok);
            }

            for (int k = 0; k < instcount; k++)
            {
                string addressMode;
                int addr;
                try
                {
                    Token aMode = tokens.at(ct);
                    addressMode = readIAER(aMode);
                    ct++;
                }
                catch (const std::exception& e)
                {
                    parseError(2, tokens.back());
                }

                try
                {
                    Token addrs = tokens.at(ct);
                    addr = readInt(addrs);
                    ct++;
                }
                catch (const std::exception& e)
                {
                    parseError(0, tokens.back());
                }
            }

            curBaseAddress = curBaseAddress + instcount;
            moduleSize[moduleNo] = curBaseAddress;
        }
    }

    void parseSecondPass()
    {
        int ct = 0;
        int moduleNo = 0;
        int curBaseAddress = 0;
        int memoryMap = 0;

        while (ct < tokens.size() - 1)
        {
            moduleNo++;
            Token curTok;
            int defcount;
            try
            {
                curTok = tokens.at(ct);
                defcount = readInt(curTok);
                ct++;
            }
            catch (exception E)
            {
                parseError(0, tokens.back());
            }

            ct = ct + 2 * defcount;

            int usecount;
            try
            {
                curTok = tokens.at(ct);
                usecount = readInt(curTok);
                ct++;
            }
            catch (exception E)
            {
                parseError(0, tokens.back());
            }
            vector<string> useCountSyms;
            vector<int> referredInE;

            for (int p = 0; p < usecount; p++)
            {
                try
                {
                    Token sym = tokens.at(ct);
                    string k = readSymbol(sym);
                    useCountSyms.push_back(k);

                    for (int e = 0; e < symbolTable.size(); e++)
                    {
                        if (symbolTable[e].symbolName.compare(k) == 0)
                        {
                            symbolTable[e].used = true;
                        }
                    }

                    ct++;
                }
                catch (exception E)
                {
                    parseError(1, tokens.back());
                }
            }

            int instcount;
            Token insTok;
            try
            {
                insTok = tokens.at(ct);
                instcount = readInt(insTok);
                ct++;
            }
            catch (const std::exception& e)
            {
                parseError(0, tokens.back());
            }

            if (instcount > 512)
            {
                parseError(6, insTok);
            }

            int memory = 0;
            bool errorS = false;
            string errstr = "";

            for (int k = 0; k < instcount; k++)
            {
                string addressMode;
                int addr;
                try
                {
                    Token aMode = tokens.at(ct);
                    addressMode = readIAER(aMode);
                    ct++;
                }
                catch (const std::exception& e)
                {
                    parseError(2, tokens.back());
                }

                try
                {
                    Token addrs = tokens.at(ct);
                    addr = readInt(addrs);
                    ct++;
                }
                catch (const std::exception& e)
                {
                    parseError(0, tokens.back());
                }

                bool opcodeErr = false;
                int opcode = addr / 1000;
                int operand = addr % 1000;
                int outputAddr = 0;

                if (addr > 9999)
                {
                    opcodeErr = true;
                    errstr = "Error: Illegal opcode; treated as 9999";
                    opcode = 9;
                    operand = 999;
                    outputAddr = 9999;
                }
                else if (addressMode[0] == 'R')
                {
                    opcode = addr / 1000;
                    operand = addr % 1000;

                    if (operand < instcount)
                        outputAddr = curBaseAddress + opcode * 1000 + operand;
                    else
                    {
                        outputAddr = curBaseAddress + opcode * 1000;
                        errstr = "Error: Relative address exceeds module size; relative zero used";
                        errorS = true;
                    }
                }
                else
                {
                    if (addressMode[0] == 'I')
                    {
                        if (addr > 9999)
                        {
                            addr = 9999;
                            errstr = "Error: Illegal immediate value; treated as 9999";
                            errorS = true;
                        }
                        outputAddr = addr;
                    }
                    else if (addressMode[0] == 'E')
                    {
                        string useSym;
                        try
                        {
                            useSym = useCountSyms.at(operand);
                            referredInE.push_back(operand);
                            bool notfound = true;

                            for (int u = 0; u < symbolTable.size(); u++)
                            {
                                Symbol s = symbolTable[u];
                                if (s.symbolName.compare(useSym) == 0)
                                {
                                    int opc = opcode;
                                    int operand = s.address;
                                    outputAddr = (opc * 1000) + operand;
                                    notfound = false;
                                    break;
                                }
                            }

                            if (notfound)
                            {
                                cout << "Error: External operand exceeds length of uselist; treated as relative=0";
                                outputAddr = opcode * 1000 + 0;
                                errorS = true;


                            }
                        }
                        catch (const std::exception& e)
                        {
                            cout << "Error: External operand exceeds length of uselist; treated as relative=0";
                            outputAddr = opcode * 1000 + 0;
                            errorS = true;


                        }
                    }
                    else if (addressMode[0] == 'A')
                    {
                        if (operand < 512)
                        {
                            outputAddr = addr;
                        }
                        else
                        {
                            errstr = "Error: Absolute address exceeds machine size; zero used";
                            outputAddr = opcode * 1000;
                            errorS = true;
                        }
                    }
                }

                cout << setfill('0') << setw(3) << memoryMap;
                memoryMap++;
                cout << ":" << " " << setfill('0') << setw(4) << outputAddr;

                if (opcodeErr)
                {
                    cout << " " << errstr;
                }
                else if (errorS)
                {
                    cout << " " << errstr;
                }

                cout << endl;

                errstr = "";
                errorS = false;
            }

            curBaseAddress = curBaseAddress + instcount;

            for (int g = 0; g < useCountSyms.size(); g++)
            {
                bool found = false;
                for (int n = 0; n < referredInE.size(); n++)
                {
                    if (g == referredInE[n])
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    cout << "Warning: Module " << moduleNo << ": " << useCountSyms[g] << " was not used"  << endl;
                }
            }
        }
    }

    void printSymbolTable()
    {
        cout << "Symbol Table" << endl;

        for (int i = 0; i < symbolTable.size(); i++)
        {
            cout << symbolTable[i].symbolName << "=" << symbolTable[i].address;
            if (symbolTable[i].alreadyDefined)
            {
                cout << " Error: This variable is multiple times defined; first value used";
            }
            cout << endl;
        }
    }

    void printWarnings()
    {
        for (int e = 0; e < symbolTable.size(); e++)
        {
            if (!symbolTable[e].used)
            {
                cout << "Warning: Module " << symbolTable[e].moduleNumber << ": " << symbolTable[e].symbolName << " was defined but never used" << endl;
            }
        }
    }

    void parseInputFile(string fileName)
    {
        tokenizeInputFile(fileName);
        parseFirstPass();
        printSymbolTable();
        cout << endl << "Memory Map" << endl;
        parseSecondPass();
        cout << endl;
        printWarnings();
    }
};

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cout << "Wrong Inputs" << endl;
        exit(0);
    }

    string fileName = argv[1];

    Parser parser;
    parser.parseInputFile(fileName);

    return 0;
}
