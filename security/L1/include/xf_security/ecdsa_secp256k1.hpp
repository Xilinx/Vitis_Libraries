/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file ecdsa.hpp
 * @brief header file for Elliptic Curve Digital Signature Algorithm
 * related function. Now it support curve secp256k1.
 * This file is part of Vitis Security Library.
 */

#ifndef _XF_SECURITY_ECDSA_SECP256K1_HPP_
#define _XF_SECURITY_ECDSA_SECP256K1_HPP_

#include <ap_int.h>
#include "xf_security/ecc.hpp"

namespace xf {
namespace security {
/**
 * @brief Elliptic Curve Digital Signature Algorithm on curve secp256k1.
 * This class provide signing and verifying functions.
 *
 * @tparam HashW Bit Width of digest that used for signting and verifying.
 */
template <int HashW>
class ecdsaSecp256k1 {
   public:
    // Elliptic-curve definition parameter for y^2 = x^3 + ax + b in GF(p)
    ap_uint<256> a;
    /// Elliptic-curve definition parameter for y^2 = x^3 + ax + b in GF(p)
    ap_uint<256> b;
    /// Elliptic-curve definition parameter for y^2 = x^3 + ax + b in GF(p)
    ap_uint<256> p;

    /// X coordinate of generation point of curve secp256k1.
    ap_uint<256> Gx;
    /// Y coordinate of generation point of curve secp256k1.
    ap_uint<256> Gy;
    /// Order of curve secp256k1.
    ap_uint<256> n;

