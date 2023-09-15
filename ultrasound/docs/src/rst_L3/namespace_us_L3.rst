.. index:: pair: namespace; L3
.. _doxid-namespaceus_1_1_l3:
.. _cid-us::l3:

namespace L3
============

.. toctree::
	:hidden:





.. _doxid-namespaceus_1_1_l3_1a66512891e941838017d77ce6d7597e48:
.. _cid-us::l3::g_scanline:
.. _doxid-namespaceus_1_1_l3_1ac588ff128c48dbddd742e1b1d8a45ad2:
.. _cid-us::l3::plane_wave:
.. _doxid-namespaceus_1_1_l3_1a5ce19108889165ae9b57526c2af68d0f:
.. _cid-us::l3::scanline:
.. _doxid-namespaceus_1_1_l3_1add20bf811e33fb08fc2a03e0e306f244:
.. _cid-us::l3::scanline_allinaie:
.. _doxid-namespaceus_1_1_l3_1af3cb755d65409119bd4ea7e39f9bbee3:
.. _cid-us::l3::synthetic_aperture:
.. ref-code-block:: cpp
	:class: overview-code-block

	// global variables

	graph_scanline <float, :ref:`NUM_LINE_t<doxid-scanline___allin_a_i_e_8hpp_1aabf1945e7d5e52f2da5dcd6a455a5ea3>`, :ref:`NUM_ELEMENT_t<doxid-scanline___allin_a_i_e_8hpp_1ab951932b8608d7fb527999ff80fb34b9>`, :ref:`NUM_SAMPLE_t<doxid-scanline___allin_a_i_e_8hpp_1a32a998d6d1c20a683e4ea8a829bf444c>`, :ref:`NUM_SEG_t<doxid-scanline___allin_a_i_e_8hpp_1af16d0b28edc69e7553cfa3905600bc14>`, :ref:`NUM_DEP_SEG_t<doxid-scanline___allin_a_i_e_8hpp_1ac7e2e31244e10a526a91c580bcf4d8f4>`, :ref:`VECDIM_img_t<doxid-scanline___allin_a_i_e_8hpp_1a09b8cb45ed1521067284e0536d486735>`, :ref:`LEN_OUT_img_t<doxid-scanline___allin_a_i_e_8hpp_1a6e69fdcc24ade3a68778b039a550059f>`, :ref:`LEN32b_PARA_img_t<doxid-scanline___allin_a_i_e_8hpp_1aa6c578751569ca7fbf2d30068abea69b>`, :ref:`VECDIM_foc_t<doxid-scanline___allin_a_i_e_8hpp_1a9a39096db75722b623856558e99fbfa4>`, :ref:`LEN_OUT_foc_t<doxid-scanline___allin_a_i_e_8hpp_1a8976da99620acb53f4b451c54d7cfc5b>`, :ref:`LEN32b_PARA_foc_t<doxid-scanline___allin_a_i_e_8hpp_1a8322f7ffd9336bfd56c7f60d80da27e6>`, :ref:`VECDIM_delay_t<doxid-scanline___allin_a_i_e_8hpp_1a6d710f9b6781b02315288cdc03502c81>`, :ref:`LEN_IN_delay_t<doxid-scanline___allin_a_i_e_8hpp_1a17cca994b1c45408bcb025ef3b0d7ed3>`, :ref:`LEN_OUT_delay_t<doxid-scanline___allin_a_i_e_8hpp_1a85f2ea09842ae5101db7a6cb45d1514a>`, :ref:`LEN32b_PARA_delay_t<doxid-scanline___allin_a_i_e_8hpp_1a6d8b500dd5e79bf2b0654b5b0c33ea3e>`, :ref:`VECDIM_apodi_t<doxid-scanline___allin_a_i_e_8hpp_1a5ccf1d9e873f20003a5e9c2c057b04d6>`, :ref:`LEN_IN_apodi_t<doxid-scanline___allin_a_i_e_8hpp_1a9af0e7586f286a3ca2c1ba21ac399a45>`, :ref:`LEN_OUT_apodi_t<doxid-scanline___allin_a_i_e_8hpp_1aa2863dabbc92458bba1ae44c5373c0d7>`, :ref:`LEN32b_PARA_apodi_t<doxid-scanline___allin_a_i_e_8hpp_1ac7ef8840bf9b916255ad15e0f64236bd>`, :ref:`LEN_IN_apodi_f_t<doxid-scanline___allin_a_i_e_8hpp_1aea806d1e7b1f275da65d99b5d9cb1fbb>`, :ref:`LEN_IN_apodi_d_t<doxid-scanline___allin_a_i_e_8hpp_1abd45abd12dff17c425f8785a3630aa16>`, :ref:`VECDIM_interp_t<doxid-scanline___allin_a_i_e_8hpp_1acf43419b73f891fb9c14ac08884a9d13>`, :ref:`LEN_IN_interp_t<doxid-scanline___allin_a_i_e_8hpp_1ac61d815dc628bf3a89ee955c9d379746>`, :ref:`LEN_IN_interp_rf_t<doxid-scanline___allin_a_i_e_8hpp_1a7c2bf82f73e7fa72221f4586f6926202>`, :ref:`LEN_OUT_interp_t<doxid-scanline___allin_a_i_e_8hpp_1a525249d0267b7c716574aedc8e21d5fd>`, :ref:`NUM_UPSample_t<doxid-scanline___allin_a_i_e_8hpp_1a4c13d22618b149023f510c17f5092dd4>`, :ref:`LEN32b_PARA_interp_t<doxid-scanline___allin_a_i_e_8hpp_1ade3cee6d9ec9d44aeed6f63eb3019ffe>`, :ref:`VECDIM_sample_t<doxid-scanline___allin_a_i_e_8hpp_1ac8d94ac07c6ecbc9b38eebd794d37cae>`, :ref:`LEN_IN_sample_t<doxid-scanline___allin_a_i_e_8hpp_1a3032c8dd4489e3143e8907e56485a63e>`, :ref:`LEN_OUT_sample_t<doxid-scanline___allin_a_i_e_8hpp_1ae6f2326a2d5f2d56b49b43e64df0d861>`, :ref:`LEN32b_PARA_sample_t<doxid-scanline___allin_a_i_e_8hpp_1a757e74e9ccffcd98ff4a6c8d08df0f96>`, :ref:`VECDIM_mult_t<doxid-scanline___allin_a_i_e_8hpp_1a4e2bbd228e4f0123f35956b199c18e0a>`, :ref:`LEN_IN_mult_t<doxid-scanline___allin_a_i_e_8hpp_1add28a848a50ad887a240142eaf4396cb>`, :ref:`LEN_OUT_mult_t<doxid-scanline___allin_a_i_e_8hpp_1ac38f28f88c1cad84f4b9bd624ad368be>`, :ref:`NUM_DEP_SEG_mult_t<doxid-scanline___allin_a_i_e_8hpp_1a19433d8f6de78d7be6568f2c530cb2a4>`, :ref:`MULT_ID_t<doxid-scanline___allin_a_i_e_8hpp_1add691fd2b39e98018ce8b02a1e0782ff>`, :ref:`LEN32b_PARA_mult_t<doxid-scanline___allin_a_i_e_8hpp_1a0ed9cfb9e0f1dc6d8d848708ee27004c>`> g_scanline

