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

#ifndef _XF_SECURITY_ECDSA_NISTP256_HPP_
#define _XF_SECURITY_ECDSA_NISTP256_HPP_

#include <ap_int.h>
#include <cmath>
#include "modular.hpp"

namespace xf {
namespace security {
namespace internal {
const ap_uint<256> preComputeX[64] = {
    ap_uint<256>("0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296"),
    ap_uint<256>("0x76A94D138A6B41858B821C629836315FCD28392EFF6CA038A5EB4787E1277C6E"),
    ap_uint<256>("0x34A2D4A3B009165987FFD1528603ED61190D0B710D6A564C2DB2E35F12D0441B"),
    ap_uint<256>("0xE716AED2CF069E4D997789672E6D6BD2508676F2F4FD0A64F077E8DAA245573F"),
    ap_uint<256>("0xA018366F4E91E90D8E5C643340E586B4714AB749C9052A0503E8465C6EADE3C4"),
    ap_uint<256>("0x0EC73885141FE54FFEF6A0B570CD98D530E431C1AAD5FCFE8F7DCECB7D96DFF1"),
    ap_uint<256>("0xF8F5DCCF4C6A93D7A4A54DAAFAA3449AA87A8069875405D43725C5DCE392D805"),
    ap_uint<256>("0x6D28B6BFFD4DAF313F85EAAD8E4D71B91CA631161F99218F984A23176F922DBD"),
    ap_uint<256>("0x7FE36B40AF22AF8921656B32262C71DA1AB919365C65DFB63A5A9E22185A5943"),
    ap_uint<256>("0x6965B6384D7061E685371FE7FF26519E76BBA9DDA2AEA7817AFF4FE1FDDE3445"),
    ap_uint<256>("0x0FBC341C8C669D7632CA9F0D41BC43DC1EFE47B273B95775258FCA6C4D9AEFBD"),
    ap_uint<256>("0x6608C243773C85DCBC666B9BA97323B234A8BFE70A2E3338C6E3197AA3FE67B2"),
    ap_uint<256>("0xD8DE765227B7873763DE93C34D8B561CF73A835FBB8E9C71C2EBAF8017E55104"),
    ap_uint<256>("0x54CCC9415026D73F20A845B72A58E5B18BD27F198542A0BEEEA6BC92071E5C83"),
    ap_uint<256>("0xC5440C597814A47D9F6CC7D1513D7F38E40CD02E32847F01C30FB77122D32936"),
    ap_uint<256>("0x241C567A4227F1C506C79B97A6BADCA61C37101CB8971583BF9F4172B066FD48"),
    ap_uint<256>("0x0FA822BC2811AAA58492592E326E25DE29493BAAAD651F7E90E75CB48E14DB63"),
    ap_uint<256>("0x54BC18D7A99899547DDC6988D7EE1B3F2B481AB443DA43FF68F41305B76A6987"),
    ap_uint<256>("0x1D35C9699761E3F285F248239267756F5194B85279F96B7C60AAFAD170AAE231"),
    ap_uint<256>("0x55D9A959844B5AEF388FF0F7AA02F29ACBF5CA9AA567E0E65572AEA8750E4F5F"),
    ap_uint<256>("0x6E29F959BE28C47FAE5ABCA185755C08346924376F5412C1D4D3D2DE4351964C"),
    ap_uint<256>("0xFF046A9EB2BFEED9C00F2EF0796F458EC141C259A845631128A7D4110CB71280"),
    ap_uint<256>("0xE486C7DFFEABB058C1F9AA2349EE7EFF8B2C7E63CF570CC5C7D0B24CC5852E50"),
    ap_uint<256>("0xF41D7F4BB5E50430CFE08CF8B5E2EE0AB63B9998A43D1FC584B6B8EC2B519178"),
    ap_uint<256>("0x4A5B506612A677A657880B3A18A2E902E9A521B074CA0141A84AA9397512218E"),
    ap_uint<256>("0x2EB3910BDE2AB995012C29DF8BBE0F5032C3B2574328E5F76628D837008E2DF0"),
    ap_uint<256>("0x7EF2EE3C5C792A0C0FEF6335224D9428A7D2C98F6743333EC739A5EA3ECCA7E0"),
    ap_uint<256>("0x0E51416421640AEB57802554EB5FA77AA9CB53529975E04A6701F090EC49E853"),
    ap_uint<256>("0x224A02299EECC99A0634A786F116445779C3BB5B5501D267F0699BF9E2F2B734"),
    ap_uint<256>("0x4A89A61457374B4CCDB5D111B56493FE167D5EE4D1041CE2EA6065AD7789B84D"),
    ap_uint<256>("0xE5E892363A31885CEDC2F995F36D1F90BA82337D0B1FC80D3438C84A72BD05A0"),
    ap_uint<256>("0xE4107E431E221F5071E7474104FA90A45312F0C21FB084DE21579992FAB5C2CF"),
    ap_uint<256>("0x447D739BEEDB5E67FB982FD588C6766EFC35FF7DC297EAC357C84FC9D789BD85"),
    ap_uint<256>("0x9022E314949CCF3E8937542B8CDEC18EA2F8D5618688CE241EBD8BAC137DE736"),
    ap_uint<256>("0x73BAFF0419EDA72389386CF2B5156DEDDC34C10619515EB5741145C144CD3397"),
    ap_uint<256>("0x9CF646B91A4C25BBC974446C2976FB982683BEC78B098CB30E2E5FB31FA4E33C"),
    ap_uint<256>("0xF81F5BE38B8CA534E8A5C7938D18DF9E0D238966F74E1A6B826FADC0523B716D"),
    ap_uint<256>("0x85685474D77E08482397F463E53CDE1D022ECA56C3915C978ED9C7E787354B7A"),
    ap_uint<256>("0x1136B759C12B3B11B319E52D6BF9597D9E3607554615622ADDC6DC1B12378C16"),
    ap_uint<256>("0x0D2BF28BA7C2A51A90F573A82589F18E07E50AB01786DF709C762EF1943E832A"),
    ap_uint<256>("0x8A535F566EC73617F5622DF4373713269E4C35874AFDF43AAEE9C75DF7F82F2A"),
    ap_uint<256>("0xAD6090DFDCB41A019AF7CD273DA8A366B7A3030E99E53D5E9C837F4FE476C81D"),
    ap_uint<256>("0xB6E06C516305BBC4E466557075B929FCDE1ADF89F510A848A52F2C3693C4A205"),
    ap_uint<256>("0x45A511C97F608BF76DBC4189D991ECE618C452B1B42A627F3CD5F4E4A9AA52DF"),
    ap_uint<256>("0x20E118564880C79CB927E3501CF2A53F030B86E3CECAC60728CF1AB99076F57B"),
    ap_uint<256>("0x5C9CC4F8723A027B0318EC7FDFD7DAC93FDD478E694FD54ADD1452E899273A8F"),
    ap_uint<256>("0xBAFFD4EBF8A8B5AB46AC18444F3DCDE3351A3A75556C6C4AE7DD9AB328523EB3"),
    ap_uint<256>("0x06AD2A098FCED766ED22A34D257E4C96479F0B1A76DB3C5F835208D8BA111101"),
    ap_uint<256>("0xA6D39677A78492762736FF8344315FC596439591A3C6B94A6CF20FFB313728BE"),
    ap_uint<256>("0xAC25DA80089CF4E033D4DB5710FF5936FD683B4D0DAB013E6EEF62FF4514C6FD"),
    ap_uint<256>("0x285250EDC3BCFCD9027BBA12176D343A26035672D5B4A81155D4E37A2FD20BAE"),
    ap_uint<256>("0xC472C1062ED0366F818DEBC07EF8890E1FC5147DD0DC267C171F3A2A3E07E4F1"),
    ap_uint<256>("0x55D5398D1666432B557582C99AD534580101FB06A050E62C320F09C3839BB85F"),
    ap_uint<256>("0x884286463CA0872694FEBAB24BFC3383CC268940EFEC85ECC62DBC9C32513926"),
    ap_uint<256>("0xB4958C4E21DE21D47FB60C55EDB925559EAA8520A5ED115E3E0B5D5AD4441FBB"),
    ap_uint<256>("0xD0E0919688E932B64134BF13B346027CD8A7FD0B7D761D04AC24E6436E12E1DF"),
    ap_uint<256>("0x68F6B8542783DFEEEB5B06E70CE08FFEFD75F3FA01876BD86A703F10E895DF07"),
    ap_uint<256>("0x7F9460A23D474B66C3B41D1793B353D6646BD35404739E0DD34DAB1767EA1F34"),
    ap_uint<256>("0x9C39CB60A33D563D2958F49F1B6EE94582F2D7A846967F0DCEC9B4D78E6D8483"),
    ap_uint<256>("0x3D9285261122993897566E7430181E25E2FD65F1B1B173747DDA97C55C38D4E4"),
    ap_uint<256>("0xF50B99B7468810B73A5A7DCF08BD17915C08966A2ECC1E07606304B1A44E8DE3"),
    ap_uint<256>("0x042531823FF5ED0B0D77F4880DA0B0E07400F246C3CDAA859E1994056037CFB4"),
    ap_uint<256>("0x9BBF06DAD9AB5905E05471CE16D5222C89C2CAA39F26267AC0747129885FBD44"),
    ap_uint<256>("0xB12FADF52943DFA54943FCCB3B7893E796357686C10319E31EAD3233444C0448"),
};
const ap_uint<256> preComputeY[64] = {
    ap_uint<256>("0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5"),
    ap_uint<256>("0xA985FE61341F260E6CB0A1B5E11E87208599A0040FC78BAA0E9DDD724B8C5110"),
    ap_uint<256>("0xBEAAED6A53A1E3C22BCA71046E777FC0E7D766B9DEDDD81DB424E7845E93B146"),
    ap_uint<256>("0x353663E694FC72AB5912B06687B9A851D13D0DF2FA07C9B3505FC26B469218D1"),
    ap_uint<256>("0xE2BBEC1714110B167C6CE578349D8369D5F7284E44614F37F45C42026B26E8D0"),
    ap_uint<256>("0xD6224F4E87AE875D91ACC4EF580652511D5264CE87ED78AA9EC841AC7C7B552C"),
    ap_uint<256>("0xE58176CF66D63054389D3E336461327351F3DA64A52143BA026619516CDA02FA"),
    ap_uint<256>("0xAF39D905141DD2FA40FBE1A61CCB4A1C4C24E9F0B84DA29944ABFF02A0EF3CFF"),
    ap_uint<256>("0xE697D45825B636249F09F40407DCA6F174B3D5867B8AF212D50D152C699CA101"),
    ap_uint<256>("0xD1BCDAE39C482511325E496638732DBEF47CE6051A5F99D97CC4389E18855113"),
    ap_uint<256>("0xBD8022632F360E3FE4014B1D4957D3D3950B069E200A9FF1ED3B6EA9D3E71CA0"),
    ap_uint<256>("0xA1A916BEC521C168846F640DA746E03FC22B159F40C543081923FE5CE5B47A28"),
    ap_uint<256>("0x2FD29465BE13F2D15C4C628AD1DCF924C2F73FCE1940DB1B2A02EF80E52E08CD"),
    ap_uint<256>("0x1C433F45B45145323A8F8715DAD2BF22929E0BCC5D8EE496CFD08EF7140916A1"),
    ap_uint<256>("0xD27EE9BA383E1FA72EDC1E1D23ECF4ACD8A6D28631E41E5D9A42FA3747CABFD4"),
    ap_uint<256>("0x40A62D93D4302D4B9363817C043203A48EA87138AEA366057F7D2C6792857B08"),
    ap_uint<256>("0xBFF44AE8F5DBA80D6F4AD4BCB3DF188B34B1A65050FE82F5E41124545F462EE7"),
    ap_uint<256>("0x4B2C8C1211E6EAF37391B851BA73E2FD52EB8ED4BB73B119FE457CD05B9AAE49"),
    ap_uint<256>("0xC7226CB62DF608231D660AB622EBF810EDF58AA4729A66F15867063ACCD6AC71"),
    ap_uint<256>("0x69CB7F9AA5DAD203766D574FBCC8EC524C9810C633AD1B15C858EB76BCA97DB0"),
    ap_uint<256>("0x34565D9F500F32F65052EC6CC184246DEF640C527A0BFB63118824BD563FD88F"),
    ap_uint<256>("0x432F55ACC0953A170A01EDDBDD4CFCC9012B6E6EBD28487A7EC3271F5EC33919"),
    ap_uint<256>("0x51FD75ED5606A12E9EE88A5C9F51E05A694463D63392EBD866BA3CADAECF107D"),
    ap_uint<256>("0xE6A669BEBD9AF8D6F1046DE3FAA82347AB49ACC36919E1F9A7A1665DCA6A3551"),
    ap_uint<256>("0xEB13461CEAC089F1C42604FBE1627D40626DB15419E26D9D0BEADA7A4C4F3840"),
    ap_uint<256>("0x3F29C02337474B3A77D37D348DA4999F9540D120ECF65F490910CC4ED274EAAE"),
    ap_uint<256>("0xAFB6862730ACC011A4F67F51D5E609DB81B21450DFBD3D20302B22DD552AC094"),
    ap_uint<256>("0xCF331CEA65905469278EB4A53A91030DF568394190C9EE36336E3D1376405CB2"),
    ap_uint<256>("0xFA41A8D29B6D22B4149A08AD871D7FFF07B704B673C7AFD0840F585491EC7FDF"),
    ap_uint<256>("0x45B04E87ED480D5C08C2EA979FE677E6A72E280634711999EF9B7D7F018E3EA8"),
    ap_uint<256>("0x77439DE4DA1B87D21301EB01E79B5C3ECA73995B9CD099AEC936DE2BE1A69F5D"),
    ap_uint<256>("0x1E5F11E6CF701C9A3948C668741C2B323C7892DCFF7B2410D028403F2B955C2B"),
    ap_uint<256>("0x2D4825AB834131EEE12E9D953A4AAFF73D349B95A7FAE5000C7E33C972E25B32"),
    ap_uint<256>("0x2FAE5E4F2904A39466D0BB045226CE087F49366C44EA7657F4EF5C0844C42ECC"),
    ap_uint<256>("0x1E97DE634977AC5F6A00D1E8C18F825E779EF92CF826134941EF2A139ADCB8E4"),
    ap_uint<256>("0x37B0624DC1F65A891E408E258B821F319E205827EBC1603219C45E060E0D4563"),
    ap_uint<256>("0xDC7F49329C1F06DF1D56D29D380E0328660758503A878330464002F512632401"),
    ap_uint<256>("0x20B50EB50BF587B6EEBC913B2A6F728706A891DC1FD6FDBD8954402BD16E04C6"),
    ap_uint<256>("0x7DEC0FCF45168FDD09F0ABC60FE9ECB29614AA28E751CCE379F59BD0B3488127"),
    ap_uint<256>("0x0CAC3F4313BD00AC7087A10A94B4E7ED27EC9DB96055144648263AF15B20D37C"),
    ap_uint<256>("0x0455C08468B08BD737E02819085A92BFCDE533864C8C7669C5F9A0AC223094B7"),
    ap_uint<256>("0x77B5D1DD48F4C91EACCD75C2CC5BD8A8393FDC364F3048DF818ABC84EE6705DD"),
    ap_uint<256>("0xBDB4277C21B323F5C5E1F126E79ED73B63F99419A82A9F87537F4D865DB36D05"),
    ap_uint<256>("0x73BE0EC773EA9B6D3FE3337FCB625AD25A919B27D22955CE7B52BD12125EC16C"),
    ap_uint<256>("0xFF67B352AC437EF731BDC3E3FB6DE9979FE0DC9B40E1B71E8583BEDBADA7AFE6"),
    ap_uint<256>("0x84EFE07A1DFED259A827589D3708AF964F003675A11ACAB5ADDACA695CE80D6E"),
    ap_uint<256>("0x0FA93DC5DBC95E83003A8EB3795AC09C24511CCFBF48B6EE1978300CD5ACF387"),
    ap_uint<256>("0x2D5B8B1C58F85E3559A4C01BB9EB62A0DE429D82B70E89AEC1A7BA9D5FB37EF4"),
    ap_uint<256>("0x674F84749B0B881666B8BABD2D27ECDF824A920C2284059BF2BAB833C357F5F4"),
    ap_uint<256>("0xEBC69D985CB44C7B883DA9312A1B338C810983E8243BF37A60B5397705830541"),
    ap_uint<256>("0x7866C086F4AD7EA83F7284AD1046C1716E9B1A2CF1EFF323F9DE455A54E0AB97"),
    ap_uint<256>("0x41820E01945F70E96432AB5A84D667D885F3B7B1E22B5D8A571C683D86B0DAA9"),
    ap_uint<256>("0x576E229049FF8E2D059C6A9E8EBAA72AD90D6A7F1833D9E1F7F631184FED936F"),
    ap_uint<256>("0xF778429C01C2DD5A69660DB365D81207CE0EB122EBDBC15A0EE4282AAFB8A54D"),
    ap_uint<256>("0xFCD6E7BF996640EFE5C041FBC38CB7E12DB9F6132D4638414C7E3C9F4ADD7118"),
    ap_uint<256>("0x069C9A59FE7D1452B4EB60B691DBF867544CC4BD9846B8DF6A6544DB9FD28FBE"),
    ap_uint<256>("0xCBE1FEBA92E40CE6FBC8044DFDA45028CF5293D2F310BF7F90C76F8A78712655"),
    ap_uint<256>("0xD0D516778FB0618769F1F2BF8F076661C6FF815124E34FFC58BEC2CACD9063EC"),
    ap_uint<256>("0xF097BF1EE493E51003D196B7BCA16546BBADE52F3DBE436894B3356F6B2C50FB"),
    ap_uint<256>("0xD32CA0A5D468EAB53FF057035C2869EACC0AB138197ABC7F6FAAEF00C0CCF0D3"),
    ap_uint<256>("0xE2B5061E55F18F0C8789973A2DEDCEBBE975F18D21721E854A3F3BA6DB7F3588"),
    ap_uint<256>("0xC90E8BCCFAE00F7E628CA4D0C8661FACEEA3E4F534EC5DD400B7325ED8775D23"),
    ap_uint<256>("0x1BCC7FA84DE120A36755DAF30A6F47E8C0D4BDDC15036ED2A3447DFA7A1D3E88"),
    ap_uint<256>("0x02E2459599B0F4ACE96F8F6677B2A6A71D05E70332FC4DF296575E76B369DE57"),
};

const ap_uint<256> a = ap_uint<256>("0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC");
const ap_uint<256> b = ap_uint<256>("0x5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B");
const ap_uint<256> p = ap_uint<256>("0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF");
const ap_uint<256> Gx = ap_uint<256>("0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296");
const ap_uint<256> Gy = ap_uint<256>("0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5");
const ap_uint<256> n = ap_uint<256>("0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551");

ap_uint<256> productMod_p_fastReduction(ap_uint<256> opA, ap_uint<256> opB) {
    ap_uint<128> aH = opA.range(255, 128);
    ap_uint<128> aL = opA.range(127, 0);
    ap_uint<128> bH = opB.range(255, 128);
    ap_uint<128> bL = opB.range(127, 0);

    ap_uint<256> aLbH = aL * bH;
    ap_uint<256> aHbL = aH * bL;
    ap_uint<512> aHbH = aH * bH;
    ap_uint<256> aLbL = aL * bL;
    ap_uint<512> mid = aLbH + aHbL;

    ap_uint<512> mul = (aHbH << 256) + (mid << 128) + aLbL;
    ap_uint<256> P = ap_uint<256>("0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF");
    ap_uint<256> s1 = 0;
    ap_uint<256> s2 = 0;
    ap_uint<256> s3 = 0;
    ap_uint<256> s4 = 0;
    ap_uint<256> s5 = 0;
    ap_uint<256> s6 = 0;
    ap_uint<256> s7 = 0;
    ap_uint<256> s8 = 0;
    ap_uint<256> s9 = 0;

    s1 = mul.range(255, 0);
    s2.range(255, 96) = mul.range(511, 352);
    s3.range(223, 96) = mul.range(511, 384);
    s4.range(255, 192) = mul.range(511, 448);
    s4.range(95, 0) = mul.range(351, 256);

    s5.range(255, 224) = mul.range(287, 256);
    s5.range(223, 192) = mul.range(447, 416);
    s5.range(191, 96) = mul.range(511, 416);
    s5.range(95, 0) = mul.range(383, 288);

    s6.range(255, 224) = mul.range(351, 320);
    s6.range(223, 192) = mul.range(287, 256);
    s6.range(95, 0) = mul.range(447, 352);

    s7.range(255, 224) = mul.range(383, 352);
    s7.range(223, 192) = mul.range(319, 288);
    s7.range(127, 0) = mul.range(511, 384);

    s8.range(255, 224) = mul.range(415, 384);
    s8.range(191, 96) = mul.range(351, 256);
    s8.range(95, 0) = mul.range(511, 416);

    s9.range(255, 224) = mul.range(447, 416);
    s9.range(191, 96) = mul.range(383, 288);
    s9.range(63, 0) = mul.range(511, 448);

    ap_uint<256> tmp;
    tmp = xf::security::internal::addMod(s1, s2, P);
    tmp = xf::security::internal::addMod(tmp, s2, P);
    tmp = xf::security::internal::addMod(tmp, s3, P);
    tmp = xf::security::internal::addMod(tmp, s3, P);
    tmp = xf::security::internal::addMod(tmp, s4, P);
    tmp = xf::security::internal::addMod(tmp, s5, P);

    ap_uint<256> tmp1;
    tmp1 = xf::security::internal::addMod(s6, s7, P);
    tmp1 = xf::security::internal::addMod(tmp1, s8, P);
    tmp1 = xf::security::internal::addMod(tmp1, s9, P);

    tmp = xf::security::internal::subMod(tmp, tmp1, P);

    if (tmp >= P) {
        tmp -= P;
    }

    return tmp;
}

ap_uint<256> productMod_p(ap_uint<256> opA, ap_uint<256> opB) {
    return productMod_p_fastReduction(opA, opB);
}

ap_uint<256> productMod_n(ap_uint<256> opA, ap_uint<256> opB) {
    return xf::security::internal::productMod<256>(opA, opB, n);
}

ap_uint<256> getNeg(ap_uint<256> y) {
    return p - y;
}

void getNAF(ap_uint<256> x, ap_uint<256>& np, ap_uint<256>& nm) {
    ap_uint<256> xh = x >> 1;
    ap_uint<256> x3 = x + xh;
    ap_uint<256> c = xh ^ x3;
    np = x3 & c;
    nm = xh & c;
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
    // Only apply to elliptic curve with a=-3.

    if (Y1 == 0) {
        X2 = 1;
        Y2 = 1;
        Z2 = 0;
        return;
    }
#pragma HLS inline
    ap_uint<256> ySq = productMod_p(Y1, Y1);
    ap_uint<256> ySq_2 = xf::security::internal::addMod(ySq, ySq, p);
    ap_uint<256> ySq_4 = xf::security::internal::addMod(ySq_2, ySq_2, p);
    ap_uint<256> yQu_4 = productMod_p(ySq_2, ySq_2);
    ap_uint<256> yQu_8 = xf::security::internal::addMod(yQu_4, yQu_4, p);

    ap_uint<256> S = productMod_p(ySq_4, X1);

    ap_uint<256> zSq = productMod_p(Z1, Z1);
    ap_uint<256> M_op_1 = xf::security::internal::subMod(X1, zSq, p);
    ap_uint<256> M_op_2 = xf::security::internal::addMod(X1, zSq, p);
    ap_uint<256> M_base = productMod_p(M_op_1, M_op_2);
    ap_uint<256> M = xf::security::internal::addMod(M_base, M_base, p);
    M = xf::security::internal::addMod(M, M_base, p);

    ap_uint<256> MSq = productMod_p(M, M);
    ap_uint<256> S_2 = xf::security::internal::addMod(S, S, p);
    X2 = xf::security::internal::subMod(MSq, S_2, p);

    ap_uint<256> tmp = xf::security::internal::subMod(S, X2, p);
    tmp = productMod_p(tmp, M);
    Y2 = xf::security::internal::subMod(tmp, yQu_8, p);

    ap_uint<256> tmp_1 = productMod_p(Y1, Z1);
    Z2 = xf::security::internal::addMod(tmp_1, tmp_1, p);
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

void dotProductJacobianAffine(ap_uint<256> Px, ap_uint<256> Py, ap_uint<256> k, ap_uint<256>& Rx, ap_uint<256>& Ry) {
#pragma HLS inline
    ap_uint<256> X2, Y2, Z2;

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

template <int HashW>
bool sign(ap_uint<HashW> hash, ap_uint<256> k, ap_uint<256> privateKey, ap_uint<256>& r, ap_uint<256>& s) {
#pragma HLS allocation function instances = productMod_p limit = 1
    ap_uint<256> x, y;
    dotProductNAFPrecomputeJacobian(Gx, Gy, k, x, y);

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

template <int HashW>
bool verify(ap_uint<256> r, ap_uint<256> s, ap_uint<HashW> hash, ap_uint<256> Px, ap_uint<256> Py) {
#pragma HLS allocation function instances = productMod_p limit = 1
    // Call verifyPubKey() to verify public key if needed
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
        dotProductNAFPrecomputeJacobian(Gx, Gy, u1, t1x, t1y);
        dotProductJacobianAffine(Px, Py, u2, t2x, t2y);

        ap_uint<256> x, y;
        add(t1x, t1y, t2x, t2y, x, y);

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

} // namespace internal

/**
 * @brief signing function.
 * It will return true if input parameters are legal, otherwise return false.
 *
 * @param hash digest value of message to be signed, with length set to 256.
 * @param k A random key to sign the message, should kept different each time to be used.
 * @param privateKey Private Key to sign the message
 * @param r part of signing pair {r, s}
 * @param s part of signing pair {r, s}
 */
bool nistp256Sign(ap_uint<256> hash, ap_uint<256> k, ap_uint<256> privateKey, ap_uint<256>& r, ap_uint<256>& s) {
    return internal::sign<256>(hash, k, privateKey, r, s);
}

/**
 * @brief verifying function.
 * It will return true if verified, otherwise false.
 *
 * @param r part of signing pair {r, s}
 * @param s part of signing pair {r, s}
 * @param hash digest value of message to be signed, with length set to 256.
 * @param Px X coordinate of public key point P.
 * @param Py Y coordinate of public key point P.
 */
bool nistp256Verify(ap_uint<256> r, ap_uint<256> s, ap_uint<256> hash, ap_uint<256> Px, ap_uint<256> Py) {
    return internal::verify<256>(r, s, hash, Px, Py);
}

} // namespace security
} // namespace xf

#endif
