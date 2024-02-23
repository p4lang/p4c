#include "backends/p4tools/modules/testgen/test/lib/p4info_api.h"

#include "backends/p4tools/common/control_plane/p4info_map.h"

namespace Test {

namespace {

using P4::ControlPlaneAPI::p4rt_id_t;
using P4::ControlPlaneAPI::szudzikPairing;

TEST_F(P4RuntimeApiTest, SzudzikPairingisCorrect) {
    EXPECT_EQ(szudzikPairing(0, 0), 0);
    EXPECT_EQ(szudzikPairing(0, 1), 1);
    EXPECT_EQ(szudzikPairing(1, 0), 2);
    EXPECT_EQ(szudzikPairing(3, 5), 28);
    EXPECT_EQ(szudzikPairing(5, 3), 33);
    EXPECT_EQ(szudzikPairing(UINT32_MAX, UINT32_MAX), UINT64_MAX);
    EXPECT_EQ(szudzikPairing(0, UINT32_MAX), 18446744065119617025U);
    EXPECT_EQ(szudzikPairing(UINT32_MAX, 0), 18446744069414584320U);
}

}  // namespace

}  // namespace Test
