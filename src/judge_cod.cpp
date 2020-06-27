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
    parser.add<string>("input_dir", 'i', "cod data folder location", true, "");
    parser.add("log", 'l', "print the detail log, no log by default");

    parser.parse_check(argc, argv);

    if (argc == 1 || parser.exist("help"))
    {
        cout << parser.usage();
        return 0;
    }

    string input = parser.get<string>("input_dir");

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
        vector<string> files = get_all_files(input, "cif");
        for(auto item : files) {
            try {

                if(log) {
                    cout << "-----------------------FILE " + item + " -----------------------" << endl;
                }

                CIF cif = CIF(item, log);
                cif.parse_file();
                cif.build_base_cell();

                bool CC = false, CH = false;
                map<string, double> atom_dist = cif.get_atom_dist();
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

                if (CC && CH)
                {
                    cout << item << " is orangic!" << endl;
                }
                else
                {
                    cout << item << " is inorangic!" << endl;
                }

            }
            catch(...) {

            }
        }
    }
    catch (Exception err)
    {
        cerr << err.msg << endl;
    }   
    return 0;
}