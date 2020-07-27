#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

enum ConsoleColor : WORD
{
    BLACK = 0x0000,
    DARK_BLUE = 0x0001,
    DARK_GREEN = 0x0002,
    DARK_CYAN = 0x0003,
    DARK_RED = 0x0004,
    DARK_MAGENTA = 0x0005,
    DARK_YELLOW = 0x0006,
    GREY = 0x0007,
    DARK_GREY = 0x0008,
    BLUE = 0x0009,
    GREEN = 0x000A,
    CYAN = 0x000B,
    RED = 0x000C,
    MAGENTA = 0x000D,
    YELLOW = 0x000E,
    WHITE = 0x000F,
};

void SetColor(WORD color)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(out, color);
}

void ResetColor() { SetColor(GREY); }

void ReplaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return;
    int start = 0;
    while ((start = str.find(from, start)) != std::string::npos)
    {
        str.replace(start, from.length(), to);
        start += to.length();
    }
}

class ArgumentParser
{
private:
    std::vector<std::string> tokens;

public:
    ArgumentParser(){}

    ArgumentParser(int argc, const char **argv)
    {
        tokens.reserve(argc);
        for (int i = 0; i < argc; i++)
            tokens.push_back(argv[i]);
    }
    inline const std::string &operator[](int index) { return GetArgumentAt(index); }

    const std::string &GetOption(const std::string &option)
    {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(tokens.begin(), tokens.end(), option);
        if (itr != tokens.end() && ++itr != tokens.end())
        {
            return *itr;
        }

        static const std::string empty_string("");
        return empty_string;
    }

    const std::string &GetArgumentAt(int index)
    {
        if (index >= 0 && index < tokens.size())
            return tokens[index];

        static const std::string empty_string("");
        return empty_string;
    }

    bool DoesOptionExist(const std::string &option)
    {
        return std::find(tokens.begin(), tokens.end(), option) != tokens.end();
    }
};
