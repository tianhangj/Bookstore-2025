#pragma once

#include <filesystem>
#include <format>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "book.hpp"
#include "database.hpp"
#include "string.hpp"
#include "user.hpp"

class Context {
   private:
    template <class... Arg>
    void log(std::string str) {
        auto [timestamp, _str] = log_db->begin();
        log_db->insert(timestamp - 1, str);
    }
    Context() {
        if (!std::filesystem::exists("./data")) {
            std::filesystem::create_directory("./data");
        }
        cur_user = User::default_user();
        login_users = new std::multiset<String>;
        login_users->insert(cur_user.userid);
        select_book = "";
        user_db = new Database<String, User>("./data/user.db");
        std::vector<User> cur_root = user_db->query("root");
        if (cur_root.empty()) {
            User user_root("root", "root", "sjtu", 7);
            user_db->insert("root", user_root);
        }
        ISBN_db = new Database<String, Book>("./data/isbn.db");
        bookname_db = new Database<String, Book>("./data/bookname.db");
        author_db = new Database<String, Book>("./data/author.db");
        keyword_db = new Database<String, Book>("./data/keyword.db");

        finance_db = new Database<int, std::pair<double, double>>("./data/finance.db");
        auto finance_root = finance_db->query(0);
        if (finance_root.empty()) {
            finance_db->insert(0, std::make_pair(0.0, 0.0));
        }
        log_db = new Database<int, String>("./data/log.db");
        auto log_first = log_db->query(0);
        if (log_first.empty()) {
            log_db->insert(0, "Bookstore initialized.");
        }
        employee_log_db = new Database<String, String>("./data/emply.db");
        book_finance_db = new Database<String, std::pair<double, double>>("./data/book_finance.db");
        this->father_context = nullptr;
    }
    Context(Context* context, User user) {
        login_users = context->login_users;

        user_db = context->user_db;
        ISBN_db = context->ISBN_db;
        bookname_db = context->bookname_db;
        author_db = context->author_db;
        keyword_db = context->keyword_db;
        finance_db = context->finance_db;
        log_db = context->log_db;
        employee_log_db = context->employee_log_db;
        book_finance_db = context->book_finance_db;

        cur_user = user;
        select_book = "";
        father_context = context;
    }

   public:
    std::multiset<String>* login_users;
    User cur_user;
    String select_book;
    Database<String, User>* user_db;
    Database<String, Book>* ISBN_db;
    Database<String, Book>* bookname_db;
    Database<String, Book>* author_db;
    Database<String, Book>* keyword_db;
    Database<int, std::pair<double, double>>* finance_db;
    Database<int, String>* log_db;
    Database<String, String>* employee_log_db;
    Database<String, std::pair<double, double>>* book_finance_db;
    struct Context* father_context;

   public:
    Context(Context& context) = default;
    static Context* get_default_context() {
        static Context context;
        return &context;
    }
    void close() {
        delete login_users;

        delete this->user_db;

        delete this->ISBN_db;
        delete this->bookname_db;
        delete this->author_db;
        delete this->keyword_db;

        delete this->finance_db;
    }
    int get_privilege() { return cur_user.privilege; }
    Context* switch_user(String userid, String passwd) {
        std::vector<User> user = user_db->query(userid);
        if (user.empty()) {
            return nullptr;
        }
        assert(user.size() == 1);
        if (this->cur_user.privilege > user[0].privilege || user[0].passwd == passwd) {
            login_users->insert(user[0].userid);
            log(std::format("[INFO]user[id={}]: Switched to user[id={}]", cur_user.userid.s,
                            userid.s));
            return new Context(this, user[0]);
        } else {
            return nullptr;
        }
    }
    bool logout() {
        if (this->cur_user.privilege < 1) {
            return false;
        }
        login_users->erase(login_users->lower_bound(this->cur_user.userid));
        return true;
    }

    bool register_user(String userid, String passwd, String username) {
        std::vector<User> user = user_db->query(userid);
        if (!user.empty()) {
            return false;
        }
        user_db->insert(userid, User(userid, username, passwd, 1));
        employee_log_db->insert(cur_user.userid.s, std::format("Register user[id={}]", userid.s));
        log(std::format("[INFO]user[id={}]: Register user[id={}]", cur_user.userid.s, userid.s));
        return true;
    }

    bool change_passwd(String userid, String cur_passwd, String new_passwd) {
        if (this->cur_user.privilege < 1) {
            return false;
        }
        std::vector<User> user = user_db->query(userid);
        if (user.empty()) {
            return false;
        }
        assert(user.size() == 1);
        if ((this->cur_user.privilege == 7 && cur_passwd == "") || user[0].passwd == cur_passwd) {
            user_db->remove(userid, user[0]);
            user[0].passwd = new_passwd;
            user_db->insert(userid, user[0]);
            employee_log_db->insert(cur_user.userid.s,
                                    std::format("Changed passwd user[id={}]", userid.s));
            log(std::format("[INFO]Changed passwd user[id={}]", cur_user.userid.s, userid.s));
            return true;
        } else {
            return false;
        }
    }

