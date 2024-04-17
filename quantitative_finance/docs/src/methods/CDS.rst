.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

*******************
Credit Default Swap
*******************

.. toctree::
   :maxdepth: 1

Overview
========

A Credit Default Swap (CDS) is a financial contract between two counterparties in which one party pays the other party credit protection against possible default of the underlying asset.

A series of premium payments are made at regular intervals, these payments continue to be made as long as the underlying asset survives and do not go into default.

The price of the contract is obtained by computing the sum of the present value of each leg (Premium Leg) and the sum of expected default payments (Protection Leg).


.. math::
        Premium Leg = \sum_{i=1}^N \pi.N.P(T_i).\delta t.DF_i

        Protection Leg = \sum_{i=1}^N N.(1-R).[P(T_{i-1}) - P(T_i)].DF_i

        \pi (CDS Spread) = \frac{Premium Leg}{Protection Leg}


For fair pricing, these legs must be equal with :math:`\pi` being the price of the contract of the fair spread.

:math:`P(T)` - Probability of survival at time T

:math:`DF_i` - Discount Factor at time t

:math:`R` - Recovery Rate

:math:`N` - Notional Value

