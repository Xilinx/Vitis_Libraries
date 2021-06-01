#ifndef _DSPLIB_CONDITIONAL_WIDGET_HPP_
#define _DSPLIB_CONDITIONAL_WIDGET_HPP_

// This file holds the definition of the conditional widget class
/**
 * @file conditional_widget.hpp
 *
 **/

#include <adf.h>
#include <vector>

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

using namespace adf;

template <unsigned int addWidget, unsigned int windowSize, class widgetClass>
class ConditionalWidget {
   public:
    using portConnect = connect<window<windowSize> >;
    ConditionalWidget(){}; // default constructor
    template <typename inType, typename outType>
    static kernel create(port<inType>& inPort, port<outType>& outPort) {
        kernel widget;
        if (addWidget == 1) {
            widget = kernel::create_object<widgetClass>();
            portConnect(inPort, widget.in[0]);
            portConnect(widget.out[0], outPort);
        } else {
            portConnect(inPort, outPort);
        }

        return widget;
    }
};
}
}
}
}
}

#endif