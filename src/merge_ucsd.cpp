#include <iostream>
#include <cctype>
#include <iomanip>
#include <sys/wait.h>
#include <unistd.h>
#include "../include/cif.h"
#include "../include/cif_func.h"
#include "../include/cif_sim.h"
#include "../include/cmdline.h"
#include "../include/conf.h"
#include "../include/fp/fplib.h"

using namespace std;

const double Threshold = 0.01;

void get_res() noexcept(false);

double get_cif_similarity(CIF &a, CIF &b);

int main(int argc, char *argv[])
{

    int cnt = 0;
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
    if (parser.exist("log")) {
        log = true;
    }

    if (!is_folder_exist(input))
    {
        cout << "COD data not found!" << endl;
    }

    try
    {
        map<string, vector<string>> files;
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
        for(auto item : blocks) {
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
                p += element_format(path[i]) + "/";
            }

            string base_path = output + "/" + p;
            set<string> cur;
            for (int i = 0; i < files.size(); i++)
            {
                cur.insert(files[i]);
                auto cpid = fork();
                if(cpid == 0) {
                    try {
                        if (log)
                        {
                            cout << "-----------------------FILE " + files[i] + " -----------------------" << endl;
                        }
                        CIF cif = CIF(input + "/" + files[i], log);
                        cif.parse_file();

                        if (!is_folder_exist(base_path))
                        {
                            make_dir(base_path);
                            cp_file(input + "/" + files[i], base_path);
                        }
                        else
                        {
                            string similar_file = "";

                            vector<string> cfiles = get_all_files(base_path, "cif");

                            for (auto cmp_item : cfiles)
                            {
                                CIF other = CIF(cmp_item, log);
                                if (cur.count(get_filename(cmp_item)) > 0)
                                {
                                    continue;
                                }
                                other.parse_file();

                                double sim = get_cif_similarity(cif, other);

                                if (sim < 0 || sim > Threshold)
                                {
                                    continue;
                                }
                                else
                                {
                                    // cerr << "[sim]" << get_filename(item) << " - " << get_filename(cmp_item) << ":\\tt" << sim << endl;
                                    similar_file = get_filename(cmp_item);

                                    cnt++;
                                    if (log)
                                    {
                                        cout << "File " << item << "[" << base_path << "] "
                                             << "find similar file - " << similar_file << endl;
                                    }

                                    exit(0);
                                }
                            }

                            cp_file(input + "/" + files[i], base_path);
                        }
                    }
                    catch(...) {

                    }

                    
                    exit(0);
                }
                else
                {
                    waitpid(cpid, 0, 0);
                }
            }
        }
    }

    cout << cnt << endl;
    return 0;
}

double get_cif_similarity(CIF &a, CIF &b)
{
    map<string, vector<double>> a_atom_cd = a.get_atom_cd();
    map<string, vector<double>> b_atom_cd = b.get_atom_cd();

    if (a_atom_cd.size() != b_atom_cd.size())
    {
        return -1;
    }

    for (auto a_iter = a_atom_cd.begin(), b_iter = b_atom_cd.begin(); a_iter != a_atom_cd.end() && b_iter != b_atom_cd.end(); a_iter++, b_iter++)
    {
        if (del_split(a_iter->first, '-')[1] != del_split(b_iter->first, '-')[1])
        {
            return -1;
        }
    }

    return get_fp_similarity(a, b);
}