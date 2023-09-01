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
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        // find
        std::vector<XTypePtr> res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 1);
        REQUIRE(res[0].get() != x.get());
        REQUIRE(res[0]->get_property("a_property") == x->get_property("a_property"));
        //// update
        x->set_property("a_property", "two");
        client.update({x});
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        std::vector<XTypePtr> updated = client.find(TestType::classname, {});
        REQUIRE(updated.size() == 1);
        REQUIRE(updated[0].get() != x.get());
        REQUIRE(updated[0]->get_property("a_property") == x->get_property("a_property"));
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
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        // find
        res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 2);
        // clear
        client.clear();
        REQUIRE(client.find("", {}).size() == 0);
        // add a third one
        // NOTE: a_relation for z is unknown
        auto z = std::make_shared<TestType>();
        z->set_property("my_uri", "789");
        y->add_fact("a_relation", z);
        REQUIRE(y->get_facts("a_relation").size() == 1);
        // adding Xtype with depth limit (partial export)
        // 1 means, that x, y and relation to y are stored but not z and relation from y to z
        client.add({x},1);
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        // find
        res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 2);
        // check if partially exported models also get imported correctly
        for (auto c : res)
        {
            // full import
            if (c->uri() == x->uri())
            {
                REQUIRE(c->has_facts("a_relation") == x->has_facts("a_relation"));
            }
            // partial import
            if (c->uri() == y->uri())
            {
                REQUIRE(c->has_facts("a_relation") != y->has_facts("a_relation"));
            }
        }
        // updating Xtype with depth limit (partial export)
        // 2 means, that x, y, z and relation to y, z are stored but z is only partially exported
        // Before we can continue we have the following issue.
        // In the DB we now have a state which is NOT x->y->z and the registry has been filled with this state by the previous find().
        // When we export, we call get_facts() ... this loads the partial y from the DB (since we cleared the registry before)
        // That means that the x->y->z we know is NOT the one in the registry.
        // NOTE: In practice this should not bother us since we do not clear the registry normally
        // So, we do ...
        registry->clear();
        REQUIRE(registry->commit(x, false));
        REQUIRE(registry->commit(y, false));
        REQUIRE(registry->commit(z, false));
        // ... to get the registry in the state we want before updating the DB
        client.update({x});
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        // find
        res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 3);
        // check if partially exported models also get partially imported
        for (auto c : res)
        {
            // full import
            if (c->uri() == x->uri())
                REQUIRE(c->has_facts("a_relation") == x->has_facts("a_relation"));
            // full import
            if (c->uri() == y->uri())
                REQUIRE(c->has_facts("a_relation") == y->has_facts("a_relation"));
            // partial import (but both are unknown)
            if (c->uri() == z->uri())
                REQUIRE(c->has_facts("a_relation") == z->has_facts("a_relation"));
        }
        // test auto loading of unknown xtypes when resolving via get_facts
        // first drop all pointers in the registry (forces loading from DB)
        registry->clear();
        // we check the state
        REQUIRE(registry->knows_uri(x->uri()) == false);
        REQUIRE(registry->knows_uri(y->uri()) == false);
        REQUIRE(registry->knows_uri(z->uri()) == false);
        // when we load x only x is known to the registry ... the facts to y are unresolved (yet)
        auto x2 = client.load(x->uri());
        REQUIRE(registry->knows_uri(x->uri()) == true);
        REQUIRE(registry->knows_uri(y->uri()) == false);
        REQUIRE(registry->knows_uri(z->uri()) == false);
        // now we want to resolve y by asking for x's facts (triggers auto load)
        auto x2_facts = x2->get_facts("a_relation");
        REQUIRE(registry->knows_uri(x->uri()) == true);
        REQUIRE(registry->knows_uri(y->uri()) == true);
        REQUIRE(registry->knows_uri(z->uri()) == false);
        // now we want to resolve z by asking for y's facts (triggers another auto load)
        auto y2_facts = x2_facts[0].target.lock()->get_facts("a_relation");
        REQUIRE(registry->knows_uri(x->uri()) == true);
        REQUIRE(registry->knows_uri(y->uri()) == true);
        REQUIRE(registry->knows_uri(z->uri()) == true);
        // TODO: check that inverse relations are filled by add_fact() and get_facts()
        SECTION("Test remove with delete policy")
        {
            // remove y
            // y is a child of x, so x is not affected, but z is a child of y, so y and z get deleted
            client.remove(y->uri());
            REQUIRE(client.find(TestType::classname, {}).size() == 1);
        }
        // clear
        client.clear();
        registry->clear();
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
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        // find
        std::vector<XTypePtr> res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 1);
        REQUIRE(res[0].get() != x.get());
        REQUIRE(res[0]->get_property("a_property") == x->get_property("a_property"));
        //// update
        x->set_property("a_property", "two");
        client.update({x});
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        std::vector<XTypePtr> updated = client.find(TestType::classname, {});
        REQUIRE(updated.size() == 1);
        REQUIRE(updated[0].get() != x.get());
        REQUIRE(updated[0]->get_property("a_property") == x->get_property("a_property"));
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
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        // find
        std::vector<XTypePtr> res = client.find(TestType::classname, {});
        REQUIRE(res.size() == 1);
        REQUIRE(res[0].get() != x.get());
        REQUIRE(res[0]->get_property("a_property") == x->get_property("a_property"));
        //// update
        x->set_property("a_property", "two");
        client.update({x});
        // To really test if x is correctly resolved, we have to clear the registry!!!
        registry->clear();
        std::vector<XTypePtr> updated = client.find(TestType::classname, {});
        REQUIRE(updated.size() == 1);
        REQUIRE(updated[0].get() != x.get());
        REQUIRE(updated[0]->get_property("a_property") == x->get_property("a_property"));
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
    REQUIRE(not registry->knows_class(x->get_classname()));
    REQUIRE(not registry->knows_uri(x->uri()));
    registry->register_class<TestType>();
    registry->commit(x, false);
    REQUIRE(registry->knows_uri(x->uri()));
    // NOTE: get_by_uri() returns a temporary copy of the valid instance inside the registry.
    REQUIRE(registry->get_by_uri(x->uri())->uri() == x->uri());
    registry->drop(x->uri());
    REQUIRE(not registry->knows_uri(x->uri()));
}
