#include "../fielddefines.h"
#if defined(ENABLE_FIELD_BYTES_INT_2)
#include "clmul_common_impl.h"
#include "../int_utils.h"
#include "../lintrans.h"
#include "../sketch_impl.h"
#endif
#include "../sketch.h"
namespace {
#ifdef ENABLE_FIELD_INT_9
typedef RecLinTrans<uint16_t, 5, 4> StatTableTRI9;
constexpr StatTableTRI9 SQR_TABLE_TRI9({0x1, 0x4, 0x10, 0x40, 0x100, 0x6, 0x18, 0x60, 0x180});
constexpr StatTableTRI9 SQR2_TABLE_TRI9({0x1, 0x10, 0x100, 0x18, 0x180, 0x14, 0x140, 0x1e, 0x1e0});
constexpr StatTableTRI9 SQR4_TABLE_TRI9({0x1, 0x180, 0x1e0, 0x198, 0x1fe, 0x80, 0xa0, 0x88, 0xaa});
constexpr StatTableTRI9 QRT_TABLE_TRI9({0, 0x4e, 0x4c, 0x1aa, 0x48, 0x22, 0x1a2, 0x100, 0x58});
typedef FieldTri<uint16_t, 9, 1, StatTableTRI9, &SQR_TABLE_TRI9, &SQR2_TABLE_TRI9, &SQR4_TABLE_TRI9, &QRT_TABLE_TRI9, &QRT_TABLE_TRI9, &QRT_TABLE_TRI9, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri9;
#endif
#ifdef ENABLE_FIELD_INT_10
typedef RecLinTrans<uint16_t, 5, 5> StatTable10;
constexpr StatTable10 SQR_TABLE_10({0x1, 0x4, 0x10, 0x40, 0x100, 0x9, 0x24, 0x90, 0x240, 0x112});
constexpr StatTable10 SQR2_TABLE_10({0x1, 0x10, 0x100, 0x24, 0x240, 0x41, 0x19, 0x190, 0x136, 0x344});
constexpr StatTable10 SQR4_TABLE_10({0x1, 0x240, 0x136, 0x141, 0x35d, 0x18, 0x265, 0x2e6, 0x227, 0x36b});
constexpr StatTable10 QRT_TABLE_10({0xec, 0x86, 0x84, 0x30e, 0x80, 0x3c2, 0x306, 0, 0x90, 0x296});
typedef Field<uint16_t, 10, 9, StatTable10, &SQR_TABLE_10, &SQR2_TABLE_10, &SQR4_TABLE_10, &QRT_TABLE_10, &QRT_TABLE_10, &QRT_TABLE_10, IdTrans, &ID_TRANS, &ID_TRANS> Field10;
typedef FieldTri<uint16_t, 10, 3, RecLinTrans<uint16_t, 5, 5>, &SQR_TABLE_10, &SQR2_TABLE_10, &SQR4_TABLE_10, &QRT_TABLE_10, &QRT_TABLE_10, &QRT_TABLE_10, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri10;
#endif
#ifdef ENABLE_FIELD_INT_11
typedef RecLinTrans<uint16_t, 6, 5> StatTable11;
constexpr StatTable11 SQR_TABLE_11({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0xa, 0x28, 0xa0, 0x280, 0x205});
constexpr StatTable11 SQR2_TABLE_11({0x1, 0x10, 0x100, 0xa, 0xa0, 0x205, 0x44, 0x440, 0x428, 0x2a8, 0x291});
constexpr StatTable11 SQR4_TABLE_11({0x1, 0xa0, 0x428, 0x1a, 0x645, 0x3a9, 0x144, 0x2d5, 0x9e, 0x4e7, 0x649});
constexpr StatTable11 QRT_TABLE_11({0x734, 0x48, 0x4a, 0x1de, 0x4e, 0x35e, 0x1d6, 0x200, 0x5e, 0, 0x37e});
typedef Field<uint16_t, 11, 5, StatTable11, &SQR_TABLE_11, &SQR2_TABLE_11, &SQR4_TABLE_11, nullptr, nullptr, &QRT_TABLE_11, IdTrans, &ID_TRANS, &ID_TRANS> Field11;
typedef FieldTri<uint16_t, 11, 2, RecLinTrans<uint16_t, 6, 5>, &SQR_TABLE_11, &SQR2_TABLE_11, &SQR4_TABLE_11, &QRT_TABLE_11, &QRT_TABLE_11, &QRT_TABLE_11, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri11;
#endif
#ifdef ENABLE_FIELD_INT_12
typedef RecLinTrans<uint16_t, 6, 6> StatTable12;
constexpr StatTable12 SQR_TABLE_12({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x9, 0x24, 0x90, 0x240, 0x900, 0x412});
constexpr StatTable12 SQR2_TABLE_12({0x1, 0x10, 0x100, 0x9, 0x90, 0x900, 0x41, 0x410, 0x124, 0x249, 0x482, 0x804});
constexpr StatTable12 SQR4_TABLE_12({0x1, 0x90, 0x124, 0x8, 0x480, 0x920, 0x40, 0x412, 0x924, 0x200, 0x82, 0x904});
constexpr StatTable12 QRT_TABLE_12({0x48, 0xc10, 0xc12, 0x208, 0xc16, 0xd82, 0x200, 0x110, 0xc06, 0, 0xda2, 0x5a4});
typedef Field<uint16_t, 12, 9, StatTable12, &SQR_TABLE_12, &SQR2_TABLE_12, &SQR4_TABLE_12, &QRT_TABLE_12, &QRT_TABLE_12, &QRT_TABLE_12, IdTrans, &ID_TRANS, &ID_TRANS> Field12;
typedef FieldTri<uint16_t, 12, 3, RecLinTrans<uint16_t, 6, 6>, &SQR_TABLE_12, &SQR2_TABLE_12, &SQR4_TABLE_12, &QRT_TABLE_12, &QRT_TABLE_12, &QRT_TABLE_12, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri12;
#endif
#ifdef ENABLE_FIELD_INT_13
typedef RecLinTrans<uint16_t, 5, 4, 4> StatTable13;
constexpr StatTable13 SQR_TABLE_13({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x36, 0xd8, 0x360, 0xd80, 0x161b, 0x185a});
constexpr StatTable13 SQR2_TABLE_13({0x1, 0x10, 0x100, 0x1000, 0xd8, 0xd80, 0x185a, 0x514, 0x1176, 0x17b8, 0x1b75, 0x17ff, 0x1f05});
constexpr StatTable13 SQR4_TABLE_13({0x1, 0xd8, 0x1176, 0x1f05, 0xd96, 0x18e8, 0x68, 0xbdb, 0x1a61, 0x1af2, 0x1a37, 0x3b9, 0x1440});
constexpr StatTable13 QRT_TABLE_13({0xcfc, 0x1500, 0x1502, 0x382, 0x1506, 0x149c, 0x38a, 0x118, 0x1516, 0, 0x14bc, 0x100e, 0x3ca});
typedef Field<uint16_t, 13, 27, StatTable13, &SQR_TABLE_13, &SQR2_TABLE_13, &SQR4_TABLE_13, &QRT_TABLE_13, &QRT_TABLE_13, &QRT_TABLE_13, IdTrans, &ID_TRANS, &ID_TRANS> Field13;
#endif
#ifdef ENABLE_FIELD_INT_14
typedef RecLinTrans<uint16_t, 5, 5, 4> StatTable14;
constexpr StatTable14 SQR_TABLE_14({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x21, 0x84, 0x210, 0x840, 0x2100, 0x442, 0x1108});
constexpr StatTable14 SQR2_TABLE_14({0x1, 0x10, 0x100, 0x1000, 0x84, 0x840, 0x442, 0x401, 0x31, 0x310, 0x3100, 0x118c, 0x1844, 0x486});
constexpr StatTable14 SQR4_TABLE_14({0x1, 0x84, 0x31, 0x1844, 0x501, 0x15ce, 0x3552, 0x3101, 0x8c5, 0x3a5, 0x1cf3, 0xd74, 0xc8a, 0x3411});
constexpr StatTable14 QRT_TABLE_14({0x13f2, 0x206, 0x204, 0x3e06, 0x200, 0x1266, 0x3e0e, 0x114, 0x210, 0, 0x1246, 0x2848, 0x3e4e, 0x2258});
typedef Field<uint16_t, 14, 33, StatTable14, &SQR_TABLE_14, &SQR2_TABLE_14, &SQR4_TABLE_14, &QRT_TABLE_14, &QRT_TABLE_14, &QRT_TABLE_14, IdTrans, &ID_TRANS, &ID_TRANS> Field14;
typedef FieldTri<uint16_t, 14, 5, RecLinTrans<uint16_t, 5, 5, 4>, &SQR_TABLE_14, &SQR2_TABLE_14, &SQR4_TABLE_14, &QRT_TABLE_14, &QRT_TABLE_14, &QRT_TABLE_14, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri14;
#endif
#ifdef ENABLE_FIELD_INT_15
typedef RecLinTrans<uint16_t, 5, 5, 5> StatTableTRI15;
constexpr StatTableTRI15 SQR_TABLE_TRI15({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x6, 0x18, 0x60, 0x180, 0x600, 0x1800, 0x6000});
constexpr StatTableTRI15 SQR2_TABLE_TRI15({0x1, 0x10, 0x100, 0x1000, 0x6, 0x60, 0x600, 0x6000, 0x14, 0x140, 0x1400, 0x4006, 0x78, 0x780, 0x7800});
constexpr StatTableTRI15 SQR4_TABLE_TRI15({0x1, 0x6, 0x14, 0x78, 0x110, 0x660, 0x1540, 0x7f80, 0x106, 0x614, 0x1478, 0x7910, 0x1666, 0x7554, 0x3ffe});
constexpr StatTableTRI15 QRT_TABLE_TRI15({0, 0x114, 0x116, 0x428, 0x112, 0x137a, 0x420, 0x6d62, 0x102, 0x73a, 0x135a, 0x6460, 0x460, 0x4000, 0x6de2});
typedef FieldTri<uint16_t, 15, 1, StatTableTRI15, &SQR_TABLE_TRI15, &SQR2_TABLE_TRI15, &SQR4_TABLE_TRI15, &QRT_TABLE_TRI15, &QRT_TABLE_TRI15, &QRT_TABLE_TRI15, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri15;
#endif
#ifdef ENABLE_FIELD_INT_16
typedef RecLinTrans<uint16_t, 6, 5, 5> StatTable16;
constexpr StatTable16 SQR_TABLE_16({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x2b, 0xac, 0x2b0, 0xac0, 0x2b00, 0xac00, 0xb056, 0xc10e});
constexpr StatTable16 SQR2_TABLE_16({0x1, 0x10, 0x100, 0x1000, 0x2b, 0x2b0, 0x2b00, 0xb056, 0x445, 0x4450, 0x45ac, 0x5a6c, 0xa647, 0x657e, 0x571a, 0x7127});
constexpr StatTable16 SQR4_TABLE_16({0x1, 0x2b, 0x445, 0xa647, 0x12a1, 0xf69d, 0x7f07, 0x9825, 0x6fad, 0x399d, 0xb515, 0xd7d1, 0x3fb4, 0x4b06, 0xe4df, 0x93c7});
constexpr StatTable16 QRT_TABLE_16({0x732, 0x72b8, 0x72ba, 0x7e96, 0x72be, 0x78b2, 0x7e9e, 0x8cba, 0x72ae, 0xfa24, 0x7892, 0x5892, 0x7ede, 0xbec6, 0x8c3a, 0});
typedef Field<uint16_t, 16, 43, StatTable16, &SQR_TABLE_16, &SQR2_TABLE_16, &SQR4_TABLE_16, &QRT_TABLE_16, &QRT_TABLE_16, &QRT_TABLE_16, IdTrans, &ID_TRANS, &ID_TRANS> Field16;
#endif
}
Sketch* ConstructClMul2Bytes(int bits, int implementation) {
    switch (bits) {
#ifdef ENABLE_FIELD_INT_10
    case 10: return new SketchImpl<Field10>(implementation, 10);
#endif
#ifdef ENABLE_FIELD_INT_11
    case 11: return new SketchImpl<Field11>(implementation, 11);
#endif
#ifdef ENABLE_FIELD_INT_12
    case 12: return new SketchImpl<Field12>(implementation, 12);
#endif
#ifdef ENABLE_FIELD_INT_13
    case 13: return new SketchImpl<Field13>(implementation, 13);
#endif
#ifdef ENABLE_FIELD_INT_14
    case 14: return new SketchImpl<Field14>(implementation, 14);
#endif
#ifdef ENABLE_FIELD_INT_16
    case 16: return new SketchImpl<Field16>(implementation, 16);
#endif
    }
    return nullptr;
}
Sketch* ConstructClMulTri2Bytes(int bits, int implementation) {
    switch (bits) {
#ifdef ENABLE_FIELD_INT_9
    case 9: return new SketchImpl<FieldTri9>(implementation, 9);
#endif
#ifdef ENABLE_FIELD_INT_10
    case 10: return new SketchImpl<FieldTri10>(implementation, 10);
#endif
#ifdef ENABLE_FIELD_INT_11
    case 11: return new SketchImpl<FieldTri11>(implementation, 11);
#endif
#ifdef ENABLE_FIELD_INT_12
    case 12: return new SketchImpl<FieldTri12>(implementation, 12);
#endif
#ifdef ENABLE_FIELD_INT_14
    case 14: return new SketchImpl<FieldTri14>(implementation, 14);
#endif
#ifdef ENABLE_FIELD_INT_15
    case 15: return new SketchImpl<FieldTri15>(implementation, 15);
#endif
    }
    return nullptr;
}
