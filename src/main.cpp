

#include <iomanip>
#include <iostream>
#include <regex>

#include "context.hpp"

int n;
char buffer[1024];
Context* cur_context;

// patterns
// [UserID]: ([0-9a-zA-Z_]+)
// [Password]: ([0-9a-zA-Z_]+)
// [CurrentPassword]: ([0-9a-zA-Z_]+)
// [NewPassword]: ([0-9a-zA-Z_]+)
// [Username]: (.+)
// [Privilege]: ([0137])
const std::regex switch_user("^ *su +([0-9a-zA-Z_]{1,30})( +([0-9a-zA-Z_]{1,30}))? *$");
const std::regex logout("^ *logout *$");
const std::regex register_user("^ *register +([0-9a-zA-Z_]{1,30}) +([0-9a-zA-Z_]{1,30}) +([^\\s]{1,30}) *$");
const std::regex change_passwd("^ *passwd +([0-9a-zA-Z_]{1,30})( +([0-9a-zA-Z_]{1,30}))? +([0-9a-zA-Z_]{1,30}) *$");
const std::regex add_user("^ *useradd +([0-9a-zA-Z_]{1,30}) +([0-9a-zA-Z_]{1,30}) +([0137]) +([^\\s]{1,30}) *$");
const std::regex delete_user("^ *delete +([0-9a-zA-Z_]{1,30}) *$");

// [ISBN]: ([^\\s]+:ISBN)
// [BookName]: ([^\"]+:bookname)
// [Quantity]: ([0-9]+:quantity)
const std::regex show_book(
    "^ *show(( +-(ISBN)=([^\\s]{1,20}))|( +-(name)=\"([^\"]{1,60})\")|( +-(author)=\"([^\"]{1,60})\")|( +-(keyword)=\"([^\"|]{1,60})\"))? *$");
const std::regex buy("^ *buy +([^\\s]{1,20}) +(([1-9][0-9]{0,9})|0) *$");
const std::regex select_book("^ *select +([^\\s]{1,20}) *$");
const std::regex modify_book(
    "^ *modify(( +-(ISBN)=([^\\s]{1,20}))|( +-(name)=\"([^\"]{1,60})\")|( +-(author)=\"([^\"]{1,60})\")|( +"
    "-(keyword)=\"([^\"]{1,60})\")|( +-(price)=((([1-9][0-9]*)|0)(\\.[0-9]{1,2})?)))+ *$");
const std::regex import_book("^ *import +([1-9][0-9]{0,9}) +((([1-9][0-9]*)|0)(\\.[0-9]{1,2})?) *$");

const std::regex show_finance("^ *show finance( +(([1-9][0-9]{0,9})|0))? *$");

// #define Invalid cout << "Invalid" << "--------------" << __LINE__ << "\n"
#define Invalid cout << "Invalid\n"
using std::cout, std::cerr, std::endl;

int main() {
    cur_context = Context::get_default_context();
    while (true) {
        std::cin.getline(buffer, 1024);
        int l = strlen(buffer);
        while ( l > 0 && buffer[l-1] == ' ') {
            buffer[--l] = '\0';
        }
        std::string input(buffer);
        std::smatch result;
        cerr << "> " << buffer << endl;
        if (input == "exit" || input == "quit") {
            Context::get_default_context()->close();
            return 0;
        }
        if ( input == "" ) {
            Context::get_default_context()->close();
            return 0;
        }
        if (std::regex_match(input, result, switch_user)) {
            std::string userid = result[1], passwd = result[3];
            Context* ret = cur_context->switch_user(userid, passwd);
            if (ret != nullptr) {
                cur_context = ret;
            } else {
                Invalid;
            }
        } else if (std::regex_match(input, result, logout)) {
            if (cur_context->logout()) {
                cur_context = cur_context->father_context;
            } else {
                Invalid;
            }
        } else if (std::regex_match(input, result, register_user)) {
            std::string userid = result[1], passwd = result[2], username = result[3];
            if (!cur_context->register_user(userid, passwd, username)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, change_passwd)) {
            std::string userid = result[1], cur_passwd, new_passwd;
            cur_passwd = result[3];
            new_passwd = result[4];
            if (!cur_context->change_passwd(userid, cur_passwd, new_passwd)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, add_user)) {
            std::string userid = result[1], passwd = result[2], _privilege = result[3],
                        username = result[4];
            int privilege;
            sscanf(_privilege.c_str(), "%d", &privilege);
            if (!cur_context->add_user(userid, passwd, privilege, username)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, delete_user)) {
            std::string userid = result[1];
            if (!cur_context->delete_user(userid)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, show_book)) {
            std::string filter_type = "", filter = "";
            for (int i = 3; i < result.size(); i += 3) {
                filter_type = result[i];
                filter = result[i + 1];
                if (filter_type != "") {
                    break;
                }
            }
            // cerr << filter_type << " " << filter << endl;
            std::vector<Book> output;
            if (!cur_context->find_book(filter_type, filter, output)) {
                Invalid;
            } else {
                if (output.size() == 0) {
                    std::cout << "\n";
                } else {
                    for (const Book& book : output) {
                        std::cout << book.ISBN << "\t" << book.name << "\t" << book.author << "\t";
                        std::cout << book.keyword << std::fixed << std::setprecision(2) << "\t"
                                  << book.price << "\t" << book.quantity << "\n";
                    }
                }
            }
        } else if (std::regex_match(input, result, buy)) {
            std::string ISBN = result[1], _quantity = result[2];
            long long quantity;
            sscanf(_quantity.c_str(), "%lld", &quantity);
            if ( quantity > 2147483647 ) {
                Invalid;
            } else {
                double ret = cur_context->buy(ISBN, quantity);
                if (ret < 0) {
                    Invalid;
                } else {
                    std::cout << std::fixed << std::setprecision(2) << ret << "\n";
                }
            }
        } else if (std::regex_match(input, result, select_book)) {
            std::string ISBN = result[1];
            if (!cur_context->select(ISBN)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, modify_book)) {
            if (result.size() == 1) {
                Invalid;
            } else {
                std::vector<std::pair<String, String>> modifier;
                for (int i = 3; i < result.size(); i += 3) {
                    if (result[i].length()) {
                        modifier.emplace_back(result[i], result[i + 1]);
                        // cerr << result[i] << " " << result[i + 1] << endl;
                    }
                }
                if (!cur_context->modify(modifier)) {
                    Invalid;
                }
            }
        } else if (std::regex_match(input, result, import_book)) {
            std::string _quantity = result[1], _total_cost = result[2];
            long long quantity;
            double total_cost;
            sscanf(_quantity.c_str(), "%lld", &quantity);
            sscanf(_total_cost.c_str(), "%lf", &total_cost);
            if ( quantity > 2147483647 ) {
                Invalid;
            } else if (!cur_context->import_book(quantity, total_cost)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, show_finance)) {
            long long count = -1;
            std::string _count = result[2];
            sscanf(_count.c_str(), "%lld", &count);
            if ( count > 2147483647 ) {
                Invalid;
            } else {
                String output;
                if (cur_context->show_finance(count, output)) {
                    cout << output << "\n";
                } else {
                    Invalid;
                }
            }
        } else {
            Invalid;
        }
    }
}