    ap_uint<256> preComputeX[64] = {ap_uint<256>("0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"),
                                    ap_uint<256>("0xE60FCE93B59E9EC53011AABC21C23E97B2A31369B87A5AE9C44EE89E2A6DEC0A"),
                                    ap_uint<256>("0x8282263212C609D9EA2A6E3E172DE238D8C39CABD5AC1CA10646E23FD5F51508"),
                                    ap_uint<256>("0x175E159F728B865A72F99CC6C6FC846DE0B93833FD2222ED73FCE5B551E5B739"),
                                    ap_uint<256>("0x363D90D447B00C9C99CEAC05B6262EE053441C7E55552FFE526BAD8F83FF4640"),
                                    ap_uint<256>("0x8B4B5F165DF3C2BE8C6244B5B745638843E4A781A15BCD1B69F79A55DFFDF80C"),
                                    ap_uint<256>("0x723CBAA6E5DB996D6BF771C00BD548C7B700DBFFA6C0E77BCB6115925232FCDA"),
                                    ap_uint<256>("0xEEBFA4D493BEBF98BA5FEEC812C2D3B50947961237A919839A533ECA0E7DD7FA"),
                                    ap_uint<256>("0x100F44DA696E71672791D0A09B7BDE459F1215A29B3C03BFEFD7835B39A48DB0"),
                                    ap_uint<256>("0xE1031BE262C7ED1B1DC9227A4A04C017A77F8D4464F3B3852C8ACDE6E534FD2D"),
                                    ap_uint<256>("0xFEEA6CAE46D55B530AC2839F143BD7EC5CF8B266A41D6AF52D5E688D9094696D"),
                                    ap_uint<256>("0xDA67A91D91049CDCB367BE4BE6FFCA3CFEED657D808583DE33FA978BC1EC6CB1"),
                                    ap_uint<256>("0x53904FAA0B334CDDA6E000935EF22151EC08D0F7BB11069F57545CCC1A37B7C0"),
                                    ap_uint<256>("0x8E7BCD0BD35983A7719CCA7764CA906779B53A043A9B8BCAEFF959F43AD86047"),
                                    ap_uint<256>("0x385EED34C1CDFF21E6D0818689B81BDE71A7F4F18397E6690A841E1599C43862"),
                                    ap_uint<256>("0x06F9D9B803ECF191637C73A4413DFA180FDDF84A5947FBC9C606ED86C3FAC3A7"),
                                    ap_uint<256>("0x3322D401243C4E2582A2147C104D6ECBF774D163DB0F5E5313B7E0E742D0E6BD"),
                                    ap_uint<256>("0x85672C7D2DE0B7DA2BD1770D89665868741B3F9AF7643397721D74D28134AB83"),
                                    ap_uint<256>("0x0948BF809B1988A46B06C9F1919413B10F9226C60F668832FFD959AF60C82A0A"),
                                    ap_uint<256>("0x6260CE7F461801C34F067CE0F02873A8F1B0E44DFC69752ACCECD819F38FD8E8"),
                                    ap_uint<256>("0xE5037DE0AFC1D8D43D8348414BBF4103043EC8F575BFDC432953CC8D2037FA2D"),
                                    ap_uint<256>("0xE06372B0F4A207ADF5EA905E8F1771B4E7E8DBD1C6A6C5B725866A0AE4FCE725"),
                                    ap_uint<256>("0x213C7A715CD5D45358D0BBF9DC0CE02204B10BDDE2A3F58540AD6908D0559754"),
                                    ap_uint<256>("0x4E7C272A7AF4B34E8DBB9352A5419A87E2838C70ADC62CDDF0CC3A3B08FBD53C"),
                                    ap_uint<256>("0xFEA74E3DBE778B1B10F238AD61686AA5C76E3DB2BE43057632427E2840FB27B6"),
                                    ap_uint<256>("0x76E64113F677CF0E10A2570D599968D31544E179B760432952C02A4417BDDE39"),
                                    ap_uint<256>("0xC738C56B03B2ABE1E8281BAA743F8F9A8F7CC643DF26CBEE3AB150242BCBB891"),
                                    ap_uint<256>("0xD895626548B65B81E264C7637C972877D1D72E5F3A925014372E9F6588F6C14B"),
                                    ap_uint<256>("0xB8DA94032A957518EB0F6433571E8761CEFFC73693E84EDD49150A564F676E03"),
                                    ap_uint<256>("0xE80FEA14441FB33A7D8ADAB9475D7FAB2019EFFB5156A792F1A11778E3C0DF5D"),
                                    ap_uint<256>("0xA301697BDFCD704313BA48E51D567543F2A182031EFD6915DDC07BBCC4E16070"),
                                    ap_uint<256>("0x90AD85B389D6B936463F9D0512678DE208CC330B11307FFFAB7AC63E3FB04ED4"),
                                    ap_uint<256>("0x8F68B9D2F63B5F339239C1AD981F162EE88C5678723EA3351B7B444C9EC4C0DA"),
                                    ap_uint<256>("0xE4F3FB0176AF85D65FF99FF9198C36091F48E86503681E3E6686FD5053231E11"),
                                    ap_uint<256>("0x8C00FA9B18EBF331EB961537A45A4266C7034F2F0D4E1D0716FB6EAE20EAE29E"),
                                    ap_uint<256>("0xE7A26CE69DD4829F3E10CEC0A9E98ED3143D084F308B92C0997FDDFC60CB3E41"),
                                    ap_uint<256>("0xB6459E0EE3662EC8D23540C223BCBDC571CBCB967D79424F3CF29EB3DE6B80EF"),
                                    ap_uint<256>("0xD68A80C8280BB840793234AA118F06231D6F1FC67E73C5A5DEDA0F5B496943E8"),
                                    ap_uint<256>("0x324AED7DF65C804252DC0270907A30B09612AEB973449CEA4095980FC28D3D5D"),
                                    ap_uint<256>("0x4DF9C14919CDE61F6D51DFDBE5FEE5DCEEC4143BA8D1CA888E8BD373FD054C96"),
                                    ap_uint<256>("0x9C3919A84A474870FAED8A9C1CC66021523489054D7F0308CBFC99C8AC1F98CD"),
                                    ap_uint<256>("0x6057170B1DD12FDF8DE05F281D8E06BB91E1493A8B91D4CC5A21382120A959E5"),
                                    ap_uint<256>("0xA576DF8E23A08411421439A4518DA31880CEF0FBA7D4DF12B1A6973EECB94266"),
                                    ap_uint<256>("0x7778A78C28DEC3E30A05FE9629DE8C38BB30D1F5CF9A3A208F763889BE58AD71"),
                                    ap_uint<256>("0x0928955EE637A84463729FD30E7AFD2ED5F96274E5AD7E5CB09EDA9C06D903AC"),
                                    ap_uint<256>("0x85D0FEF3EC6DB109399064F3A0E3B2855645B4A907AD354527AAE75163D82751"),
                                    ap_uint<256>("0xFF2B0DCE97EECE97C1C9B6041798B85DFDFB6D8882DA20308F5404824526087E"),
                                    ap_uint<256>("0x827FBBE4B1E880EA9ED2B2E6301B212B57F1EE148CD6DD28780E5E2CF856E241"),
                                    ap_uint<256>("0xEAA649F21F51BDBAE7BE4AE34CE6E5217A58FDCE7F47F9AA7F3B58FA2120E2B3"),
                                    ap_uint<256>("0xE4A42D43C5CF169D9391DF6DECF42EE541B6D8F0C9A137401E23632DDA34D24F"),
                                    ap_uint<256>("0x1EC80FEF360CBDD954160FADAB352B6B92B53576A88FEA4947173B9D4300BF19"),
                                    ap_uint<256>("0x146A778C04670C2F91B00AF4680DFA8BCE3490717D58BA889DDB5928366642BE"),
                                    ap_uint<256>("0xFA50C0F61D22E5F07E3ACEBB1AA07B128D0012209A28B9776D76A8793180EEF9"),
                                    ap_uint<256>("0xDA1D61D0CA721A11B1A5BF6B7D88E8421A288AB5D5BBA5220E53D32B5F067EC2"),
                                    ap_uint<256>("0xA8E282FF0C9706907215FF98E8FD416615311DE0446F1E062A73B0610D064E13"),
                                    ap_uint<256>("0x174A53B9C9A285872D39E56E6913CAB15D59B1FA512508C022F382DE8319497C"),
                                    ap_uint<256>("0x959396981943785C3D3E57EDF5018CDBE039E730E4918B3D884FDFF09475B7BA"),
                                    ap_uint<256>("0xD2A63A50AE401E56D645A1153B109A8FCCA0A43D561FBA2DBB51340C9D82B151"),
                                    ap_uint<256>("0x64587E2335471EB890EE7896D7CFDC866BACBDBD3839317B3436F9B45617E073"),
                                    ap_uint<256>("0x8481BDE0E4E4D885B3A546D3E549DE042F0AA6CEA250E7FD358D6C86DD45E458"),
                                    ap_uint<256>("0x13464A57A78102AA62B6979AE817F4637FFCFED3C4B1CE30BCD6303F6CAF666B"),
                                    ap_uint<256>("0xBC4A9DF5B713FE2E9AEF430BCC1DC97A0CD9CCEDE2F28588CADA3A0D2D83F366"),
                                    ap_uint<256>("0x8C28A97BF8298BC0D23D8C749452A32E694B65E30A9472A3954AB30FE5324CAA"),
                                    ap_uint<256>("0x08EA9666139527A8C1DD94CE4F071FD23C8B350C5A4BB33748C4BA111FACCAE0")};
    ap_uint<256> preComputeY[64] = {ap_uint<256>("0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"),
                                    ap_uint<256>("0xF7E3507399E595929DB99F34F57937101296891E44D23F0BE1F32CCE69616821"),
                                    ap_uint<256>("0x11F8A8098557DFE45E8256E830B60ACE62D613AC2F7B17BED31B6EAFF6E26CAF"),
                                    ap_uint<256>("0xD3506E0D9E3C79EBA4EF97A51FF71F5EACB5955ADD24345C6EFA6FFEE9FED695"),
                                    ap_uint<256>("0x04E273ADFC732221953B445397F3363145B9A89008199ECB62003C7F3BEE9DE9"),
                                    ap_uint<256>("0x4AAD0A6F68D308B4B3FBD7813AB0DA04F9E336546162EE56B3EFF0C65FD4FD36"),
                                    ap_uint<256>("0x96E867B5595CC498A921137488824D6E2660A0653779494801DC069D9EB39F5F"),
                                    ap_uint<256>("0x5D9A8CA3970EF0F269EE7EDAF178089D9AE4CDC3A711F712DDFD4FDAE1DE8999"),
                                    ap_uint<256>("0xCDD9E13192A00B772EC8F3300C090666B7FF4A18FF5195AC0FBD5CD62BC65A09"),
                                    ap_uint<256>("0x9D7061928940405E6BB6A4176597535AF292DD419E1CED79A44F18F29456A00D"),
                                    ap_uint<256>("0xE57C6B6C97DCE1BAB06E4E12BF3ECD5C981C8957CC41442D3155DEBF18090088"),
                                    ap_uint<256>("0x9BACAA35481642BC41F463F7EC9780E5DEC7ADC508F740A17E9EA8E27A68BE1D"),
                                    ap_uint<256>("0x5BC087D0BC80106D88C9ECCAC20D3C1C13999981E14434699DCB096B022771C8"),
                                    ap_uint<256>("0x10B7770B2A3DA4B3940310420CA9514579E88E2E47FD68B3EA10047E8460372A"),
                                    ap_uint<256>("0x283BEBC3E8EA23F56701DE19E9EBF4576B304EEC2086DC8CC0458FE5542E5453"),
                                    ap_uint<256>("0x7C80C68E603059BA69B8E2A30E45C4D47EA4DD2F5C281002D86890603A842160"),
                                    ap_uint<256>("0x56E70797E9664EF5BFB019BC4DDAF9B72805F63EA2873AF624F3A2E96C28B2A0"),
                                    ap_uint<256>("0x7C481B9B5B43B2EB6374049BFA62C2E5E77F17FCC5298F44C8E3094F790313A6"),
                                    ap_uint<256>("0x53A562856DCB6646DC6B74C5D1C3418C6D4DFF08C97CD2BED4CB7F88D8C8E589"),
                                    ap_uint<256>("0xBC2DA82B6FA5B571A7F09049776A1EF7ECD292238051C198C1A84E95B2B4AE17"),
                                    ap_uint<256>("0x4571534BAA94D3B5F9F98D09FB990BDDBD5F5B03EC481F10E0E5DC841D755BDA"),
                                    ap_uint<256>("0x7A908974BCE18CFE12A27BB2AD5A488CD7484A7787104870B27034F94EEE31DD"),
                                    ap_uint<256>("0x4B6DAD0B5AE462507013AD06245BA190BB4850F5F36A7EEDDFF2C27534B458F2"),
                                    ap_uint<256>("0x17749C766C9D0B18E16FD09F6DEF681B530B9614BFF7DD33E0B3941817DCAAE6"),
                                    ap_uint<256>("0x6E0568DB9B0B13297CF674DECCB6AF93126B596B973F7B77701D3DB7F23CB96F"),
                                    ap_uint<256>("0xC90DDF8DEE4E95CF577066D70681F0D35E2A33D2B56D2032B4B1752D1901AC01"),
                                    ap_uint<256>("0x893FB578951AD2537F718F2EACBFBBBB82314EEF7880CFE917E735D9699A84C3"),
                                    ap_uint<256>("0xFEBFAA38F2BC7EAE728EC60818C340EB03428D632BB067E179363ED75D7D991F"),
                                    ap_uint<256>("0x2804DFA44805A1E4D7C99CC9762808B092CC584D95FF3B511488E4E74EFDF6E7"),
                                    ap_uint<256>("0xEED1DE7F638E00771E89768CA3CA94472D155E80AF322EA9FCB4291B6AC9EC78"),
                                    ap_uint<256>("0x7370F91CFB67E4F5081809FA25D40F9B1735DBF7C0A11A130C0D1A041E177EA1"),
                                    ap_uint<256>("0x0E507A3620A38261AFFDCBD9427222B839AEFABE1582894D991D4D48CB6EF150"),
                                    ap_uint<256>("0x662A9F2DBA063986DE1D90C2B6BE215DBBEA2CFE95510BFDF23CBF79501FFF82"),
                                    ap_uint<256>("0x1E63633AD0EF4F1C1661A6D0EA02B7286CC7E74EC951D1C9822C38576FEB73BC"),
                                    ap_uint<256>("0xEFA47267FEA521A1A9DC343A3736C974C2FADAFA81E36C54E7D2A4C66702414B"),
                                    ap_uint<256>("0x2A758E300FA7984B471B006A1AAFBB18D0A6B2C0420E83E20E8A9421CF2CFD51"),
                                    ap_uint<256>("0x067C876D06F3E06DE1DADF16E5661DB3C4B3AE6D48E35B2FF30BF0B61A71BA45"),
                                    ap_uint<256>("0xDB8BA9FFF4B586D00C4B1F9177B0E28B5B0E7B8F7845295A294C84266B133120"),
                                    ap_uint<256>("0x648A365774B61F2FF130C0C35AEC1F4F19213B0C7E332843967224AF96AB7C84"),
                                    ap_uint<256>("0x035EC51092D8728050974C23A1D85D4B5D506CDC288490192EBAC06CAD10D5D"),
                                    ap_uint<256>("0xDDB84F0F4A4DDD57584F044BF260E641905326F76C64C8E6BE7E5E03D4FC599D"),
                                    ap_uint<256>("0x9A1AF0B26A6A4807ADD9A2DAF71DF262465152BC3EE24C65E899BE932385A2A8"),
                                    ap_uint<256>("0x40A6BF20E76640B2C92B97AFE58CD82C432E10A7F514D9F3EE8BE11AE1B28EC8"),
                                    ap_uint<256>("0x34626D9AB5A5B22FF7098E12F2FF580087B38411FF24AC563B513FC1FD9F43AC"),
                                    ap_uint<256>("0xC25621003D3F42A827B78A13093A95EEAC3D26EFA8A8D83FC5180E935BCD091F"),
                                    ap_uint<256>("0x1F03648413A38C0BE29D496E582CF5663E8751E96877331582C237A24EB1F962"),
                                    ap_uint<256>("0x493D13FEF524BA188AF4C4DC54D07936C7B7ED6FB90E2CEB2C951E01F0C29907"),
                                    ap_uint<256>("0xC60F9C923C727B0B71BEF2C67D1D12687FF7A63186903166D605B68BAEC293EC"),
                                    ap_uint<256>("0xBE3279ED5BBBB03AC69A80F89879AA5A01A6B965F13F7E59D47A5305BA5AD93D"),
                                    ap_uint<256>("0x4D9F92E716D1C73526FC99CCFB8AD34CE886EEDFA8D8E4F13A7F7131DEBA9414"),
                                    ap_uint<256>("0xAEEFE93756B5340D2F3A4958A7ABBF5E0146E77F6295A07B671CDC1CC107CEFD"),
                                    ap_uint<256>("0xB318E0EC3354028ADD669827F9D4B2870AAA971D2F7E5ED1D0B297483D83EFD0"),
                                    ap_uint<256>("0x6B84C6922397EBA9B72CD2872281A68A5E683293A57A213B38CD8D7D3F4F2811"),
                                    ap_uint<256>("0x8157F55A7C99306C79C0766161C91E2966A73899D279B48A655FBA0F1AD836F1"),
                                    ap_uint<256>("0x7F97355B8DB81C09ABFB7F3C5B2515888B679A3E50DD6BD6CEF7C73111F4CC0C"),
                                    ap_uint<256>("0xCCC9DC37ABFC9C1657B4155F2C47F9E6646B3A1D8CB9854383DA13AC079AFA73"),
                                    ap_uint<256>("0x2E7E552888C331DD8BA0386A4B9CD6849C653F64C8709385E9B8ABF87524F2FD"),
                                    ap_uint<256>("0xE82D86FB6443FCB7565AEE58B2948220A70F750AF484CA52D4142174DCF89405"),
                                    ap_uint<256>("0xD99FCDD5BF6902E2AE96DD6447C299A185B90A39133AEAB358299E5E9FAF6589"),
                                    ap_uint<256>("0x38EE7B8CBA5404DD84A25BF39CECB2CA900A79C42B262E556D64B1B59779057E"),
                                    ap_uint<256>("0x69BE159004614580EF7E433453CCB0CA48F300A81D0942E13F495A907F6ECC27"),
                                    ap_uint<256>("0x0D3A81CA6E785C06383937ADF4B798CAA6E8A9FBFA547B16D758D666581F33C1"),
                                    ap_uint<256>("0x40A30463A3305193378FEDF31F7CC0EB7AE784F0451CB9459E71DC73CBEF9482"),
                                    ap_uint<256>("0x620EFABBC8EE2782E24E7C0CFB95C5D735B783BE9CF0F8E955AF34A30E62B945")};

