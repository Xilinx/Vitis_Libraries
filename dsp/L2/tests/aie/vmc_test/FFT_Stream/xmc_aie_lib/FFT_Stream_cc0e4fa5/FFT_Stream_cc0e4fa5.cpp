#include <adf.h>
#include "FFT_Stream_cc0e4fa5.h"

class DUT : public adf::graph {
public:
	FFT_Stream_cc0e4fa5 mygraph;
	adf::input_plio DUT_in[2];
	adf::output_plio DUT_out[2];

	DUT() {
		DUT_in[0] = adf::input_plio::create("DUT_in[0]", adf::plio_32_bits, "data/i0");
		DUT_in[1] = adf::input_plio::create("DUT_in[1]", adf::plio_32_bits, "data/i1");
		DUT_out[0] = adf::output_plio::create("DUT_out[0]", adf::plio_32_bits, "data/o0");
		DUT_out[1] = adf::output_plio::create("DUT_out[1]", adf::plio_32_bits, "data/o1");

		adf::connect<> ni0(DUT_in[0].out[0], mygraph.in[0]);
		adf::connect<> ni1(DUT_in[1].out[0], mygraph.in[1]);
		adf::connect<> no0(mygraph.out[0], DUT_out[0].in[0]);
		adf::connect<> no1(mygraph.out[1], DUT_out[1].in[0]);
	}
};

DUT g;

#ifdef __AIESIM__
int main(void)
{
	g.init();
	g.run();
	g.end();
	return 0;
}
#endif
