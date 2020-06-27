#include <iostream>
#include <iomanip>
#include <fstream>
#include "../include/cmdline.h"
#include "../include/cif.h"
#include "../include/cif_func.h"
#include "../include/func.h"

using namespace std;

int main(int argc, char *argv[]) {

    // cmd
    cmdline::parser parser;
    parser.add<string>("input_dir", 'i', "cod data folder location", true, "");
    parser.add<string>("log_file", 'f', "cod classification log file", true, "");
    parser.add<string>("output_dir", 'o', "icsd classification result export location", true);

    parser.add("log", 'l', "print the detail log, no log by default");

    parser.parse_check(argc, argv);

    if (argc == 1 || parser.exist("help"))
    {
        cout << parser.usage();
        return 0;
    }

    string input = parser.get<string>("input_dir");
    string output = parser.get<string>("output_dir");
    string log_file = parser.get<string>("log_file");

    bool log = false;
    if (parser.exist("log"))
    {
        log = true;
    }

    if (!is_folder_exist(input))
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

    // parse log
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
            for (int i = 0; i < files.size(); i++) {
                try {
                    if (log)
                    {
                        cout << "-----------------------FILE " + files[i] + " -----------------------" << endl;
                    }

                    CIF cif = CIF(input + "/" + files[i], log);
                    cif.parse_file();
                    cif.build_base_cell();

                    bool CC = false, CH = false;
                    map<string, double> atom_dist = cif.get_atom_dist();

                    if(!atom_dist.empty()) {
                        for (auto iter = atom_dist.begin(); iter != atom_dist.end(); iter++)
                        {
                            vector<string> values = del_split(iter->first, '-');

                            string atom_a = values[1];
                            string atom_b = values[3];
                            double distance = iter->second;

                            auto a = radius_dict.find(atom_a), b = radius_dict.find(atom_b);
                            if (a == radius_dict.end())
                            {
                                a = radius_dict.find(Universal);
                                // throw Exception(atom_a + " 's radius not found");
                            }

                            if (b == radius_dict.end())
                            {
                                b = radius_dict.find(Universal);
                                // throw Exception(atom_b + " 's radius not found");
                            }

                            double radius_a = get_num(a->second.radius);
                            double radius_b = get_num(b->second.radius);

                            if (distance <= 1.2 * (radius_a + radius_b) && distance >= 0.5 * (radius_a + radius_b))
                            {
                                if (atom_a == "C" && atom_b == "C")
                                {
                                    CC = true;
                                }

                                if (atom_a == "C" && atom_b == "H" || atom_a == "H" && atom_b == "C")
                                {
                                    CH = true;
                                }
                            }
                        }
                    }

                    if (CC && CH)
                    {
                        cout << files[i] << " is orangic!" << endl;
                        string s = base_path + files[i];
                        unlink(s.c_str());
                    }
                    else
                    {
                        cout << files[i] << " is inorangic!" << endl;
                    }
                }
                catch(...) {

                }
            }
        }
    }
    return 0;
}