#include "assembler.h"
#include <sstream>
#include <limits>
#include <unordered_map>



struct AssemblyBuildState
{
    std::unordered_map<std::string, unsigned int> &label_to_index_map;
    std::unordered_map<std::string, unsigned int> &const_to_index_map;
};


static Op check_Op(const std::string &str)
{
    if (str == "add" || str == "ADD")
        return Op::ADD;
    else if (str == "SUB" || str == "sub")
        return Op::SUB;
    else if (str == "OR"  || str ==  "or")
        return Op::OR;
    else if (str == "NOT" || str == "not")
        return Op::NOT;
    else if (str == "AND" || str == "and")
        return Op::AND;
    else if (str == "XOR" || str == "xor")
        return Op::XOR;
    else if (str == "MUL" || str == "mul")
        return Op::MUL;
    else if (str == "JE"  || str ==  "je")
        return Op::JE;
    else if (str == "JNE" || str == "jne")
        return Op::JNE;
    else if (str == "JLT" || str == "jlt")
        return Op::JLT;
    else if (str == "JLE" || str == "jle")
        return Op::JLE;
    else if (str == "JGT" || str == "jgt")
        return Op::JGT;
    else if (str == "JGE" || str == "jge")
        return Op::JGE;
    else
        return Op::NONE;
}

static bool check_Identifier(const char *str, size_t num_chars)
{
    if (!num_chars)
        return false;

    char c = str[0];
    if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && c != '.' && c != '_')
    {
        return false;
    }

    for (size_t i = 1; i < num_chars; ++i)
    {
        c = str[i];
        if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '.' && c != '_')
        {
            return false;
        }
    }

    return true;
}

static bool check_Label(std::string &str)
{
    if (str.back() == ':')
    {
        return check_Identifier(str.c_str(), str.size() - 1);
    }
    else
    {
        return false;
    }
}


static Instruction parse_Instruction(std::istream &code, AssemblyBuildState &build_state)
{
    Instruction instruction;

    enum Expect
    {
        EXPECT_LABEL = 1,
        EXPECT_CONST_DEF = 2,
        EXPECT_OP = 4,
        EXPECT_REG = 8,
        EXPECT_ADDR = 16,
        EXPECT_IMM = 32,
        EXPECT_IDF = 64
    };

    static constexpr Expect expect_beg = Expect(EXPECT_LABEL | EXPECT_CONST_DEF | EXPECT_OP);
    static constexpr Expect expect_src = Expect(EXPECT_REG | EXPECT_ADDR | EXPECT_IMM | EXPECT_IDF);
    static constexpr Expect expect_dst = Expect(EXPECT_REG | EXPECT_ADDR | EXPECT_IDF);


    enum ReadState
    {
        READ_NONE,
        READ_LABEL,
        READ_CONST_DEF,
        READ_OP,
        READ_SRC1,
        READ_SRC2,
        READ_DST
    };


    Expect expect = expect_beg;
    ReadState read_state = READ_NONE;

    while (code)
    {
        std::string word;
        code >> word;

        if (word[0] == ';') // comment ; hit
        {
            code.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (expect & EXPECT_OP)
        {
            Op op = check_Op(word);
            if (op != Op::NONE)
            {
                instruction.op = op;
                expect = expect_src;
                read_state = READ_OP;
            }
        }
        else if (expect & EXPECT_LABEL)
        {
            if (check_Label(word))
            {
                auto found_it = build_state.label_to_index_map.find(word);
            }
        }
    }

    return instruction;
}


Assembly parse_Assembly(std::istream &code)
{
    Assembly assembly;

    while (code)
    {
        std::string line;
        std::getline(code, line);

        if (line.empty())
        {
            continue;
        }

        std::istringstream line_ss {line};
    }

    return assembly;
}


void assemble(std::istream &code, std::ostream &output)
{
}