    ecdsaSecp256k1() {
#pragma HLS inline
    }

    ap_uint<256> productMod_p4(ap_uint<256> a, ap_uint<256> b) {
        ap_uint<128> aH = a.range(255, 128);
        ap_uint<128> aL = a.range(127, 0);
        ap_uint<128> bH = b.range(255, 128);
        ap_uint<128> bL = b.range(127, 0);

        ap_uint<256> aLbH = aL * bH;
        ap_uint<256> aHbL = aH * bL;
        ap_uint<512> aHbH = aH * bH;
        ap_uint<256> aLbL = aL * bL;
        ap_uint<512> mid = aLbH + aHbL;

        ap_uint<512> mul = (aHbH << 256) + (mid << 128) + aLbL;
        ap_uint<256> P = ap_uint<256>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
        ap_uint<256> c0 = mul.range(255, 0);
        ap_uint<256> c1 = mul.range(511, 256);
        ap_uint<256> w1 = 0;
        ap_uint<256> w2 = 0;
        ap_uint<256> w3 = 0;
        ap_uint<256> w4 = 0;
        ap_uint<256> w5 = 0;
        ap_uint<256> w6 = 0;

        w1.range(255, 32) = c1.range(223, 0);
        w2.range(255, 9) = c1.range(246, 0);
        w3.range(255, 8) = c1.range(247, 0);
        w4.range(255, 7) = c1.range(248, 0);
        w5.range(255, 6) = c1.range(249, 0);
        w6.range(255, 4) = c1.range(251, 0);

        ap_uint<256> s1 = c1.range(255, 252) + c1.range(255, 250) + c1.range(255, 249) + c1.range(255, 248) +
                          c1.range(255, 247) + c1.range(255, 224);
        ap_uint<256> k11 = (s1 << 2) + (s1 << 1) + s1;
        ap_uint<256> k = (s1 << 32) + (k11 << 7) + (s1 << 6) + (s1 << 4) + s1;

        ap_uint<256> tmp;
        tmp = xf::security::internal::addMod(k, c0, P);
        tmp = xf::security::internal::addMod(tmp, w1, P);
        tmp = xf::security::internal::addMod(tmp, w2, P);
        tmp = xf::security::internal::addMod(tmp, w3, P);
        tmp = xf::security::internal::addMod(tmp, w4, P);
        tmp = xf::security::internal::addMod(tmp, w5, P);
        tmp = xf::security::internal::addMod(tmp, w6, P);
        tmp = xf::security::internal::addMod(tmp, c1, P);

        if (tmp >= P) {
            tmp -= P;
        }

        return tmp;
    }

