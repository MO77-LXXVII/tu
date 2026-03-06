#include <iostream>

#include "bank_client.hpp"
#include "../terminal_utils/output.hpp"

using namespace terminal_utils;
namespace tu = terminal_utils;

using namespace std;









int main()
{
    // BankClient p1(Mode::update_mode, "Mohanad", "Eid", "koko@wawa.com", "01022112211", "a3", "0000", 1000.5);
    BankClient p1(BankClient::Mode::delete_mode, "Mohanad", "Eid", "koko@wawa.com", "01022112211", "a3", "0000", 1000.5);


    // p1.save();
    // p2.save();
    // p3.save();

    // p1.print_client_details();
    // p2.print_client_details();
    // p3.print_client_details();

    // BankClient::list_clients();

    // tu::output::Table table;
    // table.add_row({"id", "name", "score"});
    // table.add_row({"id"});
    // table.add_row({"id", "name", "score", "health", "age", "5555555"});
    // table.add_row({"id", "name", "score", "health"});

    // table.print(tu::output::Alignment::Center);




    // p1.delete_client();

    BankClient::list_clients();




}
