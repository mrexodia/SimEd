#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "handle.h"
#include "dynamicmem.h"

using namespace std;

bool maplines(const char* incfile, vector<string>* lines)
{
    Handle hFile=CreateFileA(incfile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(!hFile)
    {
        puts("open fail");
        return false;
    }
    Memory<char*> file(GetFileSize(hFile, 0));
    DWORD read;
    if(!ReadFile(hFile, file, file.size(), &read, 0))
    {
        puts("read fail");
        return false;
    }
    std::string curLine;
    for(unsigned int i=0; i<file.size(); i++)
    {
        if(file[i]=='\r')
            continue;
        if(file[i]=='\n') //newline
        {
            lines->push_back(curLine);
            curLine="";
        }
        else
            curLine+=file[i];
    }
    if(curLine.length())
        lines->push_back(curLine);
    return true;
}

enum STATE
{
    _none,
    _struct,
    _const
};

int repl( string &s, const string &search, const string &replace )
{
    int repltotal=0;
    for( size_t pos = 0; ; pos += replace.length() )
    {
        // Locate the substring to replace
        pos = s.find( search, pos );
        if( pos == string::npos ) break;
        // Replace by erasing and inserting
        s.erase( pos, search.length() );
        s.insert( pos, replace );
        repltotal++;
    }
    return repltotal;
}

bool contains(const string & a, const string & b)
{
    return a.find(b) != string::npos;
}

bool startswith(const string & a, const string & b)
{
    return a.substr(0, b.length())==b;
}

bool convertlines(vector<string>* inclines, vector<string>* hlines)
{
    STATE state=_none;;
    for(unsigned int i=0; i<inclines->size(); i++)
    {
        std::string curLine=inclines->at(i);
        ///generic stuff
        repl(curLine, ";", "//"); //handle comments
        while(startswith(curLine, "\t")) //remove indentation
            curLine.erase(0, 1);
        repl(curLine, "\t", " "); //handle tabbing
        while(repl(curLine, "  ", " ")); //handle duplicate spaces
        repl(curLine, " dd", " DWORD");
        repl(curLine, " dw", " WORD");
        repl(curLine, " db", " BYTE");
        if(startswith(curLine, "//")) //comment line
        {
        }
        else if(contains(curLine, "PROTO")) //ignore PROTO
        {
            curLine.insert(0, "//");
        }
        else if(contains(curLine, ".const")) //ignore .const section
        {
            curLine.insert(0, "//");
        }
        else if(contains(curLine, " equ ")) //equ -> #define
        {
            size_t valstart = curLine.find(" equ ")+5;
            size_t hexid = curLine.substr(valstart).find("h");
            size_t commentid = curLine.substr(valstart).find("//");
            if(hexid!=string::npos && (commentid==string::npos || commentid > hexid))
            {
                string hexval=curLine.substr(valstart, hexid);
                string needle=hexval+"h";
                string replval="0x"+hexval;
                repl(curLine, needle, replval);
            }
            curLine.insert(0, "#define ");
            repl(curLine, " equ ", " "); //remove equ
        }
        else if(state==_none && contains(curLine, " struct")) //struct
        {
            state=_struct;
            curLine="typedef struct {";
        }
        else if(state==_struct && contains(curLine, " ends")) //end of struct
        {
            state=_none;
            repl(curLine, " ends", ";");
            curLine.insert(0, "} ");
        }
        else if(state==_struct) //in struct
        {
            repl(curLine, " <?>", "");
            repl(curLine, " ?", "");
            size_t space=curLine.find(' ');
            string name=curLine.substr(0, space);
            string type=curLine.substr(space+1);
            space=type.find(' ');
            if(space!=string::npos)
                type=type.substr(0, space);
            repl(curLine, name, "\1");
            repl(curLine, type, "\2");
            repl(curLine, "\1", type);
            name+=";";
            repl(curLine, "\2", name);
        }
        else if(contains(curLine, "BYTE")) //ignore BYTE
        {
            curLine.insert(0, "//");
        }
        hlines->push_back(curLine);
    }
    return true;
}

void dputs(FILE* file, const char* text)
{
    fprintf(file, "%s\n", text);
}

int main(int argc, char* argv[])
{
    if(argc<3)
    {
        puts("usage: inc2h.exe input.inc output.h");
        return 1;
    }
    vector<string> inclines;
    if(!maplines(argv[1], &inclines))
        return 1;
    vector<string> hlines;
    if(!convertlines(&inclines, &hlines))
        return 1;
    const char* output=argv[2];
    DeleteFileA(output);
    FILE* file=fopen(output, "a+");
    for(unsigned int i=0; i<hlines.size(); i++)
        dputs(file, hlines.at(i).c_str());
    fclose(file);
    puts("all done!");
    return 0;
}
