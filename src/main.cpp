

#include <iostream>
#include <regex>
#include <stack>

#include "context.hpp"

int n;
char buffer[1024];
std::stack<Context*> contexts;

// patterns
// [UserID]: ([0-9a-zA-Z_]+)
// [Password]: ([0-9a-zA-Z_]+)
// [CurrentPassword]: ([0-9a-zA-Z_]+)
// [NewPassword]: ([0-9a-zA-Z_]+)
// [Username]: (.+)
// [Privilege]: ([0137])
const std::regex switch_user("^su ([0-9a-zA-Z_]+) ([0-9a-zA-Z_]+)?$");
const std::regex logout("^logout$");
const std::regex register_user("^register ([0-9a-zA-Z_]+) ([0-9a-zA-Z_]+) (.+)$");
const std::regex change_passwd("^passwd ([0-9a-zA-Z_]+)( ([0-9a-zA-Z_]+))? ([0-9a-zA-Z_]+)$");
const std::regex add_user("^useradd ([0-9a-zA-Z_]+) ([0-9a-zA-Z_]+) ([0137]) (.+)$");
const std::regex delete_user("^delete ([0-9a-zA-Z_]+)$");

// [ISBN]: ([^\\s]+:ISBN)
// [BookName]: ([^\\s\"]+:bookname)
// [Quantity]: ([0-9]+:quantity)
const std::regex show_book(
    "^show(( -(ISBN)=([^\\s]+))|( -(name)=\"([^\\s\"]+)\")|( -(author)=\"([^\\s\"]+)\")|( -(keyword)=\"([^\\s\"]+)\"))?$");
const std::regex buy("^buy ([^\\s]+) ([0-9]+)$");
const std::regex select_book("^select ([^\\s]+)$");
const std::regex modify_book(
    "^modify(( -(ISBN)=([^\\s]+))|( -(name)=\"([^\\s\"]+)\")|( -(author)=\"([^\\s\"]+)\")|( -(keyword)=\"([^\\s\"]+)\")|( -(price)=([0-9\\.]+)))+$");
const std::regex import_book("^import ([0-9]+) ([0-9\\.]+)$");

#define Invalid cout << "Invalid" << "--------------" << __LINE__ << "\n"

using std::cout, std::cerr, std::endl;

int main() {
    contexts.push(Context::get_default_context());
    while (true) {
        std::cin.getline(buffer, 1024);
        const std::string input(buffer);
        std::smatch result;
        cerr << buffer << endl;
        if (input == "exit") {
            break;
        }
        if (std::regex_match(input, result, switch_user)) {
            std::string userid = result[1], passwd = result[2];
            Context* ret = contexts.top()->switch_user(userid, passwd);
            if (ret != nullptr) {
                contexts.push(ret);
            } else {
                Invalid;
            }
        } else if (std::regex_match(input, result, logout)) {
            if (contexts.top()->get_privilege() >= 1) {
                delete contexts.top();
                contexts.pop();
            } else {
                Invalid;
            }
        } else if (std::regex_match(input, result, register_user)) {
            std::string userid = result[1], passwd = result[2], username = result[3];
            if (!contexts.top()->register_user(userid, passwd, username)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, change_passwd)) {
            std::string userid = result[1], cur_passwd, new_passwd;
            cur_passwd = result[3];
            new_passwd = result[4];
            if (!contexts.top()->change_passwd(userid, cur_passwd, new_passwd)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, add_user)) {
            std::string userid = result[1], passwd = result[2], _privilege = result[3],
                        username = result[4];
            int privilege;
            sscanf(_privilege.c_str(), "%d", &privilege);
            if (!contexts.top()->add_user(userid, passwd, privilege, username)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, delete_user)) {
            std::string userid = result[1];
            if (!contexts.top()->delete_user(userid)) {
                Invalid;
            }
        } else if (std::regex_match(input, result, show_book)) {
            std::string filter_type = "", filter = "";
            for (auto p : result) {
                cerr << "\"" << p << "\" ";
            }
            cerr << endl;
            for ( int i = 3; i < result.size(); i += 3) {
                filter_type = result[i];
                filter = result[i+1];
                if (filter_type != "") {
                    break;
                }
            }
            cerr << filter_type << " " << filter << endl;
            std::vector<Book> output;
            if (!contexts.top()->find_book(filter_type, filter, output)) {
                Invalid;
            } else {
                if (output.size() == 0) {
                    std::cout << "\n";
                } else {
                    for (const Book& book : output) {
                        std::cout << book.ISBN << "\t" << book.name << "\t" << book.author << "\t";
                        std::cout << book.keyword << std::fixed << std::setprecision(2) << "\t" << book.price << "\t" << book.quantity
                                  << "\n";
                    }
                }
            }
        } else if (std::regex_match(input, result, buy)) {
            std::string ISBN = result[1], _quantity = result[2];
            int quantity;
            sscanf(_quantity.c_str(), "%d", &quantity);
            double ret = contexts.top()->buy(ISBN, quantity);
            if (ret < 0) {
                Invalid;
            } else {
                std::cout << ret << "\n";
            }
        } else if (std::regex_match(input, result, select_book)) {
            std::string ISBN = result[1];
            contexts.top()->select(ISBN);
        } else if (std::regex_match(input, result, modify_book)) {
            if (result.size() == 1) {
                Invalid;
            } else {
                std::vector<std::pair<String, String>> modifier;
                for (int i = 3; i < result.size(); i += 3) {
                    if (result[i].length()) {
                        modifier.emplace_back(result[i], result[i + 1]);
                        cerr << result[i] << " " << result[i+1] << endl;
                    }
                }
                if (!contexts.top()->modify(modifier)) {
                    Invalid;
                }
            }
        } else if (std::regex_match(input, result, import_book)) {
            std::string _quantity = result[1], _total_cost = result[2];
            int quantity;
            double total_cost;
            sscanf(_quantity.c_str(), "%d", &quantity);
            sscanf(_total_cost.c_str(), "%lf", &total_cost);
            if (!contexts.top()->import_book(quantity, total_cost)) {
                Invalid;
            }
        } else {
            Invalid;
        }
    }
    // TODO: 财务记录查询
}