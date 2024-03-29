#ifndef CIF_H
#define CIF_H

#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <regex>
#include <algorithm> 
#include <vector>
#include <map>
#include <set>

#include "exception.h"
#include "func.h"
#include "conf.h"

using std::string;
using std::ifstream;
using std::ofstream;
using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::vector;
using std::map;
using std::pair;
using std::set;
using std::to_string;

#define PI acos(-1)

const string Universal = "XX";

class CIF
{

public:
    string filename;
    
    CIF(string _filename, bool _log = true) { 
        this->filename = _filename;
        this->log = _log;
        this->time = "";
    }

    // parse cif file and get information
    void parse_file() noexcept(false) {
        
        if(log) {
            cout << "Parsing the cif file - " << get_filename(this->filename) << endl;
        }

        split_cif();
        seek_crystal_info();
        cal_crystal_info();
        deal_site_loop();
        get_symm_info();
        get_atom_coordinates();
    }

    // get known resources
    void get_known_res() {
        
        if(log) {
            cout << "Getting some known resources..." << endl;
        }
        
        if(radius_dict.empty()) {
            get_atom_radius();
        }

        if(solvent_dict.empty()) {
            get_known_solvent();
        }
    }

    // build base cell
    void build_base_cell() noexcept(false) {
        if(log) {
            cout << "Building base cell..." << endl;
        }
        if(radius_dict.empty() || solvent_dict.empty()) {
            throw Exception("please get known resources first");
        }

        calc_bond_distance();
        judge_if_have_bond();
        connect_network();
    }

    // find exist solvent
    void find_solvent(bool print = true) noexcept(false) {
        if(log) {
            cout << "Looking for solvent in " << get_filename(this->filename) << endl;
        }

        vector<string> chem_list;
        for(auto &chem : this->atom_conn_set) {
            string formula = get_chem_formula(chem);
            // cout << formula << endl;
            chem_list.push_back(formula);

            bool isAdd = false;
            for(auto &atom : chem) {
                if(get_atom_state(get_species(atom)) == "1") {
                    this->framwork_list.push_back(formula);
                    isAdd = true;
                    break;
                }
            }
            if(!isAdd) {
                this->solvent_chk_list.push_back(formula);
            }
        }

        if(print && log) {
            // printVec(chem_list);
            cout << "The calculated solvent molecule to be screened is";
            cout << " [ ";
            for(auto &sol : this->solvent_chk_list) {
                if(cmp_solvent_known(sol)) {
                    cout << sol << "<known>" << " ";
                }
                else {
                    cout << sol << "<unknown>" << " ";
                }
            }
            cout << " ] " << endl;

            cout << "The MOF framework is ";
            cout << " [ ";
            printVec(this->framwork_list);
            cout << " ] " << endl;
        }
    }

    // export results after removing solvent
    void export_modify_result(string output_path, bool isForce = false) {
        if(log) {
            cout << "Exporting result..." << endl;
        }
        if(!is_folder_exist(output_path)) {
            make_dir(output_path);
        }


        set<string> rm_atom_site;

        for(auto &chem : this->atom_conn_set) {
            string formula = get_chem_formula(chem);
            if(find(this->solvent_chk_list.begin(), this->solvent_chk_list.end(), formula) == this->solvent_chk_list.end()) {
                continue;
            }

            if(!isForce && !cmp_solvent_known(formula)) {
                continue;
            }
        
            for(auto item : chem) {
                rm_atom_site.insert(item);
            }
        }

        // printSet(rm_atom_site);

        vector<string> cif_list = del_split(this->cif_buf, '\n');
        for(auto iter = cif_list.begin(); iter != cif_list.end(); ) {
            bool del = false;
            for(auto it = rm_atom_site.begin(); it != rm_atom_site.end(); it++) {
                if(std::regex_search(*iter, std::regex("^" + *it + "\\s+?"))) {
                    iter = cif_list.erase(iter);
                    del = true;
                    break;
                }
            }
            if(!del) {
                iter++;
            }
        }
        
        string clean_name = output_path + "/" + del_split(get_filename(filename), '.')[0] + "_clean.cif";

        ofstream out(clean_name);
        for(auto &line : cif_list) {
            out << line << endl;
        }
        out.close();

        if(log) {
            cout << "Export file " << clean_name << " successfully!" << endl;    
        }
    }

