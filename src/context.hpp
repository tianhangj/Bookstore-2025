#pragma once

#include <set>
#include <vector>

#include "book.hpp"
#include "database.hpp"
#include "user.hpp"

class Context {
   private:
    Context() {
        cur_user = User::default_user();
        login_users = new std::multiset<String>;
        login_users->insert(cur_user.userid);
        select_book = "";
        user_db = new Database<User>("./data/user.db");
        std::vector<User> cur_root = user_db->query("root");
        if (cur_root.empty()) {
            User user_root("root", "root", "sjtu", 7);
            user_db->insert("root", user_root);
        }
        ISBN_db = new Database<Book>("./data/isbn.db");
        bookname_db = new Database<Book>("./data/bookname.db");
        author_db = new Database<Book>("./data/author.db");
        keyword_db = new Database<Book>("./data/keyword.db");
    }
    Context(Context* context, User user) {
        user_db = context->user_db;
        ISBN_db = context->ISBN_db;
        bookname_db = context->bookname_db;
        author_db = context->author_db;
        keyword_db = context->keyword_db;
        login_users = context->login_users;

        cur_user = user;
        select_book = "";
    }

   public:
    std::multiset<String>* login_users;
    User cur_user;
    String select_book;
    Database<User>* user_db;
    Database<Book>* ISBN_db;
    Database<Book>* bookname_db;
    Database<Book>* author_db;
    Database<Book>* keyword_db;

   public:
    Context(Context& context) = default;
    static Context* get_default_context() {
        static Context context;
        return &context;
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
            return new Context(this, user[0]);
        } else {
            return nullptr;
        }
    }
    bool logout() {
        if (this->cur_user.privilege == 0) {
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
        if (this->cur_user.privilege == 7 || user[0].passwd == cur_passwd) {
            user[0].passwd = new_passwd;
            user_db->insert(userid, user[0]);
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
            }
        }
        if (j) {
            _keyword.s[j] = '\0';
            keyword_db->insert(_keyword, book);
        }
    }

    bool find_book(String filter_type, String filter, std::vector<Book>& output) {
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
        book[0].quantity -= quantity;
        this->update_book(book[0]);
        return book[0].price * quantity;
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
        Book original_book = book[0], new_book = book[0];
        for (const auto& [key, value] : modifier) {
            if (key == "ISBN") {
                if (value != original_book.ISBN && ISBN_db->query(key).empty()) {
                    new_book.ISBN = value;
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
                        if (keywords.contains(keywd)) {
                            return false;
                        }
                        keywords.insert(keywd);
                        index = 0;
                        keywd.s[0] = '\0';
                    }
                }
                if (index) {
                    keywd.s[index] = '\0';
                    if (keywords.contains(keywd)) {
                        return false;
                    }
                }
                new_book.keyword = value;
            } else if (key == "price") {
                sscanf(value.s, "%lf", &(new_book.price));
            }
        }
        this->remove_book(original_book);
        this->update_book(new_book);
        this->select_book = new_book.ISBN;
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
        book[0].quantity += quantity;
        this->update_book(book[0]);
        return true;
    }
};