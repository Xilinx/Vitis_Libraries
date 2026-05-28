#include <stdio.h>
#include "test.hpp"

xf::solver::aie::test::test_graph substitutionTestHarness;

int main(void) {
    substitutionTestHarness.init();
    substitutionTestHarness.run(NITER);
    substitutionTestHarness.end();

    return 0;
}
