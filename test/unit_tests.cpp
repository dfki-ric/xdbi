#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <iostream>
#include "Client.hpp"
#include "Serverless.hpp"

#include "MultiDbClient.hpp"

#include <xtypes_generator/XTypeRegistry.hpp>
#include "ProjectRegistry.hpp"
#include "TestType.hpp"

using namespace xtypes;
using namespace xdbi;
using namespace xdbi_test_type;
namespace nl = nlohmann;

const std::string db_address = xdbi::DEFAULT_DB_ADDRESS;
const std::string graph = "cpp_unit_test";
const std::string db_path = std::getenv("TEST_DB_PATH");

nl::json readJson(const std::string &filename)
{
    std::ifstream file;
    nl::json jsonFile;
    file.open(filename);
    if (!file)
    {
        std::cerr << "Error: Couldn't open" << std::endl;
    }
    else
    {
        file >> jsonFile;
    }
    return jsonFile;
}

TEST_CASE("Test database interfaces", "[DbInterface]")
{
    auto registry = std::make_shared<ProjectRegistry>();
    auto x = registry->instantiate<TestType>();
    REQUIRE(x->has_relation("a_relation") == true);
    REQUIRE(x->has_facts("a_relation") == false);  // facts should be unknown
    // if they'd be empty:
    // REQUIRE(x->get_facts("a_relation").size() < 1);

    SECTION("Test Client interface", "[Client]")
    {
        Client client = Client(registry, db_address, "");
        REQUIRE(!client.isReady());
        client.setWorkingGraph(graph);
        // clear db before adding anything
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
        // add
        client.add({x});
        // find
        std::vector<XTypePtr> res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 1);
        REQUIRE(res[0]->get_property("a_property") == x->get_property("a_property"));
        // update
        x->set_property("a_property", "two");
        client.update({x});
        std::vector<XTypePtr> updated = client.find(TestType::classname, {});
        REQUIRE(updated.size() == 1);
        REQUIRE(updated[0]->get_property("a_property") == x->get_property("a_property"));
        REQUIRE(res[0]->get_property("a_property") != updated[0]->get_property("a_property"));
        SECTION("Test remove")
        {
            // remove
            client.remove(x->uri());
            REQUIRE(client.find(TestType::classname, {}).size() == 0);
        }
        // clear
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
        // create another test type and relate it to x
        auto y = std::make_shared<TestType>();
        y->set_property("my_uri", "456");
        REQUIRE(x->uri() != y->uri());
        x->add_fact("a_relation", y);
        REQUIRE(x->get_facts("a_relation").size() == 1);
        // adding multiple Xtypes
        client.add({x});
        // find
        res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 2);
        // add a third one
        auto z = std::make_shared<TestType>();
        z->set_property("my_uri", "789");
        y->add_fact("a_relation", z);
        REQUIRE(y->get_facts("a_relation").size() == 1);
        // clear
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
        // adding Xtype with depth limit (partial export)
        // 1 means, that x, y and relation to y are stored but not z and relation from y to z
        client.add({x},1);
        // find
        res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 2);
        // check if partially exported models also get partially imported
        for (auto c : res)
        {
            // full import
            if (c->uri() == x->uri())
                REQUIRE(c->has_facts("a_relation") == true);
            // partial import
            if (c->uri() == y->uri())
                REQUIRE(c->has_facts("a_relation") == false);
        }
        // updating Xtype with depth limit (partial export)
        // 2 means, that x, y, z and relation to y, z are stored but z is only partially exported
        client.update({x},2);
        // find
        res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 3);
        // check if partially exported models also get partially imported
        for (auto c : res)
        {
            // full import
            if (c->uri() == x->uri())
                REQUIRE(c->has_facts("a_relation") == true);
            // full import
            if (c->uri() == y->uri())
                REQUIRE(c->has_facts("a_relation") == true);
            // partial import
            if (c->uri() == z->uri())
                REQUIRE(c->has_facts("a_relation") == false);
        }
        // find with search_depth limit
        // 1 means, that x and y are imported but y only partially
        res = client.find(TestType::classname, {{"uri",x->uri()}}, 1);
        REQUIRE(res.size() == 1);
        // check if xtypes are fully/partially imported
        REQUIRE(res[0]->uri() == x->uri());
        REQUIRE(res[0]->has_facts("a_relation") == true);
        REQUIRE(res[0]->get_facts("a_relation").size() == 1);
        auto partial = res[0]->get_facts("a_relation")[0].target.lock();
        REQUIRE(partial->uri() == y->uri());
        REQUIRE(partial->has_facts("a_relation") == false);
        SECTION("Test remove with delete policy")
        {
            // remove y
            // y is a child of x, so x is not affected, but z is a child of y, so y and z get deleted
            client.remove(y->uri());
            REQUIRE(client.find(TestType::classname, {}).size() == 1);
        }
        // TODO: Test add() with partially defined XType
        // TODO: Test update() with partially defined XType
        // clear
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
    }

    SECTION("Test Serverless interface", "[Serverless]")
    {
        Serverless client = Serverless(registry, db_path, "");
        REQUIRE(!client.isReady());
        client.setWorkingGraph(graph);
        // clear db before adding anything
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
        // add
        client.add({x});
        // find
        std::vector<XTypePtr> res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 1);
        REQUIRE(res[0]->get_property("a_property") == x->get_property("a_property"));
        // update
        x->set_property("a_property", "two");
        client.update({x});
        std::vector<XTypePtr> updated = client.find(TestType::classname, {});
        REQUIRE(updated.size() == 1);
        REQUIRE(updated[0]->get_property("a_property") == x->get_property("a_property"));
        REQUIRE(res[0]->get_property("a_property") != updated[0]->get_property("a_property"));
        SECTION("Test remove")
        {
            // remove
            client.remove(x->uri());
            REQUIRE(client.find(TestType::classname, {}).size() == 0);
        }
        // clear
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
    }

    SECTION("Test MultiDbClient interface", "[MultiDbClient]")
    {
        nl::json config;
        std::string config_path = std::getenv("TEST_MULTIDB_CONFIG");
        config = readJson(config_path);
        MultiDbClient client = MultiDbClient(registry, config);
        
        REQUIRE(client.isReady());
        REQUIRE(client.getImportServerWorkingGraph("my_client") == "graph_production");
        // clear db before adding anything
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
        // add
        client.add({x});
        // find
        std::vector<XTypePtr> res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 1);
        REQUIRE(res[0]->get_property("a_property") == x->get_property("a_property"));
        // update
        x->set_property("a_property", "two");
        client.update({x});
        std::vector<XTypePtr> updated = client.find(TestType::classname, {});
        REQUIRE(updated.size() == 1);
        REQUIRE(updated[0]->get_property("a_property") == x->get_property("a_property"));
        REQUIRE(res[0]->get_property("a_property") != updated[0]->get_property("a_property"));
        SECTION("Test remove")
        {
            // remove
            client.remove(x->uri());
            REQUIRE(client.find(TestType::classname, {}).size() == 0);
        }
        // clear
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
    }
}

TEST_CASE("Ping server", "ping pong")
{
    using namespace std::literals;
    auto registry = std::make_shared<XTypeRegistry>();
    Client client = Client(registry, db_address, graph);
    for (int i = 1; i <= 5; i++)
    {
        std::time_t ms = client.ping();
        REQUIRE(ms != -1);
        std::cout << '#' << i << ": Server took " << ms << " milliseconds to respond. " << std::endl;
        std::this_thread::sleep_for(1s);
    }
}

TEST_CASE("Test XTypeRegistry", "XTypeRegistry")
{
    auto registry = std::make_shared<XTypeRegistry>();
    auto x = std::make_shared<TestType>();
    REQUIRE(not registry->holds(x));
    registry->register_class<TestType>();
    registry->hold(x);
    REQUIRE(registry->holds(x));
    registry->drop(x);
    REQUIRE(not registry->holds(x));
}
