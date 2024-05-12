rm -rf sqlite3.o db_maintenance
gcc -c ./../libs/sqlite3.c -o ./sqlite3.o
g++ db_maintenance.cpp sqlite3.o -o db_maintenance -lssl -lcrypto
./db_maintenance