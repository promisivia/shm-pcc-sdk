// #include <string>
// #include <vector>

// #include "src/

// int main() {
//   ycsbc::BwTreeDB* db = new ycsbc::BwTreeDB(2);
//   db->ThreadInit();
//   std::vector<DB::KVPair> pairs;
//   std::vector<std::string> fie;
//   std::string table = "user";
//   for (int i = 0; i < 100000; i++) {
//     db->Insert(table, to_string(i), pairs);
//   }
//   for (int i = 0; i < 100000; i++) {
//     string value;
//     db->Read(table, to_string(i), &fie, pairs);
//   }
// }