    // get symmetry info
    vector<vector<string>> get_symm() {
        return this->symm;
    }

    string get_name_HM() {
        return this->name_HM;
    }

    string get_name_Hall() {
        return this->name_Hall;
    }

    // get species for atom site
    string get_species(const string &s) {
        string species = "";
        for(auto &c : s) {
            if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                species += c;
                continue;
            }
            break;
        }
        return species;
    }

    vector<vector<double>> get_lattice() {
        return this->cell;
    }

    map<string, vector<double>> get_atom_cd() {
        return this->atom_cd;
    }

    map<string, double> get_atom_dist() {
        return this->atom_dist;
    }

    map<string, set<vector<double>>> get_atom_cd_set() {
        if(!this->atom_cd_set.empty()) {
            return this->atom_cd_set;
        }

        for(auto iter = this->atom_cd.begin(); iter != this->atom_cd.end(); iter++) {
            string species = get_species(iter->first);
            this->atom_cd_set[species].insert(iter->second);
        }

        return this->atom_cd_set;
    }

    map<string, vector<string>> get_loop_dict() {
        return this->loop_dict;
    }

    set<string> get_atoms() {
        return this->atoms;
    }

    vector<string> get_solvent_chk_list() {
        return this->solvent_chk_list;
    }

    bool judge_if_have_disorder() noexcept(false) {
        for(auto iter = this->atom_dist.begin(); iter != this->atom_dist.end(); iter++) {
            vector<string> values = del_split(iter->first, '-');

            string atom_a = values[1];
            string atom_b = values[3];
            double distance = iter->second;

            auto a = radius_dict.find(get_species(atom_a)), b = radius_dict.find(get_species(atom_b));

            if(a == radius_dict.end()) {
                a = radius_dict.find(Universal);
                throw Exception(atom_a + " 's radius not found");
            }

            if(b == radius_dict.end()) {
                b = radius_dict.find(Universal);
                throw Exception(atom_b + " 's radius not found");
            }

            double radius_a = get_num(a->second.radius);
            double radius_b = get_num(b->second.radius);

            if(distance < 0.5 * (radius_a + radius_b)) {
                return true;
            }
        }
        return false;
    }

    // convert fractional coordinates to real coordinates
    vector<double> frac2cart(vector<double> &atom_frac_cd) {
        vector<double> lx_vec = this->cell[0];
        vector<double> ly_vec = this->cell[1];
        vector<double> lz_vec = this->cell[2];

        double frac_a = atom_frac_cd[0];
        double frac_b = atom_frac_cd[1];
        double frac_c = atom_frac_cd[2];

        double x_cart = lx_vec[0] * frac_a + lx_vec[1] * frac_b + lx_vec[2] * frac_c;
        double y_cart = ly_vec[0] * frac_a + ly_vec[1] * frac_b + ly_vec[2] * frac_c;
        double z_cart = lz_vec[0] * frac_a + lz_vec[1] * frac_b + lz_vec[2] * frac_c;

        return vector<double>{x_cart, y_cart, z_cart};
    }

    // return the element is metal or not
    string get_atom_state(string element) noexcept(false) {
        auto iter = radius_dict.find(element);
        
        if(iter == radius_dict.end()) {
            iter = radius_dict.find(Universal);
            // throw Exception("unable to find the state information of the corresponding element...");
        }

        return iter->second.O_metal;
    }

    string get_time() {
        return time;
    }

    set<Connected> get_bonds() {
        return this->bonds;
    }

