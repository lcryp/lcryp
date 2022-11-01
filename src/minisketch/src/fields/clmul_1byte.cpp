#include "../fielddefines.h"
#if defined(ENABLE_FIELD_BYTES_INT_1)
#include "clmul_common_impl.h"
#include "../int_utils.h"
#include "../lintrans.h"
#include "../sketch_impl.h"
#endif
#include "../sketch.h"
namespace {
#ifdef ENABLE_FIELD_INT_2
typedef RecLinTrans<uint8_t, 2> StatTableTRI2;
constexpr StatTableTRI2 SQR_TABLE_TRI2({0x1, 0x3});
constexpr StatTableTRI2 QRT_TABLE_TRI2({0x2, 0});
typedef FieldTri<uint8_t, 2, 1, StatTableTRI2, &SQR_TABLE_TRI2, &QRT_TABLE_TRI2, &QRT_TABLE_TRI2, &QRT_TABLE_TRI2, &QRT_TABLE_TRI2, &QRT_TABLE_TRI2, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri2;
#endif
#ifdef ENABLE_FIELD_INT_3
typedef RecLinTrans<uint8_t, 3> StatTableTRI3;
constexpr StatTableTRI3 SQR_TABLE_TRI3({0x1, 0x4, 0x6});
constexpr StatTableTRI3 QRT_TABLE_TRI3({0, 0x4, 0x6});
typedef FieldTri<uint8_t, 3, 1, StatTableTRI3, &SQR_TABLE_TRI3, &QRT_TABLE_TRI3, &QRT_TABLE_TRI3, &QRT_TABLE_TRI3, &QRT_TABLE_TRI3, &QRT_TABLE_TRI3, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri3;
#endif
#ifdef ENABLE_FIELD_INT_4
typedef RecLinTrans<uint8_t, 4> StatTableTRI4;
constexpr StatTableTRI4 SQR_TABLE_TRI4({0x1, 0x4, 0x3, 0xc});
constexpr StatTableTRI4 QRT_TABLE_TRI4({0x6, 0xa, 0x8, 0});
typedef FieldTri<uint8_t, 4, 1, StatTableTRI4, &SQR_TABLE_TRI4, &QRT_TABLE_TRI4, &QRT_TABLE_TRI4, &QRT_TABLE_TRI4, &QRT_TABLE_TRI4, &QRT_TABLE_TRI4, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri4;
#endif
#ifdef ENABLE_FIELD_INT_5
typedef RecLinTrans<uint8_t, 5> StatTable5;
constexpr StatTable5 SQR_TABLE_5({0x1, 0x4, 0x10, 0xa, 0xd});
constexpr StatTable5 SQR2_TABLE_5({0x1, 0x10, 0xd, 0xe, 0x1b});
constexpr StatTable5 QRT_TABLE_5({0x14, 0x8, 0xa, 0, 0xe});
typedef Field<uint8_t, 5, 5, StatTable5, &SQR_TABLE_5, &SQR2_TABLE_5, &QRT_TABLE_5, &QRT_TABLE_5, &QRT_TABLE_5, &QRT_TABLE_5, IdTrans, &ID_TRANS, &ID_TRANS> Field5;
typedef FieldTri<uint8_t, 5, 2, RecLinTrans<uint8_t, 5>, &SQR_TABLE_5, &SQR2_TABLE_5, &QRT_TABLE_5, &QRT_TABLE_5, &QRT_TABLE_5, &QRT_TABLE_5, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri5;
#endif
#ifdef ENABLE_FIELD_INT_6
typedef RecLinTrans<uint8_t, 6> StatTableTRI6;
constexpr StatTableTRI6 SQR_TABLE_TRI6({0x1, 0x4, 0x10, 0x3, 0xc, 0x30});
constexpr StatTableTRI6 SQR2_TABLE_TRI6({0x1, 0x10, 0xc, 0x5, 0x13, 0x3c});
constexpr StatTableTRI6 QRT_TABLE_TRI6({0x3a, 0x26, 0x24, 0x14, 0x20, 0});
typedef FieldTri<uint8_t, 6, 1, StatTableTRI6, &SQR_TABLE_TRI6, &SQR2_TABLE_TRI6, &QRT_TABLE_TRI6, &QRT_TABLE_TRI6, &QRT_TABLE_TRI6, &QRT_TABLE_TRI6, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri6;
#endif
#ifdef ENABLE_FIELD_INT_7
typedef RecLinTrans<uint8_t, 4, 3> StatTableTRI7;
constexpr StatTableTRI7 SQR_TABLE_TRI7({0x1, 0x4, 0x10, 0x40, 0x6, 0x18, 0x60});
constexpr StatTableTRI7 SQR2_TABLE_TRI7({0x1, 0x10, 0x6, 0x60, 0x14, 0x46, 0x78});
constexpr StatTableTRI7 QRT_TABLE_TRI7({0, 0x14, 0x16, 0x72, 0x12, 0x40, 0x7a});
typedef FieldTri<uint8_t, 7, 1, StatTableTRI7, &SQR_TABLE_TRI7, &SQR2_TABLE_TRI7, &QRT_TABLE_TRI7, &QRT_TABLE_TRI7, &QRT_TABLE_TRI7, &QRT_TABLE_TRI7, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri7;
#endif
#ifdef ENABLE_FIELD_INT_8
typedef RecLinTrans<uint8_t, 4, 4> StatTable8;
constexpr StatTable8 SQR_TABLE_8({0x1, 0x4, 0x10, 0x40, 0x1b, 0x6c, 0xab, 0x9a});
constexpr StatTable8 SQR2_TABLE_8({0x1, 0x10, 0x1b, 0xab, 0x5e, 0x97, 0xb3, 0xc5});
constexpr StatTable8 QRT_TABLE_8({0xbc, 0x2a, 0x28, 0x86, 0x2c, 0xde, 0x8e, 0});
typedef Field<uint8_t, 8, 27, StatTable8, &SQR_TABLE_8, &SQR2_TABLE_8, &QRT_TABLE_8, &QRT_TABLE_8, &QRT_TABLE_8, &QRT_TABLE_8, IdTrans, &ID_TRANS, &ID_TRANS> Field8;
#endif
}
Sketch* ConstructClMul1Byte(int bits, int implementation) {
    switch (bits) {
#ifdef ENABLE_FIELD_INT_5
    case 5: return new SketchImpl<Field5>(implementation, 5);
#endif
#ifdef ENABLE_FIELD_INT_8
    case 8: return new SketchImpl<Field8>(implementation, 8);
#endif
    }
    return nullptr;
}
Sketch* ConstructClMulTri1Byte(int bits, int implementation) {
    switch (bits) {
#ifdef ENABLE_FIELD_INT_2
    case 2: return new SketchImpl<FieldTri2>(implementation, 2);
#endif
#ifdef ENABLE_FIELD_INT_3
    case 3: return new SketchImpl<FieldTri3>(implementation, 3);
#endif
#ifdef ENABLE_FIELD_INT_4
    case 4: return new SketchImpl<FieldTri4>(implementation, 4);
#endif
#ifdef ENABLE_FIELD_INT_5
    case 5: return new SketchImpl<FieldTri5>(implementation, 5);
#endif
#ifdef ENABLE_FIELD_INT_6
    case 6: return new SketchImpl<FieldTri6>(implementation, 6);
#endif
#ifdef ENABLE_FIELD_INT_7
    case 7: return new SketchImpl<FieldTri7>(implementation, 7);
#endif
    }
    return nullptr;
}
