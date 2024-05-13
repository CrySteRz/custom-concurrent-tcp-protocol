#pragma once
#include "sqlite3.h"
#include <iostream>
#include <vector>
#include <string>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

class DatabaseHandler {
private:
    sqlite3* db;

public:
    DatabaseHandler() {
        int rc = sqlite3_open("pcd.db", &db);
        if (rc) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        }
    }

    ~DatabaseHandler() {
        sqlite3_close(db);
    }

    

    std::string getID(const std::string& username) {
    std::string id;
    const char* sql = "SELECT UUID FROM USERS WHERE USERNAME = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            id = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
    }
    sqlite3_finalize(stmt);
    return id;
}

std::string getHashedPassword(const std::string& username) {
        std::string hashed_password;
        const char* sql = "SELECT PASSWORD FROM USERS WHERE USERNAME = ?;";
        sqlite3_stmt* stmt;
        
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            return "";
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* result = sqlite3_column_text(stmt, 0);
            if (result) {
                hashed_password = std::string(reinterpret_cast<const char*>(result));
            }
        } else {
            std::cerr << "Error fetching password: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_finalize(stmt);
        return hashed_password;
    }

bool login(const std::string& username, const std::string& password) {
    std::string hashed_input_password = hashPassword(password);
    std::string stored_hashed_password = getHashedPassword(username);
    if (!stored_hashed_password.empty() && hashed_input_password == stored_hashed_password) {
        std::cout << "Login successful." << std::endl;
        return true;
    } else {
        std::cout << "Login failed. Incorrect username or password." << std::endl;
        return false;
    }
}

std::string hashPassword(const std::string& input) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, input.c_str(), input.size());
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);
    std::stringstream ss;
    for (int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool isAdmin(const std::string& username) {
        const char* sql = "SELECT ROLE FROM USERS WHERE USERNAME = ?;";
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            sqlite3_finalize(stmt); 
            return false; 
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int is_admin = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return is_admin != 0; 
        }

        sqlite3_finalize(stmt);
        return false;
    }


};



