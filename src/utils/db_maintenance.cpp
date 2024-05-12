#include <iostream>
#include <sqlite3.h>
#include <string>
#include <sstream>
#include <random>
#include <iomanip>
#include <openssl/evp.h>

void initialize_db_with_table_and_data(sqlite3* db);
void create_users_table(sqlite3* db);
void add_user(sqlite3* db);
void display_all_users(sqlite3* db);
std::string generate_uuid();
std::string hash_password(const std::string& input);
void delete_all_users(sqlite3* db);

int main() {
    sqlite3* db;
    int rc = sqlite3_open("./../../build/pcd.db", &db);
    if(rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    int choice;
    std::cout << "1. Initialize database with table and data" << std::endl;
    std::cout << "2. Add a new user" << std::endl;
    std::cout << "3. Display all users" << std::endl;
    std::cout << "4. Delete all users" << std::endl;
    std::cout << "Enter your choice: ";
    std::cin >> choice;
    switch(choice) {
        case 1:
            initialize_db_with_table_and_data(db);
            break;
        case 2:
            add_user(db);
            break;
        case 3:
            display_all_users(db);
            break;
        case 4:
            delete_all_users(db);
            break;
        default:
            std::cerr << "Invalid choice" << std::endl;
            break;
    }

    sqlite3_close(db);
    return 0;
}

void initialize_db_with_table_and_data(sqlite3* db) {
    create_users_table(db);
    add_user(db);
}

void create_users_table(sqlite3* db) {
    char* zErrMsg = 0;
    const char* sql = "CREATE TABLE IF NOT EXISTS USERS("  \
          "UUID TEXT PRIMARY KEY," \
          "USERNAME TEXT NOT NULL," \
          "PASSWORD TEXT NOT NULL," \
          "ROLE INT NOT NULL DEFAULT 0);";

    int rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        std::cout << "Table created successfully" << std::endl;
    }
}

void add_user(sqlite3* db) {
    std::string username, password, uuid;
    bool is_admin;
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;
    password = hash_password(password);
    std::cout << "Is the user an admin? (y/n): ";
    char choice;
    std::cin >> choice;
    if(choice == 'y') {
        is_admin = true;
    } else {
        is_admin = false;
    }
    uuid = generate_uuid();
    std::stringstream sql;
    if (is_admin) {
        sql << "INSERT INTO USERS (UUID, USERNAME, PASSWORD, ROLE) VALUES ('" << uuid << "', '" << username << "', '" << password << "', 1);";
    } else {
        sql << "INSERT INTO USERS (UUID, USERNAME, PASSWORD) VALUES ('" << uuid << "', '" << username << "', '" << password << "');";
    }

    char* zErrMsg = 0;
    int rc = sqlite3_exec(db, sql.str().c_str(), 0, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        std::cout << "User added successfully" << std::endl;
    }
}

void display_all_users (sqlite3* db) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM USERS";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if(rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement" << std::endl;
        return;
    }
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::cout << "UUID: " << sqlite3_column_text(stmt, 0) << std::endl;
        std::cout << "Username: " << sqlite3_column_text(stmt, 1) << std::endl;
        std::cout << "Password: " << sqlite3_column_text(stmt, 2) << std::endl;
        std::cout << "Role: " << sqlite3_column_int(stmt, 3) << std::endl;
    }
    sqlite3_finalize(stmt);
}

void delete_all_users (sqlite3* db) {
    const char* sql = "DELETE FROM USERS";
    char* zErrMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        std::cout << "All users deleted successfully" << std::endl;
    }
}

std::string generate_uuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen); 
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

std::string hash_password(const std::string& input) {
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