    ap_uint<256> productMod_p(ap_uint<256> a, ap_uint<256> b) { return productMod_p4(a, b); }

    ap_uint<256> productMod_n(ap_uint<256> a, ap_uint<256> b) {
// XXX: a * b % n is only called a few times, no need to use specialized version
#pragma HLS inline
        return xf::security::internal::productMod<256>(a, b, n);
    }

    ap_uint<256> getNeg(ap_uint<256> y) { return p - y; }

    void getNAF(ap_uint<256> x, ap_uint<256>& np, ap_uint<256>& nm) {
        ap_uint<256> xh = x >> 1;
        ap_uint<256> x3 = x + xh;
        ap_uint<256> c = xh ^ x3;
        np = x3 & c;
        nm = xh & c;
    }

    void toJacobian(ap_uint<256> x, ap_uint<256> y, ap_uint<256>& X, ap_uint<256>& Y, ap_uint<256>& Z) {
#pragma HLS inline
        X = x;
        Y = y;
        Z = 1;
    }

    void fromJacobian(ap_uint<256> X, ap_uint<256> Y, ap_uint<256> Z, ap_uint<256>& x, ap_uint<256>& y) {
#pragma HLS inline
        if (Z == 0) {
            x = 0;
            y = 0;
        } else {
            ap_uint<256> ZInv = xf::security::internal::modularInv<256>(Z, p);
            ap_uint<256> ZInv_2 = productMod_p(ZInv, ZInv);
            ap_uint<256> ZInv_3 = productMod_p(ZInv_2, ZInv);
            x = productMod_p(X, ZInv_2);
            y = productMod_p(Y, ZInv_3);
        }
    }

