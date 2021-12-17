#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

enum ARG_TYPE { ARG_NUMBER, ARG_STRING, ARG_NONE, ARG_ERROR };

typedef struct __arg_element_t {
    string _name;
    string _value;
} arg_element_t;

typedef struct __arg_descritpion_t {
    string _name;
    string _description;
    enum ARG_TYPE _type;
    arg_element_t *_element;
} arg_descritpion_t;


class argument_parser_t
{
protected:
    map<string, arg_descritpion_t *> _arg_descritpions;
    vector<arg_descritpion_t *> _description_list;

    int _number_of_set_arguments;

    vector<string> _split_eq(string str);
    void _check_and_output();

    void _init();

public:
    argument_parser_t() {}

    argument_parser_t(int argc, const char *argv[])
    {
        parse_argument_list(argc, argv);
    }

    void parse_argument_list(int argc, const char *argv[]);


    void print_arg_description();

    void add_args(arg_descritpion_t dscrpt, vector<string> arg_name);

    enum ARG_TYPE get_type_of_arg(string arg_name);

    bool is_set(string arg_name);

    string get_argument_value(string arg_name);

    int argc();

    ~argument_parser_t();
};

inline int argument_parser_t::argc()
{
    return _number_of_set_arguments;
}

inline enum ARG_TYPE argument_parser_t::get_type_of_arg(string arg_name)
{
    try {
        enum ARG_TYPE tpy = _arg_descritpions.at(arg_name)->_type;
        return tpy;
    } catch (out_of_range &e) {
        cerr << "There is no argument named " << arg_name << endl;
        return ARG_ERROR;
    }
}

inline bool argument_parser_t::is_set(string arg_name)
{
    try {
        return _arg_descritpions.at(arg_name)->_element != NULL;
    } catch (out_of_range &e) {
        return false;
    }
}

inline argument_parser_t::~argument_parser_t()
{
    for (unsigned int i = 0; i < _description_list.size(); ++i) {
        delete _description_list[i];
    }
}

inline string argument_parser_t::get_argument_value(string arg_name)
{
    try {
        arg_element_t *element = _arg_descritpions.at(arg_name)->_element;
        if (element) {
            return element->_value;
        } else {
            cerr << "You haven't set this argument : " << arg_name << endl;
        }
    } catch (out_of_range &e) {
        cerr << "Invalid argument : " << arg_name << endl;
    }

    return "";
}

#endif