    bool add_user(String userid, String passwd, int privilege, String username) {
        if (this->cur_user.privilege < 3) {
            return false;
        }
        std::vector<User> user = user_db->query(userid);
        if (!user.empty()) {
            return false;
        }
        if (privilege >= this->cur_user.privilege) {
            return false;
        }
        user_db->insert(userid, User(userid, username, passwd, privilege));
        employee_log_db->insert(cur_user.userid.s, std::format("Add user[id={}]", userid.s));
        log(std::format("[INFO]user[id={}]: Add user[id={}]", cur_user.userid.s, userid.s));
        return true;
    }

    bool delete_user(String userid) {
        if (this->cur_user.privilege < 7) {
            return false;
        }
        std::vector<User> user = user_db->query(userid);
        if (user.empty()) {
            return false;
        }
        if (login_users->contains((user[0].userid))) {
            return false;
        }
        user_db->remove(userid, user[0]);
        employee_log_db->insert(cur_user.userid.s, std::format("Delete user[id={}]", userid.s));
        log(std::format("[INFO]user[id={}]: Delete user[id={}]", cur_user.userid.s, userid.s));
        return true;
    }

    void remove_book(Book book) {
        ISBN_db->remove(book.ISBN, book);
        bookname_db->remove(book.name, book);
        author_db->remove(book.author, book);
        String _keyword;
        int j = 0;
        for (int i = 0; book.keyword.s[i]; ++i) {
            if (book.keyword.s[i] != '|') {
                _keyword.s[j++] = book.keyword.s[i];
            } else {
                _keyword.s[j] = '\0';
                keyword_db->remove(_keyword, book);
                j = 0;
                _keyword.s[j] = '\0';
            }
        }
        if (j) {
            _keyword.s[j] = '\0';
            keyword_db->remove(_keyword, book);
        }
    }

    void update_book(Book book) {
        ISBN_db->insert(book.ISBN, book);
        bookname_db->insert(book.name, book);
        author_db->insert(book.author, book);
        String _keyword;
        int j = 0;
        for (int i = 0; book.keyword.s[i]; ++i) {
            if (book.keyword.s[i] != '|') {
                _keyword.s[j++] = book.keyword.s[i];
            } else {
                _keyword.s[j] = '\0';
                keyword_db->insert(_keyword, book);
                j = 0;
                _keyword.s[j] = '\0';
            }
        }
        if (j) {
            _keyword.s[j] = '\0';
            keyword_db->insert(_keyword, book);
        }
    }

    bool find_book(String filter_type, String filter, std::vector<Book>& output) {
        if (this->cur_user.privilege < 1) {
            return false;
        }
        employee_log_db->insert(cur_user.userid.s,
                                std::format("Find book: -{}={}", filter_type.s, filter.s));
        log(std::format("[INFO]user[id={}]: Find book: -{}={}", cur_user.userid.s, filter_type.s,
                        filter.s));
        if (filter_type == "ISBN") {
            output = ISBN_db->query(filter);
            return true;
        } else if (filter_type == "name") {
            output = bookname_db->query(filter);
            return true;
        } else if (filter_type == "author") {
            output = author_db->query(filter);
            return true;
        } else if (filter_type == "keyword") {
            for (int i = 0; filter.s[i]; ++i) {
                if (filter.s[i] == '|') {
                    return false;
                }
            }
            output = keyword_db->query(filter);
            return true;
        } else {
            output = ISBN_db->getall();
            return true;
        }
    }

    bool select(String ISBN) {
        if (cur_user.privilege < 3) {
            return false;
        }
        std::vector<Book> book = ISBN_db->query(ISBN);
        if (book.empty()) {
            Book b;
            b.ISBN = ISBN;
            update_book(b);
        }
        this->select_book = ISBN;
        log(std::format("[INFO]user[id={}]: Select book: {}", cur_user.userid.s, ISBN.s));
        return true;
    }

    double buy(String ISBN, int quantity) {
        if (cur_user.privilege < 1) {
            return -1;
        }
        if (quantity <= 0) {
            return -1;
        }
        std::vector<Book> book = ISBN_db->query(ISBN);
        if (book.empty()) {
            return -1;
        }
        assert(book.size() == 1);
        if (book[0].quantity < quantity) {
            return -1;
        }
        book[0].quantity -= quantity;
        this->update_book(book[0]);
        double cost = book[0].price * quantity;
        auto [timestamp, p] = finance_db->begin();
        finance_db->insert(timestamp - 1, std::make_pair(p.first + cost, p.second));
        auto book_finance = book_finance_db->query(ISBN);
        if (book_finance.empty()) {
            assert(false);
        } else {
            book_finance_db->remove(ISBN, book_finance[0]);
            book_finance[0].second += cost;
            book_finance_db->insert(ISBN, book_finance[0]);
        }
        employee_log_db->insert(
            cur_user.userid.s, std::format("Buy book: {} {}", cur_user.userid.s, ISBN.s, quantity));
        log(std::format("[INFO]user[id={}]: Buy book: {} {}", cur_user.userid.s, ISBN.s, quantity));
        return cost;
    }

