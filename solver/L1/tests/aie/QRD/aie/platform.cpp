#include "aie_graphs.hpp"

GivensRotationQRD mygraph("in_vec_l",
                          "data/in_vec_l.txt",
                          "in_vec_h",
                          "data/in_vec_h.txt",
                          "out_vec_l",
                          "data/out_vec_l.txt",
                          "out_vec_h",
                          "data/out_vec_h.txt");

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    printf("A\n");
    adf::return_code ret;
    printf("B\n");
    mygraph.init();
    mygraph.update(mygraph.row_num, (unsigned int)3);
    mygraph.update(mygraph.column_num, (unsigned int)3);
    printf("C\n");
    ret = mygraph.run(1);
    printf("D\n");

    if (ret != adf::ok) {
        printf("Run Failed\n");
        return ret;
    }

    ret = mygraph.end();
    if (ret != adf::ok) {
        printf("End Failed\n");
        return ret;
    }
    return 0;
}
#endif