private:
    // split the information in the cif file and save in map
    void split_cif() noexcept(false) {
        ifstream in(this->filename, ios::in);
        if(!in.is_open()) {
            throw Exception(this->filename + " does not exist or fails to open!");
        }
        else {
            string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            this->cif_buf = str;

            vector<string> cif_loop_blocks = re_split(str, "\\s+?\\r*?loop_\\r*?\\n");
            
            for (string content : cif_loop_blocks) {
                if (content.find("###########") != string::npos ||
                    std::regex_search(content, std::regex("^data_.*?")) ||
                    std::regex_search(content, std::regex("^\\s*_symmetry_space_group\\w+?"))) {

                    vector<string> chaos_loop = re_split(content, "\\r*?\\n");
                    loop_dict["chaos_loop"] = chaos_loop;

                    // for (auto item : loop_dict["chaos_loop"]) {
                    //     cerr << item << endl;
                    // }
                }
                else if (std::regex_search(content, std::regex("^\\s*_symmetry_equiv_pos\\w+?"))) {
                    vector<string> sym_loop = re_split(content, "\\r*?\\n");
                    loop_dict["sym_loop"] = sym_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_site_aniso\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["aniso_site_loop"] = site_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_site_anharm\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["anharm_site_loop"] = site_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_site_displace_Fourier\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["displace_Fourier_site_loop"] = site_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_site_Fourier\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["Fourier_site_loop"] = site_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_site_U_Fourier\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["U_Fourier_site_loop"] = site_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_site_displace_special\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["displace_special_site_loop"] = site_loop;
                }
                else if (std::regex_search(content, std::regex("^\\s*_atom_site_occ\\w+?")) &&
                         !std::regex_search(content, std::regex("^\\s*_atom_site_occupancy"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["occ_site_loop"] = site_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_site_oxford\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["oxford_site_loop"] = site_loop;
                }
                else if(std::regex_search(content, std::regex("^\\s*_atom_sites_solution\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["solution_site_loop"] = site_loop;
                }
                else if (std::regex_search(content, std::regex("^\\s*_atom_site\\w+?"))) {
                    vector<string> site_loop = re_split(content, "\\r*?\\n");
                    loop_dict["site_loop"] = site_loop;
                }
                else if (std::regex_search(content, std::regex("^\\s*_atom_type\\w+?"))) {
                    vector<string> type_loop = re_split(content, "\\r*?\\n");
                    loop_dict["type_loop"] = type_loop;
                }
                else if (std::regex_search(content, std::regex("^\\s*_geom_bond\\w+?"))) {
                    vector<string> bond_loop = re_split(content, "\\r*?\\n");
                    loop_dict["bond_loop"] = bond_loop;
                }
                else {
                    vector<string> unknow_loop = re_split(content, "\\r*?\\n");
                    if(unknow_loop.empty()) {
                        continue;
                    }
                    loop_dict["unknow_loop"] = unknow_loop;
                    // cerr << "5" << endl;
                }
            }

            in.close();
        }
    }

    // get crystal cell parameters from cif file
    void seek_crystal_info() noexcept(false) {
        string str = this->cif_buf;
        std::regex rgx("(_cell_[l|a].*?)\\s+(.*)");
        
        try {
            for (std::sregex_token_iterator it(str.begin(), str.end(), rgx), end; it != end; it++) {
                string cur = it->str();
                // cerr << cur << endl;
                vector<string> params = del_split(cur, ' ');

                if(params.size() < 2) {
                    continue;
                }
                string key = params[0];
                double value = get_num(params[1]);

                if(fabs(value + 1.0) < 0.00001) {
                    throw Exception("");
                }

                // cerr << key << " " << value << endl;
                cell_params[key] = value;
            }
        }
        catch(...) {
            throw Exception("cell parameter extract failed");
        }
    }

    // calculate lattice contstant and angle
    void cal_crystal_info() noexcept(false) {
        try {
            double a = this->cell_params["_cell_length_a"];
            double b = this->cell_params["_cell_length_b"];
            double c = this->cell_params["_cell_length_c"];
            
            double ap = this->cell_params["_cell_angle_alpha"] / 180 * PI;
            double bt = this->cell_params["_cell_angle_beta"] / 180 * PI;
            double ga = this->cell_params["_cell_angle_gamma"] / 180 * PI;

            // cerr << a << " " << b << " " << c << " " << ap << " " << bt << " " << ga << endl;

            double bc2 = pow(b, 2) + pow(c, 2) - 2 * b * c * cos(ap);
            
            double h1 = a;
            double h2 = b * cos(ga);
            double h3 = b * sin(ga);
            double h4 = c * cos(bt);
            double h5 = (pow(h2 - h4, 2) + pow(h3, 2) + pow(c, 2) - pow(h4, 2) - bc2) / (2 * h3);
            double h6 = sqrt(pow(c, 2) - pow(h4, 2) - pow(h5, 2));
            
            this->cell.push_back({h1, h2, h4});
            this->cell.push_back({0, h3, h5});
            this->cell.push_back({0, 0, h6});

            this->angel.push_back(ap);
            this->angel.push_back(bt);
            this->angel.push_back(ga);
        }
        catch(...) {
            throw Exception("cell parameter extract failed");
        }
    }
    
    // get the site information of the atom in the loop
    void deal_site_loop() noexcept(false) {
        try {
            vector<string> site_loop = this->loop_dict["site_loop"];

            // printVec(site_loop);
            // cout << endl;
            vector<string> data_value;

            for(auto data : site_loop) {
                if(std::regex_match(data, std::regex("\\s*_(\\w|\\[)+.*?"))) {
                    this->site_label.push_back(trim(data));
                }
                else {
                    data_value.push_back(data);
                }
            }

            for(auto &item : data_value) {
                if(item.find("#") != string::npos) {
                    break;
                }
                this->site_value.push_back(del_split(item, ' '));
            }
        }
        catch(...) {
            throw Exception("failed to divide data");
        }
    }

    // get the symmetry information of the atom    
    void get_symm_info() noexcept(false) {

        vector<string> sym_loop = this->loop_dict["sym_loop"];

        // cerr << sym_loop.size() << endl;

        if(!sym_loop.empty()) {
            int index = 0, i = 0;
            for(i = 0; i < sym_loop.size(); i++) {
                sym_loop[i] = trim(sym_loop[i]);
                if(sym_loop[i].find("_symmetry_") != string::npos) {
                    if(sym_loop[i].find("xyz")) {
                        index = i;
                    }
                }
                else {
                    break;
                }
            }

            for(; i < sym_loop.size(); i++) {
                sym_loop[i] = trim(sym_loop[i]);
                if(sym_loop[i][0] == '_') {
                    break;
                }

                sym_loop[i] = clean_str(sym_loop[i]);
                vector<string> cur = re_split(sym_loop[i], "[,| ]");
                vector<string> xyz;
                for(int j = index; j < cur.size(); j++) {
                    xyz.push_back(clean_str(cur[j]));
                }
                this->symm.push_back(xyz);
            }
        }

        vector<string> lines = del_split(this->cif_buf, '\n');
        for(auto item : lines) {
            if(item.find("name_H-M") != string::npos) {
                vector<string> split =  del_split(item, ' ');
                string cur = "";
                for(int i = 1; i < split.size(); i++) {
                    cur += split[i];
                    // if(i != split.size() - 1) {
                    //     cur += " ";
                    // }
                }

               this->name_HM = clean_str(cur);
            }

            if(item.find("name_Hall") != string::npos) {
                vector<string> split =  del_split(item, ' ');
                string cur = "";
                for(int i = 1; i < split.size(); i++) {
                    cur += split[i];
                    if(i != split.size() - 1) {
                        cur += " ";
                    }
                }

               this->name_Hall = clean_str(cur);
            }
        }

        // if(this->loop_dict.count("unknow_loop")) {
        //     vector<string> unknow_loop = this->loop_dict["unknow_loop"];
        //     for(auto item : unknow_loop) {
        //         if(item.find("name_H-M") != string::npos) {
        //             vector<string> split =  del_split(item, ' ');
        //             string cur = "";
        //             for(int i = 1; i < split.size(); i++) {
        //                 cur += split[i];
        //                 // if (i != split.size() - 1) {
        //                 //     cur += " ";
        //                 // }
        //             }

        //             this->name_HM = clean_str(cur);
        //         }

        //         if(item.find("name_Hall") != string::npos) {
        //             vector<string> split =  del_split(item, ' ');
        //             string cur = "";
        //             for(int i = 1; i < split.size(); i++) {
        //                 cur += split[i];
        //                 if (i != split.size() - 1) {
        //                     cur += " ";
        //                 }
        //             }

        //             this->name_Hall = clean_str(cur);
        //         }
        //     }
        // }
    }

    // get the coordinates information of the atom
    void get_atom_coordinates() noexcept(false) {
        try {
            // printVec(this->site_label);
            // cout << endl;
            int x_index = std::distance(std::begin(this->site_label), \
                                        std::find(this->site_label.begin(), this->site_label.end(), "_atom_site_fract_x"));
            int y_index = std::distance(std::begin(this->site_label), \
                                        std::find(this->site_label.begin(), this->site_label.end(), "_atom_site_fract_y"));
            int z_index = std::distance(std::begin(this->site_label), \
                                        std::find(this->site_label.begin(), this->site_label.end(), "_atom_site_fract_z"));

            int species_index = std::distance(std::begin(this->site_label), \
                                            std::find(this->site_label.begin(), this->site_label.end(), "_atom_site_type_symbol"));
            int atom_site_index = std::distance(std::begin(this->site_label), \
                                                std::find(this->site_label.begin(), this->site_label.end(), "_atom_site_label"));
            
            int occupancy_index = std::distance(std::begin(this->site_label), \
                                                std::find(this->site_label.begin(), this->site_label.end(), "_atom_site_occupancy"));
            // cerr << x_index << " " << y_index << " " << z_index << " " << species_index << " " << atom_site_index << endl;
            
            if(x_index >= this->site_label.size() || 
                y_index >= this->site_label.size() ||
                z_index >= this->site_label.size()) {

                // printVec(this->site_label);
                // cout << endl;

                throw Exception("[ERROR] CIF file atom site section parse error!");
            }

            for(auto atom_info : this->site_value) {
                if(atom_info.size() < this->site_label.size()) {
                    // printVec(atom_info);
                    // cout << endl;

                    // printVec(site_label);
                    // cout << endl;
                    throw Exception("[ERROR] CIF file atom site section has some problem");
                }

                double x = get_num(atom_info[x_index]);
                double y = get_num(atom_info[y_index]);
                double z = get_num(atom_info[z_index]);

                if(occupancy_index < atom_info.size()) {
                    // cerr << get_num(atom_info[occupancy_index]) << endl;
                    if(get_num(atom_info[occupancy_index]) < 1.0) {
                        throw Exception("Atom occupancy is smaller than 1.0");
                    }
                }

                string species = "";
                if(species_index < atom_info.size()) {
                    species = get_species(atom_info[species_index]);
                }
                else {
                    species = get_species(atom_info[atom_site_index]);
                }

                string atom_site = atom_info[atom_site_index];

                string key = atom_site + "-" + species;
                
                this->atom_cd[key] = vector<double>{x, y, z};
                this->atoms.insert(species);
            }

            // printMap(this->atom_cd);

        }
        catch(std::exception &e) {
            throw Exception(string(e.what()));
        }
    }

    // get cif file post time
    void get_post_time() {
        // _audit_update_record
        string str = this->cif_buf;
        std::regex rgx("_audit_update_record\\s+(.*)");
        for (std::sregex_token_iterator it(str.begin(), str.end(), rgx), end; it != end; it++) {
            string cur = it->str();
            // cerr << cur << endl;
            vector<string> params = del_split(cur, ' ');
            time = clean_str(params[1]);
        }
    }

    // convert real coordinates to fractional coordinates
    vector<double> cart2frac(vector<double> &atom_cart_cd) {
        
    }

    double calc_distance(vector<double> &a_cd, vector<double> &b_cd) {
        // coordinate transformation
        vector<double> a_cart = frac2cart(a_cd);
        vector<double> b_cart = frac2cart(b_cd);

        double distance = sqrt(pow(fabs(a_cart[0] - b_cart[0]), 2) + \
                                pow(fabs(a_cart[1] - b_cart[1]), 2) + \
                                pow(fabs(a_cart[2] - b_cart[2]), 2));
        return distance;
    }

    // calculate bond distance
    void calc_bond_distance() {
        for(auto a = this->atom_cd.begin(); a != this->atom_cd.end(); a++) {
            for(auto b = this->atom_cd.begin(); b != this->atom_cd.end(); b++) {
                if(a == b) {
                    continue;
                }
                vector<double> a_cd = a->second;
                vector<double> b_cd = b->second;            

                double distance = calc_distance(a_cd, b_cd);

                string name = a->first + "-" + b->first;

                this->atom_dist[name] = distance;
            }
        }

        // printMap(this->atom_dist);
    }

    void judge_if_have_bond() noexcept(false) {
        for(auto iter = this->atom_dist.begin(); iter != this->atom_dist.end(); iter++) {
            vector<string> values = del_split(iter->first, '-');

            // printVec(values);
            // cout << endl;

            string atom_a = values[1];
            string atom_b = values[3];

            // cout << values[0] << " " << values[2] << endl;

            // cerr << atom_a << " " << atom_b << endl;
            double distance = iter->second;

            auto a = radius_dict.find(atom_a), b = radius_dict.find(atom_b);

            if(a == radius_dict.end()) {
                a = radius_dict.find(Universal);
                throw Exception(atom_a + " 's radius not found");
            }

            if(b == radius_dict.end()) {
                b = radius_dict.find(Universal);
                throw Exception(atom_b + " 's radius not found");
            }

            double radius_a = get_num(a->second.radius);
            double radius_b = get_num(b->second.radius);

            if(distance <= 1.2 * (radius_a + radius_b) && distance >= 0.5 * (radius_a + radius_b)) {
                this->have_bond_list.push_back(iter->first);
                this->connect_list.push_back(Connected(values[0], values[2]));
                
                this->bonds.insert(Connected(values[1], values[3]));
                this->bonds.insert(Connected(values[3], values[1]));
            }
        }
        // printVec(this->have_bond_list);
        if(log) {
            cout << "The number of bonded atom pairs is " << this->have_bond_list.size() << endl;
        }
    }

    string get_father(map<string, string> &atom_rel, string x) {
        return (atom_rel[x] == x) ? x : get_father(atom_rel, atom_rel[x]);
    }

    void to_union(map<string, string> &atom_rel, string &a, string &b) {
        string p1 = get_father(atom_rel, a);
        string p2 = get_father(atom_rel, b);
        atom_rel[p1] = p2;
    }

    // use disjoint set union
    void connect_network() noexcept(false) {
        map<string, string> atom_rel;
        
        for(auto item : this->connect_list) {
            // cerr << item.atom_a << " " << item.atom_b << endl;
            atom_rel[item.atom_a] = item.atom_a;
            atom_rel[item.atom_b] = item.atom_b;
        }
        
        for(auto &item : this->connect_list) {
            to_union(atom_rel, item.atom_a, item.atom_b);
        }
        
        for(auto &item : this->connect_list) {
            bool inSet = false;
            for(auto &s : this->atom_conn_set) {
                if(get_father(atom_rel, *s.begin()) == get_father(atom_rel, item.atom_a)) {
                    s.insert(item.atom_a);
                    s.insert(item.atom_b);
                    inSet = true;
                    break;
                }
            }

            if(!inSet) {
                this->atom_conn_set.push_back(set<string>{item.atom_a, item.atom_b});
            }
        }

        // printVec(this->atom_conn_set);
    }

    // get the chemical formula of the atoms in the group
    string get_chem_formula(set<string> &s) {
        map<string, int> cnt;
        for(auto &atom : s) {
            string species = get_species(atom);
            cnt[species]++;
        }

        string chem_formula = "";
        for(auto iter = cnt.begin(); iter != cnt.end(); iter++) {
            chem_formula += iter->first + to_string(iter->second);
        }
        return chem_formula;
    }

    // compare existing list of solvent
    bool cmp_solvent_known(string formula) noexcept(false) {
        return solvent_dict.count(formula) > 0;
    }

private:
    // detail log
    bool log;

    string cif_buf;
   
    map<string, vector<string>> loop_dict;

    // label - atom coordinates
    map<string, vector<double>> atom_cd;
    
    // specie - atom coordinates
    map<string, set<vector<double>>> atom_cd_set;

    // atom
    set<string> atoms;

    // atom&atom - distance
    map<string, double> atom_dist;
    vector<string> have_bond_list;
    
    vector<Connected> connect_list;
    set<Connected> bonds;
    vector<set<string>> atom_conn_set;

    // atom_site - atom_species
    // map<set<string>, set<string>> cn_species;

    map<string, double> cell_params;
    vector<vector<double>> cell;
    vector<double> angel;

    vector<string> site_label;
    vector<vector<string>> site_value;

    // solvents
    vector<string> framwork_list;
    vector<string> solvent_chk_list;

    // symmetry
    vector<vector<string>> symm;
    string name_Hall;
    string name_HM;

    // time
    string time;
};


#endif