    void addJacobian(ap_uint<256> X1,
                     ap_uint<256> Y1,
                     ap_uint<256> Z1,
                     ap_uint<256> X2,
                     ap_uint<256> Y2,
                     ap_uint<256> Z2,
                     ap_uint<256>& X3,
                     ap_uint<256>& Y3,
                     ap_uint<256>& Z3) {
#pragma HLS inline
        ap_uint<256> I1 = productMod_p(Z1, Z1);
        ap_uint<256> I2 = productMod_p(Z2, Z2);
        ap_uint<256> J1 = productMod_p(I1, Z1);
        ap_uint<256> J2 = productMod_p(I2, Z2);
        ap_uint<256> U1 = productMod_p(X1, I2);
        ap_uint<256> U2 = productMod_p(X2, I1);
        ap_uint<256> H = xf::security::internal::subMod<256>(U1, U2, p);
        ap_uint<256> F = xf::security::internal::addMod<256>(H, H, p);
        F = productMod_p(F, F);
        ap_uint<256> K1 = productMod_p(Y1, J2);
        ap_uint<256> K2 = productMod_p(Y2, J1);
        ap_uint<256> V = productMod_p(U1, F);
        ap_uint<256> G = productMod_p(F, H);
        ap_uint<256> R = xf::security::internal::subMod<256>(K1, K2, p);
        R = xf::security::internal::addMod<256>(R, R, p);

        if (Z2 == 0) {
            X3 = X1;
            Y3 = Y1;
            Z3 = Z1;
        } else if (Z1 == 0) {
            X3 = X2;
            Y3 = Y2;
            Z3 = Z2;
        } else if (xf::security::internal::addMod<256>(K1, K2, p) == 0) {
            X3 = 1;
            Y3 = 1;
            Z3 = 0;
        } else {
            ap_uint<256> tmpX = productMod_p(R, R);
            ap_uint<256> tmp2V = xf::security::internal::addMod<256>(V, V, p);
            tmpX = xf::security::internal::addMod<256>(tmpX, G, p);
            X3 = xf::security::internal::subMod<256>(tmpX, tmp2V, p);

            ap_uint<256> tmp2 = xf::security::internal::subMod<256>(V, X3, p);
            tmp2 = productMod_p(tmp2, R);
            ap_uint<256> tmp4 = productMod_p(K1, G);
            tmp4 = xf::security::internal::addMod<256>(tmp4, tmp4, p);
            Y3 = xf::security::internal::subMod<256>(tmp2, tmp4, p);

            ap_uint<256> tmp5 = xf::security::internal::addMod<256>(Z1, Z2, p);
            tmp5 = productMod_p(tmp5, tmp5);
            ap_uint<256> tmp6 = xf::security::internal::addMod<256>(I1, I2, p);
            ap_uint<256> tmp7 = xf::security::internal::subMod<256>(tmp5, tmp6, p);
            Z3 = productMod_p(tmp7, H);
        }
    }

