#ifndef MIRROR_H
#define MIRROR_H

#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cfloat>
#include <regex>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

#include "exception.h"
#include "func.h"
#include "conf.h"


class Mirror {

public:
    Mirror(string Poscar, string Eigenval, string Outcar, string Kpoints, string Doscar) {
        this.poscar = Poscar;
        this.eigenval = Eigenval;
        this.outcar = Outcar;
        this.kpoints = Kpoints;
        this.doscar = Doscar;

        get_eig_info(Eigenval);
    }

    string readFile(string file) {
        try {
            ifstream in(file, ios::in);
            if(!in.is_open()) {
                throw Exception("[ERROR]" + file + " does not exist or fails to open!" + endl);
            }
            else {
                string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                return str;
            }
        }
        catch(...) {
            throw Exception("[ERROR]Read File " + file + " failed" + endl);
        }
    }


    void get_eig_info(string eigenval) {
        string buff = readFile(eigenval);
        vector<string> lines = re_split(buff, "\\n");

        vector<string> spin_info = re_split(lines[0], "\\s+");
        if(spin_info[spin_info.size()-1] == '1') {
            this->spin = false;
        }
        else if(spin_info[spin_info.size()-1] == '2') {
            this->spin = true;
        }
        else {
            throw Exception("[ERROR]Spin state Error, check EIGENVAL file");
        }

        vector<string> nband_info = re_split(lines[5], "\\s+");
        this->nbands = stoi(nband_info[nband_info.size()-1]);
    }
    
    void get_efermi(string outcar) {
        string str = readFile(outcar);
        vector<string> lines = re_split(str, "\\n");
        for(auto line : lines) {
            if(std::regex_match(line, std::regex("\\s+?E-fermi\\s:\\s+?(-?\\d+?.\\d+).*"))) {
                vector<string> cur = re_split(line, "\\s+");
                this->efermi = atof(cur[2]);          
            }
        }
    }

        // private:
    string poscar;
    string eigenval;
    string outcar;
    string kpoints;
    string doscar;

    bool spin;
    int nbands;
    double efermi;

private:

};

class Band {

public:
    Band(string Poscar, string Eigenval, string Outcar, string Kpoints, string Doscar) :
        mirror(Mirror(string Poscar, string Eigenval, string Outcar, string Kpoints, string Doscar)) {
        

    }

    void get_kpath_coordinates(string poscar, string outcar, vector<vector<double>> kpoints) {
        string clkpdt = this->mirror.read_line_from_file(this->mirror.kpoints);
        vector<string> lines = re_split(clkpdt, "\\n");
        string::size_type idx = lines[2].find("Line-mode");

        if(idx != string::npos) {
            throw Exception("KpointsFileError");
        }
        else {
            multimap<string, vector<double>> symbol_list;
            for(auto line : lines) {
                if(line.find("!") != std::string::npos) {
                    auto tmp = del_split(line, '!');
                    auto nums = re_split(tmp[0], "\\s+");
                    vector<double> realNums;
                    for(auto n : nums) {
                        realNums.push_back(get_num(n));
                    }

                    symbol_list.insert(make_pair(tmp[1], realNums));
                }
            }

            vector<vector<double>> rec_vector;
            try {
                // method 1

                // get_reciprocal_vector_from_outcar
                string str = readFile(outcar)
                vector<string> lines = re_split(str, "\\n");
                bool flag = false;
                for(int i = 0; i < lines.size(); i++) {
                    if(flag) {
                        auto tmp = re_split(lines[i], "\\s+");
                        if(tmp.size() != 6 || lines[i].find("length of vectors")) {
                            flag = false;
                        }
                        else {
                            vector<double> recv;
                            for(int j = 3; j < tmp.size(); j++) {
                                recv.push_back(get_num(tmp[j]));
                            }
                            rec_vector.push_back(recv);
                        }
                    }

                    if(lines[i].find("reciprocal lattice vectors")) {
                        flag = true;
                    }
                }
            }
            catch(...) {
                // method 2
                cerr << "UnknownError, now switched to method2" << endl;

                // get_reciprocal_vector_from_poscar


            }

            // prepare_kpath
            


        }
    }


    void get_band_structure() {
        eig_str = this->mirror.readFile(this->mirror.eigenval);
        vector<string> lines = re_split(eig_str, "\\n");
        for(int i = 5; i < lines.size(); i++) {
            vector<string> cur = re_split(lines[i], "\s+");
            if(cur.size() == 4) {
                vector<double> p;
                for(int i = 0; i < 3; i++) {
                    p.push_back(get_num(cur[i]))
                }
                this->kpoints.push_back(num);
            }
            else {
                eigv.push_back(lines[i]);
            }
        }

        vector<string> lbs;
        vector<vector<double>> subtract_ef_eig;
        if(this->mirror.spin) {
            // .T[1, 3).T
            for(int i = 0; i < eigv.size(); i++) {
                subtract_ef_eig.push_back(vector<double>{eigv[i][1] - this->mirror.efermi,
                                                         eigv[i][2] - this->mirror.efermi});
            }

            lbs.push_back("Wave_vector");
            lbs.push_back("spin_up");
            lbs.push_back("spin_down");
        }
        else {
            // .T[1].T
            for(int i = 0; i < eigv.size(); i++) {
                subtract_ef_eig.push_back(vector<double>{eigv[i][1] - this->mirror.efermi});
            }

            lbs.push_back("Wave_vector");
            lbs.push_back("Energy_level");
        }

        int nkp = kpoints.size();

    }


private:
    Mirror mirror;
    vector<vector<double>> kpoints;
    vector<string> eigv;
}

#endif