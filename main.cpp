#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(ifstream& in, ofstream& out, const vector<path>& include_directories, const path& current_path){
    
    static regex num_reg1(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex num_reg2(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    smatch m;
    
    int line = 1;
    string s;
    while(getline(in, s)){
        if (regex_match(s, m, num_reg1) || regex_match(s, m, num_reg2)){           
            string found_include = string(m[1]);
            path include_path = current_path.parent_path()/found_include;
            
            if(exists(include_path)){
                ifstream in(include_path);
                if(!in)return false;
                if(!Preprocess(in, out, include_directories, include_path)) return false;             
            }else{
                bool flag_include_found = false;
                for(const path& p : include_directories){
                    include_path = p/found_include;
                    if(exists(include_path)){
                        ifstream in(path(p/found_include));
                        if(!in)return false; 
                        if(!Preprocess(in, out, include_directories,  current_path)) return false;
                        flag_include_found = true;
                        break;    
                    }
                }  
                if(!flag_include_found){
                    cout <<"unknown include file "s<< found_include <<" at file "s<< current_path.string() <<" at line "s<< line << endl;
                    return false;
                }           
            }
            
        }else{
            out << s << endl;
        }
        line++;
    }
    in.close();
    return true;
    
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){
    
    ifstream in(in_file.string());
    if(!in)return false;
    
    ofstream out(out_file.string(),ios::app);
    if(!out.is_open())return false;
    
    return Preprocess(in, out, include_directories, in_file);
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}