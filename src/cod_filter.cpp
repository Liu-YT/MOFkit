#include <iostream>
#include <iomanip>
#include <fstream>
#include "../include/cmdline.h"
#include "../include/cif.h"
#include "../include/cif_func.h"
#include "../include/func.h"

using namespace std;

int main(int argc, char *argv[])
{

    // cmd
    cmdline::parser parser;
    parser.add<string>("inorangic_log", 'i', "inorangic cod log", true, "");
    parser.add<string>("log_file", 'f', "cod classification log file", true, "");
    parser.add<string>("output_dir", 'o', "icsd classification result export location", true);

    parser.add("log", 'l', "print the detail log, no log by default");

    parser.parse_check(argc, argv);

    if (argc == 1 || parser.exist("help"))
    {
        cout << parser.usage();
        return 0;
    }

    string inorangic_log = parser.get<string>("inorangic_log");
    string output = parser.get<string>("output_dir");
    string log_file = parser.get<string>("log_file");

    bool log = false;
    if (parser.exist("log"))
    {
        log = true;
    }

    if (!is_folder_exist(inorangic_log))
    {
        cout << "COD data not found!" << endl;
    }

    try
    {
        get_res(log);
    }
    catch (Exception err)
    {
        cerr << err.msg << endl;
    }


    // parse inorangic log
    cout << "Parse inorangic log!" << endl;
    set<string> all_orangic;
    ifstream o_in(inorangic_log, ios::in);
    if (!o_in.is_open())
    {
        cout << inorangic_log + " does not exist or fails to open!" << endl;
        return 0;
    }
    else
    {
        string str((std::istreambuf_iterator<char>(o_in)), std::istreambuf_iterator<char>());
        vector<string> blocks = re_split(str, "\n");
        for(auto block : blocks) {
            vector<string> tmp = re_split(block, "\\s+");
            cout << tmp[0] << " " << get_filename(tmp[0]) << endl;
            all_orangic.insert(get_filename(tmp[0]));
        }
    }



    // parse log
    cout << "Parse log!" << endl;
    ifstream in(log_file, ios::in);
    if (!in.is_open())
    {
        cout << log_file + " does not exist or fails to open!" << endl;
    }
    else
    {
        string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        vector<string> blocks = re_split(str, "\n");

        // cal
        for (auto item : blocks)
        {
            auto t = del_split(item, ':');
            string key = t[0];

            if (t.size() < 2)
            {
                continue;
            }
            vector<string> files = re_split(t[1], "\\s+");
            auto path = del_split(key, '|');
            string p = "";
            for (int i = 0; i < path.size(); i++)
            {
                p += path[i] + "/";
            }

            string base_path = output + "/" + p;
            for (int i = 0; i < files.size(); i++)
            {
                try
                {
                    if (log)
                    {
                        cout << "-----------------------FILE " + files[i] + " -----------------------" << endl;
                    }

                    if(all_orangic.count(get_filename(files[i]))) {
                        cout << files[i] << " is inorangic!" << endl;
                    }
                    else {
                        string s = base_path + files[i];
                        unlink(s.c_str());
                        cout << files[i] << " is orangic!" << endl;
                    }
                }
                catch (...)
                {
                }
            }
        }
    }
    return 0;
}