    void doubleJacobian(
        ap_uint<256> X1, ap_uint<256> Y1, ap_uint<256> Z1, ap_uint<256>& X2, ap_uint<256>& Y2, ap_uint<256>& Z2) {
#pragma HLS inline
        ap_uint<256> N = productMod_p(Z1, Z1);
        ap_uint<256> E = productMod_p(Y1, Y1);
        ap_uint<256> B = productMod_p(X1, X1);
        ap_uint<256> L = productMod_p(E, E);

        ap_uint<256> tmp1 = xf::security::internal::addMod<256>(X1, E, p);
        tmp1 = productMod_p(tmp1, tmp1);
        ap_uint<256> tmp2 = xf::security::internal::addMod<256>(B, L, p);
        ap_uint<256> tmp3 = xf::security::internal::subMod<256>(tmp1, tmp2, p);
        ap_uint<256> S = xf::security::internal::addMod<256>(tmp3, tmp3, p);

        ap_uint<256> tmp4 = productMod_p(N, N);
        tmp4 = productMod_p(tmp4, a);
        ap_uint<256> tmp5 = xf::security::internal::addMod<256>(B, B, p);
        tmp5 = xf::security::internal::addMod<256>(tmp5, B, p);
        ap_uint<256> M = xf::security::internal::addMod<256>(tmp5, tmp4, p);

        ap_uint<256> tmp6 = xf::security::internal::addMod<256>(S, S, p);
        ap_uint<256> tmp7 = productMod_p(M, M);
        X2 = xf::security::internal::subMod<256>(tmp7, tmp6, p);

        ap_uint<256> tmp8 = xf::security::internal::subMod<256>(S, X2, p);
        tmp8 = productMod_p(tmp8, M);
        ap_uint<256> tmp9 = xf::security::internal::addMod<256>(L, L, p);
        tmp9 = xf::security::internal::addMod<256>(tmp9, tmp9, p);
        tmp9 = xf::security::internal::addMod<256>(tmp9, tmp9, p);
        Y2 = xf::security::internal::subMod<256>(tmp8, tmp9, p);

        ap_uint<256> tmp10 = xf::security::internal::addMod<256>(Y1, Z1, p);
        tmp10 = productMod_p(tmp10, tmp10);
        ap_uint<256> tmp11 = xf::security::internal::addMod<256>(E, N, p);
        Z2 = xf::security::internal::subMod<256>(tmp10, tmp11, p);
    }

    void addJacobianAffine(ap_uint<256> X1,
                           ap_uint<256> Y1,
                           ap_uint<256> Z1,
                           ap_uint<256> x2,
                           ap_uint<256> y2,
                           ap_uint<256>& X3,
                           ap_uint<256>& Y3,
                           ap_uint<256>& Z3) {
        // Only apply to elliptic curve with a=-3.
        if (Z1 == 0) {
            X3 = x2;
            Y3 = y2;
            Z3 = 1;
            return;
        }
#pragma HLS inline
        ap_uint<256> T1 = productMod_p(Z1, Z1);
        ap_uint<256> T2 = productMod_p(T1, Z1);
        T1 = productMod_p(T1, x2);
        T2 = productMod_p(T2, y2);
        T1 = xf::security::internal::subMod(T1, X1, p);
        T2 = xf::security::internal::subMod(T2, Y1, p);

        if (T1 == 0) {
            if (T2 == 0) {
                doubleJacobian(x2, y2, 1, X3, Y3, Z3);
            } else {
                X3 = 1;
                Y3 = 1;
                Z3 = 0;
            }
        } else {
            Z3 = productMod_p(Z1, T1);
            ap_uint<256> T3 = productMod_p(T1, T1);
            ap_uint<256> T4 = productMod_p(T3, T1);
            T3 = productMod_p(T3, X1);
            T1 = productMod_p(T3, ap_uint<256>(2));
            X3 = productMod_p(T2, T2);
            X3 = xf::security::internal::subMod(X3, T1, p);
            X3 = xf::security::internal::subMod(X3, T4, p);
            T3 = xf::security::internal::subMod(T3, X3, p);
            T3 = productMod_p(T3, T2);
            T4 = productMod_p(T4, Y1);
            Y3 = xf::security::internal::subMod(T3, T4, p);
        }
    }

