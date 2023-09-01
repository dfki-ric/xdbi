import unittest
import xtypes_generator_py
import xdbi_py
import os

class MyXType(xtypes_generator_py.XType):
    def __init__(self):
        super().__init__("MyXType")
        self.define_property("a property", xtypes_generator_py.JsonValueType.STRING, {"one", "two"}, "one")

class TestClient(unittest.TestCase):
    # test for add/clear/remove/update/load
    def test_Client(self):
        registry = xtypes_generator_py.XTypeRegistry()
        raise NotImplementedError("No python binding for registering XType(s) in registry")
        dbi = xdbi_py.Client(registry)
        my_xtype = MyXType()
        graph = "py_unit_test_client"
        dbi.setWorkingGraph(graph)
        # clear db before adding anything
        assert dbi.clear()
        # Path where the the file is added
        p = os.path.join(os.getenv("TEST_DB_PATH"), graph)
        assert os.path.exists(p), p
        # Add the xtype data
        assert dbi.add([my_xtype])
        assert len(os.listdir(os.path.join(p, "MyXType"))) == 1
        # retrieve the xtype data we added above test whether we can find it
        print(my_xtype.get_properties())
        #assert len(dbi.find("MyXType", my_xtype.get_properties())) > 0
        # test load
        #assert dbi.load(my_xtype.uri(), "MyXType")
        # removing the xtype data
        #dbi.remove(my_xtype.uri())
        #assert len(dbi.find("MyXType", my_xtype.get_properties())) == 0

if __name__ == '__main__':
    unittest.main()
