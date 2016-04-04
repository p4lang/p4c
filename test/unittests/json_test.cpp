#include "../../lib/json.h"
#include "test.h"

using namespace Util;

namespace Test {
class TestJson : public TestBase {
    int testJson() {
        IJson* value;
        value = new JsonValue(true);
        ASSERT_EQ(value->toString(), "true");
        value = new JsonValue();
        ASSERT_EQ(value->toString(), "null");
        value = new JsonValue(JsonValue::Kind::True);
        ASSERT_EQ(value->toString(), "true");
        value = new JsonValue("5");
        ASSERT_EQ(value->toString(), "\"5\"");
        value = new JsonValue(5);
        ASSERT_EQ(value->toString(), "5");

        auto arr = new JsonArray();
        arr->append(5);
        ASSERT_EQ(arr->toString(), "[5]");
        arr->append("5");
        ASSERT_EQ(arr->toString(), "[5, \"5\"]");

        auto arr1 = new JsonArray();
        arr->append(arr1);
        ASSERT_EQ(arr->toString(), "[\n  5,\n  \"5\",\n  []\n]");
        arr1->append(true);
        ASSERT_EQ(arr->toString(), "[\n  5,\n  \"5\",\n  [true]\n]");

        auto obj = new JsonObject();
        ASSERT_EQ(obj->toString(), "{\n}");
        obj->emplace("x", "x");
        ASSERT_EQ(obj->toString(), "{\n  \"x\" : \"x\"\n}");
        obj->emplace("y", arr);
        ASSERT_EQ(obj->toString(), "{\n  \"x\" : \"x\",\n  \"y\" : [\n    5,\n    \"5\",\n    [true]\n  ]\n}");
        
        return SUCCESS;
    }
 public:
    int run() {
        RUNTEST(testJson);
        return SUCCESS;
    }
};
}  // namespace Test


int main(int, char* []) {
    Test::TestJson test;
    return test.run();
}