    void dotProductJacobian(ap_uint<256> Px, ap_uint<256> Py, ap_uint<256> k, ap_uint<256>& Rx, ap_uint<256>& Ry) {
#pragma HLS inline
        ap_uint<256> X1, Y1, Z1, X2, Y2, Z2;
        toJacobian(Px, Py, X1, Y1, Z1);

        ap_uint<256> RX = 1;
        ap_uint<256> RY = 1;
        ap_uint<256> RZ = 0;

        for (int i = 256 - 1; i >= 0; i--) {
#pragma HLS pipeline off
            doubleJacobian(RX, RY, RZ, RX, RY, RZ);
            if (k[i] == 1) {
                addJacobianAffine(RX, RY, RZ, Px, Py, RX, RY, RZ);
            }
        }
        X2 = RX;
        Y2 = RY;
        Z2 = RZ;

        fromJacobian(X2, Y2, Z2, Rx, Ry);
    }

    void dotProductNAFPrecomputeJacobian(
        ap_uint<256> Px, ap_uint<256> Py, ap_uint<256> k, ap_uint<256>& Rx, ap_uint<256>& Ry) {
#pragma HLS inline
#pragma HLS bind_storage variable = preComputeX type = ROM_1P impl = LUTRAM
#pragma HLS bind_storage variable = preComputeY type = ROM_1P impl = LUTRAM
        ap_uint<256> kNafPos = 0;
        ap_uint<256> kNafNeg = 0;
        getNAF(k, kNafPos, kNafNeg);

        ap_int<5> K[64];
    COMPUTE_K:
        for (int i = 0; i < 64; i++) {
            ap_uint<4> tmp1 = kNafPos.range(i * 4 + 3, i * 4);
            ap_uint<4> tmp2 = kNafNeg.range(i * 4 + 3, i * 4);
            K[i] = tmp1 - tmp2;
        }

        ap_uint<256> resX = 0;
        ap_uint<256> resY = 0;
        ap_uint<256> resZ = 0;
        ap_uint<256> tmpX = 0;
        ap_uint<256> tmpY = 0;
        ap_uint<256> tmpZ = 0;

    POINT_MULTI_OUTER:
        for (int j = 10; j > 0; j--) {
#pragma HLS pipeline off
        POINT_MULTI_INNER:
            for (int i = 0; i < 64; i++) {
                if (j == K[i]) {
                    addJacobianAffine(tmpX, tmpY, tmpZ, preComputeX[i], preComputeY[i], tmpX, tmpY, tmpZ);
                }
                if (-j == K[i]) {
                    ap_uint<256> tmp = preComputeY[i];
                    if (tmp > p) {
                        tmp = tmp - p;
                    }
                    ap_uint<256> negPy = getNeg(tmp);
                    addJacobianAffine(tmpX, tmpY, tmpZ, preComputeX[i], negPy, tmpX, tmpY, tmpZ);
                }
            }

            addJacobian(tmpX, tmpY, tmpZ, resX, resY, resZ, resX, resY, resZ);
        }
        fromJacobian(resX, resY, resZ, Rx, Ry);
    }

    void add(ap_uint<256> Px, ap_uint<256> Py, ap_uint<256> Qx, ap_uint<256> Qy, ap_uint<256>& Rx, ap_uint<256>& Ry) {
#pragma HLS inline
        if (Qx == 0 && Qy == 0) { // Q is zero
            Rx = Px;
            Ry = Py;
        } else if (Px == Qx && (Py + Qy) == p) {
            Rx = 0;
            Ry = 0;
        } else {
            ap_uint<256> lamda, lamda_d;
            if (Px == Qx && Py == Qy) {
                lamda = productMod_p(Px, Px);
                lamda = productMod_p(lamda, ap_uint<256>(3));
                lamda = xf::security::internal::addMod<256>(lamda, a, p);
                lamda_d = productMod_p(Py, ap_uint<256>(2));
            } else {
                lamda = xf::security::internal::subMod<256>(Qy, Py, p);
                lamda_d = xf::security::internal::subMod<256>(Qx, Px, p);
            }
            lamda_d = xf::security::internal::modularInv<256>(lamda_d, p);
            lamda = productMod_p(lamda, lamda_d);

            ap_uint<256> lamda_sqr = productMod_p(lamda, lamda);

            ap_uint<256> resX, resY;
            resX = xf::security::internal::subMod<256>(lamda_sqr, Px, p);
            resX = xf::security::internal::subMod<256>(resX, Qx, p);

            resY = xf::security::internal::subMod<256>(Px, resX, p);
            resY = productMod_p(lamda, resY);
            resY = xf::security::internal::subMod<256>(resY, Py, p);

            Rx = resX;
            Ry = resY;
        }
    }