    bool modify(std::vector<std::pair<String, String>> modifier) {
        if (cur_user.privilege < 3) {
            return false;
        }
        if (this->select_book == "") {
            return false;
        }
        std::vector<Book> book = ISBN_db->query(this->select_book);
        if (book.empty()) {
            return false;
        }
        if (modifier.size() == 0) {
            return false;
        }
        Book original_book = book[0], new_book = book[0];
        for (const auto& [key, value] : modifier) {
            if (key == "ISBN") {
                if (ISBN_db->query(value).empty()) {
                    new_book.ISBN = value;
                } else {
                    return false;
                }
            } else if (key == "name") {
                new_book.name = value;
            } else if (key == "author") {
                new_book.author = value;
            } else if (key == "keyword") {
                std::set<String> keywords;
                String keywd;
                int index = 0;
                for (int i = 0; value.s[i]; ++i) {
                    if (value.s[i] != '|') {
                        keywd.s[index++] = value.s[i];
                    } else {
                        keywd.s[index] = '\0';
                        if (index == 0) {
                            return false;
                        }
                        if (keywords.contains(keywd)) {
                            return false;
                        }
                        keywords.insert(keywd);
                        index = 0;
                        keywd.s[0] = '\0';
                    }
                }
                if (index == 0 && value.s[0]) {
                    return false;
                }
                keywd.s[index] = '\0';
                if (keywords.contains(keywd)) {
                    return false;
                }
                new_book.keyword = value;
            } else if (key == "price") {
                if (strlen(value.s) > 13) {
                    return false;
                }
                double _price;
                sscanf(value.s, "%lf", &(_price));
                new_book.price = _price;
            } else {
                assert(false);
            }
        }
        this->remove_book(original_book);
        this->update_book(new_book);
        for (struct Context* i = this; i != nullptr; i = i->father_context) {
            if (i->select_book == original_book.ISBN) {
                i->select_book = new_book.ISBN;
            }
        }
        employee_log_db->insert(cur_user.userid.s,
                                std::format("Modify book[{}]", cur_user.userid.s, select_book.s));
        log(std::format("[INFO]user[id={}]: Modify book[{}]", cur_user.userid.s, select_book.s));
        return true;
    }

    bool import_book(int quantity, double total_cost) {
        if (cur_user.privilege < 3) {
            return false;
        }
        if (this->select_book == "") {
            return false;
        }
        if (quantity <= 0 || total_cost <= 0) {
            return false;
        }
        std::vector<Book> book = ISBN_db->query(this->select_book);
        if (book.empty()) {
            return false;
        }
        auto [timestamp, p] = finance_db->begin();
        finance_db->insert(timestamp - 1, std::make_pair(p.first, p.second + total_cost));
        book[0].quantity += quantity;
        this->update_book(book[0]);
        auto book_finance = book_finance_db->query(select_book);
        if (book_finance.empty()) {
            book_finance_db->insert(select_book, std::make_pair(total_cost, 0));
        } else {
            book_finance_db->remove(select_book, book_finance[0]);
            book_finance[0].first += total_cost;
            book_finance_db->insert(select_book, book_finance[0]);
        }
        employee_log_db->insert(cur_user.userid.s,
                                std::format("Import book[{}] {}/{}", cur_user.userid.s,
                                            select_book.s, total_cost, quantity));
        log(std::format("[INFO]user[id={}]: Import book[{}] {}/{}", cur_user.userid.s,
                        select_book.s, total_cost, quantity));
        return true;
    }

    bool show_finance(int count, String& out) {
        if (this->cur_user.privilege < 7) {
            return false;
        }
        if (count == 0) {
            out = "";
            return true;
        }
        auto [timestamp, p] = finance_db->begin();
        auto [a, b] = p;
        if (count != -1) {
            if (count + timestamp > 0) {
                return false;
            }
            std::vector<std::pair<double, double>> ret = finance_db->query(timestamp + count);
            assert(ret.size() == 1);
            a -= ret[0].first;
            b -= ret[0].second;
        }
        sprintf(out.s, "+ %.2lf - %.2lf", a, b);
        return true;
    }
    bool log_all(std::vector<String>& out) {
        if (this->cur_user.privilege < 7) {
            return false;
        }
        out = log_db->getall();
        return true;
    }
    bool report_employee(std::map<String, std::vector<String>>& out) {
        if (this->cur_user.privilege < 7) {
            return false;
        }
        auto users = user_db->getall();
        for (auto user : users) {
            if (user.privilege >= 3) {
                out[user.userid] = employee_log_db->query(user.userid);
            }
        }
        return true;
    }
    bool report_finance(std::map<String, std::pair<double, double>>& out) {
        if (this->cur_user.privilege < 7) {
            return false;
        }
        auto books = ISBN_db->getall();
        for (auto book : books) {
            auto finance = book_finance_db->query(book.ISBN);
            if (!finance.empty()) {
                out[book.ISBN] = finance[0];
            }
        }
        return true;
    }
};