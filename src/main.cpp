#include "Helpers.h"
#include "json.hpp"

using Json = nlohmann::json;

class CppSetup
{
private:
    struct BuildConfig
    {
        std::string runtimeLibrary;
        std::string exceptionModel;
        std::vector<std::string> defines;
        std::vector<std::string> compilerOptions;
        std::vector<std::string> linkerOptions;
        std::vector<std::string> libPaths;
        std::vector<std::string> includePaths;
    };

    struct Template
    {
        std::string name;
        std::string sourceFiles;
        std::string vscodeFiles;
        BuildConfig debugConfig;
        BuildConfig releaseConfig;
    };

private:
    ArgumentParser parser;
    std::string vsDevCmd;
    std::string rootFolder;
    std::vector<Template> templates;

public:
    CppSetup(int argc, const char **argv) 
    {
        parser = ArgumentParser(argc, argv);
        rootFolder = ComputeRootFolder();
        std::string jsonFile = ReplaceVariables(LoadTemplatesJson());
        Json j = Json::parse(jsonFile);
        vsDevCmd = j["developerCommandPrompt"].get<std::string>();
        templates = ParseTemplates(j["templates"]);
    }

    int ParseArguments()
    {
        if (parser[1] == "new")
            return HandleNew();
        else if (parser[1] == "")
        {
            DisplayUsage();
            return 0;
        }
        else
        {
            SetColor(RED);
            std::cout << "Command '" << parser[1] << "' not found.\n";
            ResetColor();
            return 1;
        }
    }

private:
    std::string ComputeRootFolder()
    {
        std::string execPath = parser[0];
        std::string binDir = execPath.substr(0, execPath.find_last_of("\\"));
        std::string rootDir = binDir.substr(0, binDir.find_last_of("\\"));
        return rootDir;
    }

    std::string LoadTemplatesJson()
    {
        std::ifstream file(rootFolder + "\\templates\\templates.json");

        if (!file.is_open())
        {
            SetColor(RED);
            std::cout << "Could not locate templates.json in '" << (rootFolder + "\\templates") << "'\n";
            ResetColor();
            exit(2);
        }

        std::string line;
        std::string text;

        while (!file.eof())
        {
            std::getline(file, line);
            text += line + "\n";
        }
        file.close();

        return text;
    }

    std::string ReplaceVariables(std::string &text)
    {
        std::string from = "${templateFolder}";
        std::string to = rootFolder + "\\templates";
        ReplaceAll(to, "\\", "\\\\");

        ReplaceAll(text, from, to);
        return text;
    }

    std::vector<Template> ParseTemplates(const Json &json)
    {
        std::vector<Template> result;
        for (auto &t : json)
            result.push_back(ParseTemplate(t));

        return result;
    }

    Template ParseTemplate(const Json &json)
    {
        Template result;

        result.name = json["name"].get<std::string>();
        result.sourceFiles = json["sourceFiles"].get<std::string>();
        result.vscodeFiles = json["vscodeFiles"].get<std::string>();
        result.debugConfig = ParseBuildConfig(json["debugConfig"]);
        result.releaseConfig = ParseBuildConfig(json["releaseConfig"]);

        return result;
    }

    BuildConfig ParseBuildConfig(const Json &json)
    {
        BuildConfig result;

        result.runtimeLibrary = json["runtimeLibrary"].get<std::string>();
        result.exceptionModel = json["exceptionModel"].get<std::string>();
        result.compilerOptions = json["compilerOptions"].get<std::vector<std::string>>();
        result.linkerOptions = json["linkerOptions"].get<std::vector<std::string>>();
        result.libPaths = json["libPath"].get<std::vector<std::string>>();
        result.includePaths = json["includePath"].get<std::vector<std::string>>();
        result.defines = json["defines"].get<std::vector<std::string>>();

        return result;
    }

    std::string GenerateBuildFile(const Template &t)
    {
        using sstream = std::stringstream;
        sstream str;

        str << "@echo off\n\n";
        str << "call \"" << vsDevCmd << "\" > nul\n\n";
        str << "set \"EXE_NAME=%~1\"\n";
        str << "set \"BUILD_CONFIG=%~2\"\n\n";
        str << "setlocal enabledelayedexpansion\n";

        str << "pushd %~dp0..\\src\n";
        str << "for /r \".\" %%i in (*.cpp) do set \"files=!files!%%~fi \"\n";
        str << "popd\n";

        str << "if \"!BUILD_CONFIG!\"==\"release\" ( \n";
        str << "    call:release\n";
        str << ") else (\n";
        str << "    call:debug\n";
        str << ")\n";

        str << "endlocal\n";
        str << "exit /b\n\n";

        str << GenerateBuildConfig("debug", t.debugConfig);
        str << GenerateBuildConfig("release", t.releaseConfig);

        return str.str();
    }

    std::string GenerateBuildConfig(std::string name, const BuildConfig &config)
    {
        using sstream = std::stringstream;
        sstream str;

        str << ":" << name << "\n";
        str << "pushd %~dp0..\\obj\n";

        str << "cl.exe /" << config.exceptionModel << " /" << config.runtimeLibrary << " ";

        str << "/c ";

        for (auto &o : config.compilerOptions)
            str << o << " ";

        for (auto &d : config.defines)
            str << "/D \"" << d << "\" ";

        for (auto &i : config.includePaths)
            str << "/I\"" << i << "\" ";

        str << "%files%\n";

        str << "for /r \".\" %%i in (*.obj) do set \"objs=%objs%%%~fi \"\n";

        str << "link.exe ";

        for (auto &l : config.libPaths)
            str << "/LIBPATH:\"" << l << "\" ";

        for (auto &lo : config.linkerOptions)
            str << lo << " ";

        str << "%objs% ";
        str << "/out:%EXE_NAME%\n";

        str << "popd\n";
        str << "pushd %~dp0..\n";
        str << "copy obj\\%EXE_NAME% bin\\%EXE_NAME%\n";
        str << "popd\n";
        str << "goto:eof\n\n";

        return str.str();
    }

private:
    int HandleNew()
    {
        std::string name = parser[2];

        if (name == "")
        {
            SetColor(RED);
            std::cout << "Error: Missing template name\n";
            ResetColor();
            return 1;
        }

        for (auto &t : templates)
        {
            if (name == t.name)
            {
                InitializeTemplate(t);
                return 0;
            }
        }

        SetColor(RED);
        std::cout << "Error: Unknown template '" << name << "'\n";
        ResetColor();
        return 1;
    }

    void InitializeTemplate(const Template &t)
    {
        system("mkdir src");
        system("mkdir bin");
        system("mkdir obj");
        system("mkdir .vscode");
        system(("xcopy " + t.sourceFiles + " src /q /e").c_str());
        system(("xcopy " + t.vscodeFiles + " .vscode /q /e").c_str());

        std::ofstream buildFile(".vscode\\build.bat");
        buildFile << GenerateBuildFile(t);
        buildFile.flush();
        buildFile.close();
    }

    void DisplayUsage()
    {
        static const char *usage = R"(
Usage: cpps new <Template-Name>

Template-Name:    The name of the template to use
)";
        ResetColor();
        std::cout << usage;
    }
};

int main(int argc, const char **argv)
{
    CppSetup setup(argc, argv);
    return setup.ParseArguments();
}
