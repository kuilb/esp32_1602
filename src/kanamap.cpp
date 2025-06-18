#include "kanamap.h"

// 假名映射数组
const int kanaMapSize = 1000;
String kanaMap[kanaMapSize];

// 初始化假名数组
void initKanaMap() {
    // あ行
    kanaMap[354] = "\xb1"; kanaMap[450] = "\xb1";  // あ
    kanaMap[356] = "\xb2"; kanaMap[452] = "\xb2";  // い
    kanaMap[358] = "\xb3"; kanaMap[454] = "\xb3";  // う
    kanaMap[360] = "\xb4"; kanaMap[456] = "\xb4";  // え
    kanaMap[362] = "\xb5"; kanaMap[458] = "\xb5";  // お

    // 小あ行
    kanaMap[353] = "\x7a"; kanaMap[449] = "\x7a";  // ぁ
    kanaMap[355] = "\x7b"; kanaMap[451] = "\x7b";  // ぃ
    kanaMap[357] = "\x7c"; kanaMap[453] = "\x7c";  // ぅ
    kanaMap[359] = "\x7d"; kanaMap[455] = "\x7d";  // ぇ
    kanaMap[361] = "\x7e"; kanaMap[457] = "\x7e";  // ぉ

    // か行
    kanaMap[363] = "\xb6"; kanaMap[459] = "\xb6";  // か
    kanaMap[365] = "\xb7"; kanaMap[461] = "\xb7";  // き
    kanaMap[367] = "\xb8"; kanaMap[463] = "\xb8";  // く
    kanaMap[369] = "\xb9"; kanaMap[465] = "\xb9";  // け
    kanaMap[371] = "\xba"; kanaMap[467] = "\xba";  // こ

    // が行
    kanaMap[364] = "\xb6\xde"; kanaMap[460] = "\xb6\xde";  // が
    kanaMap[366] = "\xb7\xde"; kanaMap[462] = "\xb7\xde";  // ぎ
    kanaMap[368] = "\xb8\xde"; kanaMap[464] = "\xb8\xde";  // ぐ
    kanaMap[370] = "\xb9\xde"; kanaMap[466] = "\xb9\xde";  // げ
    kanaMap[372] = "\xba\xde"; kanaMap[468] = "\xba\xde";  // ご

    // さ行
    kanaMap[373] = "\xbb"; kanaMap[469] = "\xbb";  // さ
    kanaMap[375] = "\xbc"; kanaMap[471] = "\xbc";  // し
    kanaMap[377] = "\xbd"; kanaMap[473] = "\xbd";  // す
    kanaMap[379] = "\xbe"; kanaMap[475] = "\xbe";  // せ
    kanaMap[381] = "\xbf"; kanaMap[477] = "\xbf";  // そ

    // ざ行
    kanaMap[374] = "\xbb\xde"; kanaMap[470] = "\xbb\xde";  // ざ
    kanaMap[376] = "\xbc\xde"; kanaMap[472] = "\xbc\xde";  // じ
    kanaMap[378] = "\xbd\xde"; kanaMap[474] = "\xbd\xde";  // ず
    kanaMap[380] = "\xbe\xde"; kanaMap[476] = "\xbe\xde";  // ぜ
    kanaMap[382] = "\xbf\xde"; kanaMap[478] = "\xbf\xde";  // ぞ

    // た行
    kanaMap[383] = "\xc0"; kanaMap[479] = "\xc0";  // た
    kanaMap[385] = "\xc1"; kanaMap[481] = "\xc1";  // ち
    kanaMap[387] = "\xaf"; kanaMap[483] = "\xaf";  // っ
    kanaMap[388] = "\xc2"; kanaMap[484] = "\xc2";  // つ
    kanaMap[390] = "\xc3"; kanaMap[486] = "\xc3";  // て
    kanaMap[392] = "\xc4"; kanaMap[488] = "\xc4";  // と

    // だ行
    kanaMap[384] = "\xc0\xde"; kanaMap[480] = "\xc0\xde";  // だ
    kanaMap[386] = "\xc1\xde"; kanaMap[482] = "\xc1\xde";  // ぢ
    kanaMap[389] = "\xc2\xde"; kanaMap[485] = "\xc2\xde";  // づ
    kanaMap[391] = "\xc3\xde"; kanaMap[487] = "\xc3\xde";  // で
    kanaMap[393] = "\xc4\xde"; kanaMap[489] = "\xc4\xde";  // ど

    // な行
    kanaMap[394] = "\xc5"; kanaMap[490] = "\xc5";  // な
    kanaMap[395] = "\xc6"; kanaMap[491] = "\xc6";  // に
    kanaMap[396] = "\xc7"; kanaMap[492] = "\xc7";  // ぬ
    kanaMap[397] = "\xc8"; kanaMap[493] = "\xc8";  // ね
    kanaMap[398] = "\xc9"; kanaMap[494] = "\xc9";  // の

    // は行
    kanaMap[399] = "\xca"; kanaMap[495] = "\xca";  // は
    kanaMap[402] = "\xcb"; kanaMap[498] = "\xcb";  // ひ
    kanaMap[405] = "\xcc"; kanaMap[501] = "\xcc";  // ふ
    kanaMap[408] = "\xcd"; kanaMap[504] = "\xcd";  // へ
    kanaMap[411] = "\xce"; kanaMap[507] = "\xce";  // ほ

    // ば行
    kanaMap[400] = "\xca\xde"; kanaMap[496] = "\xca\xde";  // ば
    kanaMap[403] = "\xcb\xde"; kanaMap[499] = "\xcb\xde";  // び
    kanaMap[406] = "\xcc\xde"; kanaMap[502] = "\xcc\xde";  // ぶ
    kanaMap[409] = "\xcd\xde"; kanaMap[505] = "\xcd\xde";  // べ
    kanaMap[412] = "\xce\xde"; kanaMap[508] = "\xce\xde";  // ぼ

    // ぱ行
    kanaMap[401] = "\xca\xdf"; kanaMap[497] = "\xca\xdf";  // ぱ
    kanaMap[404] = "\xcb\xdf"; kanaMap[500] = "\xcb\xdf";  // ぴ
    kanaMap[407] = "\xcc\xdf"; kanaMap[503] = "\xcc\xdf";  // ぷ
    kanaMap[410] = "\xcd\xdf"; kanaMap[506] = "\xcd\xdf";  // ぺ
    kanaMap[413] = "\xce\xdf"; kanaMap[509] = "\xce\xdf";  // ぽ

    // ま行
    kanaMap[414] = "\xcf"; kanaMap[510] = "\xcf";  // ま
    kanaMap[415] = "\xd0"; kanaMap[511] = "\xd0";  // み
    kanaMap[416] = "\xd1"; kanaMap[512] = "\xd1";  // む
    kanaMap[417] = "\xd2"; kanaMap[513] = "\xd2";  // め
    kanaMap[418] = "\xd3"; kanaMap[514] = "\xd3";  // も

    // や行
    kanaMap[420] = "\xd4"; kanaMap[516] = "\xd4";  // や
    kanaMap[422] = "\xd5"; kanaMap[518] = "\xd5";  // ゆ
    kanaMap[424] = "\xd6"; kanaMap[520] = "\xd6";  // よ

    // 小や行
    kanaMap[419] = "\xac"; kanaMap[515] = "\xac";  // ゃ
    kanaMap[421] = "\xad"; kanaMap[517] = "\xad";  // ゅ
    kanaMap[423] = "\xae"; kanaMap[519] = "\xae";  // ょ

    // ら行
    kanaMap[425] = "\xd7"; kanaMap[521] = "\xd7";  // ら
    kanaMap[426] = "\xd8"; kanaMap[522] = "\xd8";  // り
    kanaMap[427] = "\xd9"; kanaMap[523] = "\xd9";  // る
    kanaMap[428] = "\xda"; kanaMap[524] = "\xda";  // れ
    kanaMap[429] = "\xdb"; kanaMap[535] = "\xdb";  // ろ

    // わ行
    kanaMap[431] = "\xdc"; kanaMap[527] = "\xdd";  // わ
    kanaMap[434] = "\xa6"; kanaMap[530] = "\xde";  // を
    kanaMap[435] = "\xdd"; kanaMap[531] = "\xdf";  // ん

    // 标点
    kanaMap[540] = "\xb0"; // ー
    kanaMap[290] = "\xa1"; // 。
    kanaMap[300] = "\xa2"; // 「
    kanaMap[301] = "\xa3"; // 」
    kanaMap[289] = "\xa4"; // 、
    kanaMap[539] = "\xa5"; // ・
}