    void dotProduct(ap_uint<256> Px, ap_uint<256> Py, ap_uint<256> k, ap_uint<256>& Rx, ap_uint<256>& Ry) {
#pragma HLS inline
        ap_uint<256> resX = 0;
        ap_uint<256> resY = 0;

        for (int i = 0; i < 256; i++) {
            if (k[i] == 1) {
                add(Px, Py, resX, resY, resX, resY);
            }
            add(Px, Py, Px, Py, Px, Py);
        }

        Rx = resX;
        Ry = resY;
    }

    /**
     * @brief Setup parameters for curve y^2 = x^3 + ax + b in GF(p)
     */
    void init() {
        a = ap_uint<256>("0x0");
        b = ap_uint<256>("0x7");
        p = ap_uint<256>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
        Gx = ap_uint<256>("0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798");
        Gy = ap_uint<256>("0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8");
        n = ap_uint<256>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
    };

    /**
     * @brief Generate Public Key point Q from private key
     *
     * @param privateKey Private Key.
     * @param Qx X coordinate of point Q.
     * @param Qy Y coordinate of point Q.
     */
    void generatePubKey(ap_uint<256> privateKey, ap_uint<256>& Qx, ap_uint<256>& Qy) {
        this->dotProduct(Gx, Gy, privateKey, Qx, Qy);
    }

    /**
     * @brief Verifying Public Key.
     * It will return true if verified, otherwise false.
     *
     * @param Px X coordinate of public key point P.
     * @param Py Y coordinate of public key point P.
     */
    bool verifyPubKey(ap_uint<256> Px, ap_uint<256> Py) {
        if (Px == 0 && Py == 0) {
            return false; // return false if public key is zero.
        } else {
            ap_uint<256> tx1 = productMod_p(Px, Px);
            tx1 = productMod_p(tx1, Px);

            ap_uint<256> tx2 = productMod_p(Px, a);
            tx2 = xf::security::internal::addMod<256>(tx2, b, p);

            ap_uint<256> tx3 = xf::security::internal::addMod<256>(tx2, tx1, p);

            ap_uint<256> ty = productMod_p(Py, Py);

            if (ty != tx3) { // return false if public key is not on the curve.
                return false;
            } else {
                ap_uint<256> nPx, nPy;
                dotProductJacobian(Px, Py, n, nPx, nPy);

                if (nPx != 0 || nPy != 0) { // return false if public key * n is not zero.
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * @brief signing function.
     * It will return true if input parameters are legal, otherwise return false.
     *
     * @param hash Digest value of message to be signed.
     * @param k A random key to sign the message, should kept different each time to be used.
     * @param privateKey Private Key to sign the message
     * @param r part of signing pair {r, s}
     * @param s part of signing pair {r, s}
     */
    bool sign(ap_uint<HashW> hash, ap_uint<256> k, ap_uint<256> privateKey, ap_uint<256>& r, ap_uint<256>& s) {
#pragma HLS allocation function instances = productMod_p limit = 1
        ap_uint<256> x, y;
        this->dotProductNAFPrecomputeJacobian(Gx, Gy, k, x, y);

        if (x >= n) {
            x -= n;
        } // x = x mod n

        if (x == 0) {
            return false;
        } else {
            r = x;

            ap_uint<256> z;
            if (HashW >= 256) {
                z = hash.range(HashW - 1, HashW - 256);
            } else {
                z = hash;
            }
            if (z >= n) {
                z -= n;
            }

            if (privateKey >= n) {
                privateKey -= n;
            }

            ap_uint<256> kInv = xf::security::internal::modularInv<256>(k, n);
            ap_uint<256> rda = productMod_n(x, privateKey);
            rda = xf::security::internal::addMod<256>(rda, z, n);

            s = productMod_n(kInv, rda);

            if (s == 0) {
                return false;
            } else {
                return true;
            }
        }
    }

    /**
     * @brief verifying function.
     * It will return true if verified, otherwise false.
     *
     * @param r part of signing pair {r, s}
     * @param s part of signing pair {r, s}
     * @param hash Digest value of message to be signed.
     * @param Px X coordinate of public key point P.
     * @param Py Y coordinate of public key point P.
     */
    bool verify(ap_uint<256> r, ap_uint<256> s, ap_uint<HashW> hash, ap_uint<256> Px, ap_uint<256> Py) {
#pragma HLS allocation function instances = productMod_p limit = 1
        if (r == 0 || r >= n || s == 0 || s >= n) {
            return false;
        } else {
            ap_uint<256> z;
            if (HashW >= 256) {
                z = hash.range(HashW - 1, HashW - 256);
            } else {
                z = hash;
            }
            if (z >= n) {
                z -= n;
            }

            ap_uint<256> sInv = xf::security::internal::modularInv<256>(s, n);

            ap_uint<256> u1 = productMod_n(sInv, z);
            ap_uint<256> u2 = productMod_n(sInv, r);

            ap_uint<256> t1x, t1y, t2x, t2y;
            this->dotProductNAFPrecomputeJacobian(Gx, Gy, u1, t1x, t1y);
            this->dotProductJacobian(Px, Py, u2, t2x, t2y);

            ap_uint<256> x, y;
            this->add(t1x, t1y, t2x, t2y, x, y);

            if (x == 0 && y == 0) {
                return false;
            } else {
                if (r == x) {
                    return true;
                } else {
                    return false;
                }
            }
        }
    }
};

} // namespace security
} // namespace xf

#endif
