#pragma once

#include <cassert>
// #include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

#include "string.hpp"

template <class Key, class T>
class Database {
   private:
    const static int BLOCK_SIZE = 1000;
    std::fstream file;
    struct Metadata {
        int head_pos;
    } metadata;
    struct Node {
        int next_pos;
        int n;
        std::pair<Key, T> key_values[BLOCK_SIZE];
    };
    template <class DT>
    void write(const DT& val, int pos) {
        // cerr << "write " << pos << " " << sizeof(DT) << endl;
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(&val), sizeof(DT));
        file.flush();
    }
    template <class DT>
    void read(DT& val, int pos) {
        // cerr << "read " << pos << " " << sizeof(DT) << endl;
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&val), sizeof(DT));
    }
    int cur_pos;

    void _lower_bound(const Key& key, const T& value, Node& node, int& node_pos,
                      int& value_index) {
        value_index = -1;
        const std::pair<Key, T> key_value(key, value);
        for (int pos = metadata.head_pos; pos != -1; pos = node.next_pos) {
            this->read(node, pos);
            node_pos = pos;
            if (node.n && node.key_values[node.n - 1] >= key_value) {
                for (int i = 0; i < node.n; ++i) {
                    if (node.key_values[i] >= key_value) {
                        value_index = i;
                        return;
                    }
                }
                assert(false);
            }
            if (node.next_pos == -1) {
                return;
            }
        }
    }
    void _lower_bound(const Key& key, Node& node, int& node_pos, int& value_index) {
        value_index = -1;
        for (int pos = metadata.head_pos; pos != -1; pos = node.next_pos) {
            this->read(node, pos);
            node_pos = pos;
            if (node.n && node.key_values[node.n - 1].first >= key) {
                for (int i = 0; i < node.n; ++i) {
                    if (node.key_values[i].first >= key) {
                        value_index = i;
                        return;
                    }
                }
                assert(false);
            }
            if (node.next_pos == -1) {
                return;
            }
        }
    }

   public:
    Database(String filename) {
        if (std::filesystem::exists(filename.s)) {
            file.open(filename.s, std::ios::in | std::ios::out | std::ios::binary);
            this->read(metadata, 0);
            file.seekg(0, std::ios::end);
            cur_pos = file.tellg();
        } else {
            file.open(filename.s, std::ios::out | std::ios::binary);
            memset(&metadata, 0, sizeof(metadata));
            metadata.head_pos = sizeof(Metadata);
            this->write(metadata, 0);

            Node head;
            memset(&head, 0, sizeof(Node));
            head.next_pos = -1;
            this->write(head, metadata.head_pos);
            cur_pos = sizeof(Metadata) + sizeof(Node);
            file.close();
            file.open(filename.s, std::ios::in | std::ios::out | std::ios::binary);
        }
    }
    ~Database() { file.close(); }

    void insert(const Key& key, const T& value) {
        Node node;
        int node_pos, value_index;
        this->_lower_bound(key, value, node, node_pos, value_index);
        if (value_index != -1 && value_index < node.n) {
            if (node.key_values[value_index] == std::make_pair(key, value)) {
                node.key_values[value_index] = std::make_pair(key, value);  // to update value
                this->write(node, node_pos);
                return;
            }
        }
        // cerr << "to_insert " << key.s << " " << value << endl;
        if (node.n == BLOCK_SIZE) {
            Node new_node;
            memset(&new_node, 0, sizeof(new_node));
            new_node.next_pos = node.next_pos;
            node.next_pos = cur_pos;
            if (value_index == -1) {
                new_node.key_values[0] = std::make_pair(key, value);
                new_node.n = 1;
            } else {
                new_node.n = node.n - value_index;
                for (int i = value_index; i < node.n; ++i) {
                    new_node.key_values[i - value_index] = node.key_values[i];
                }
                node.n = value_index + 1;
                node.key_values[value_index] = std::make_pair(key, value);
            }
            this->write(new_node, cur_pos);
            this->write(node, node_pos);
            cur_pos += sizeof(Node);
        } else {
            if (value_index == -1) {
                node.key_values[node.n] = std::make_pair(key, value);
            } else {
                for (int i = node.n; i > value_index; --i) {
                    node.key_values[i] = node.key_values[i - 1];
                }
                node.key_values[value_index] = std::make_pair(key, value);
            }
            node.n++;
            this->write(node, node_pos);
        }
    }
    void remove(const Key& key, const T& value) {
        Node node;
        int node_pos, value_index;
        this->_lower_bound(key, value, node, node_pos, value_index);
        if (value_index < node.n && value_index != -1 &&
            node.key_values[value_index] == std::make_pair(key, value)) {
            // cerr << "removed " << node.key_values[value_index].first.s << " " << value << endl;
            node.n--;
            for (int i = value_index; i < node.n; ++i) {
                node.key_values[i] = node.key_values[i + 1];
            }
            if (node.n < BLOCK_SIZE / 3 && node.next_pos != -1) {
                Node next_node;
                this->read(next_node, node.next_pos);
                if (node.n + next_node.n <= BLOCK_SIZE) {
                    for (int i = 0; i < next_node.n; ++i) {
                        node.key_values[node.n++] = next_node.key_values[i];
                    }
                    node.next_pos = next_node.next_pos;
                }
            }
            this->write(node, node_pos);
        }
    }
    std::vector<T> query(Key key) {
        Node node;
        int node_pos, value_index;
        this->_lower_bound(key, node, node_pos, value_index);
        std::vector<T> ret;
        // cerr << node_pos << " " << value_index << " " << node.n << endl;
        // cerr << node.key_values[value_index-1].first.s << " " <<
        // node.key_values[value_index-1].second << endl; cerr <<
        // node.key_values[value_index].first.s << " " << node.key_values[value_index].second <<
        // endl;
        while (node_pos != -1 && value_index < node.n && value_index != -1 &&
               node.key_values[value_index].first == key) {
            ret.push_back(node.key_values[value_index].second);
            value_index++;
            if (value_index == node.n) {
                node_pos = node.next_pos;
                if (node_pos != -1) {
                    this->read(node, node_pos);
                }
                value_index = 0;
            }
        }
        return ret;
    }
    std::vector<T> getall() {
        Node node;
        std::vector<T> ret;
        for (int pos = metadata.head_pos; pos != -1; pos = node.next_pos) {
            this->read(node, pos);
            for (int i = 0; i < node.n; ++i) {
                ret.push_back(node.key_values[i].second);
            }
        }
        return ret;
    }
    std::pair<Key, T> begin() {
        Node node;
        for (int pos = metadata.head_pos; pos != -1; pos = node.next_pos) {
            this->read(node, pos);
            if ( node.n ) {
                return node.key_values[0];
            }
        }
        assert(false);
    }
};
// usage example:
// int main() {
//     Database<int> db("./1.db");
//     db.insert("a", 1);
//     std::vector<int> ret = db.query("a");
//     db.remove("a", 1);
// }