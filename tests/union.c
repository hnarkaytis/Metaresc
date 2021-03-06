#include <math.h>
#include <check.h>
#include <metaresc.h>
#include <regression.h>

#include <union.h>

#define ASSERT_SAVE_LOAD_ANON_UNION(METHOD, TYPE, VALUE, ...) ({	\
      TYPE x = { .dummy = 0, "", { M_PI }, VALUE };			\
      ASSERT_SAVE_LOAD (METHOD, TYPE, &x, __VA_ARGS__);			\
    })

#define ASSERT_SAVE_LOAD_UNION(METHOD, TYPE, VALUE, ...) ({		\
      TYPE x = { .dummy = 0, { M_PI }, VALUE };				\
      ASSERT_SAVE_LOAD (METHOD, TYPE, &x, __VA_ARGS__);			\
    })

MR_START_TEST (embed_anon_union, "embeded anonymous union") {
  struct_embed_anon_union_t orig = { 0, { { -1 } }, };
  ALL_METHODS (ASSERT_SAVE_LOAD, struct_embed_anon_union_t, &orig);
} END_TEST

MR_START_TEST (anon_union, "anonymous union") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_anon_union_enum_t, UD_FLOAT, STRUCT_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_anon_union_enum_t, UD_INT32, STRUCT_X_CMP);
} END_TEST

MR_START_TEST (named_anon_union, "named anonymous union") {
  ALL_METHODS (ASSERT_SAVE_LOAD_ANON_UNION, struct_named_anon_union_enum_t, UD_FLOAT, STRUCT_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_ANON_UNION, struct_named_anon_union_enum_t, UD_INT32, STRUCT_X_CMP);
} END_TEST

#define STRUCT_XY_X_CMP(TYPE, X, Y, ...) ((X)->xy.x != (Y)->xy.x)

MR_START_TEST (union_enum_float, "union discriminated by enum") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum_t, UD_FLOAT, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum8_t, (int)UD_FLOAT, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum16_t, (int)UD_FLOAT, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum32_t, (int)UD_FLOAT, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum64_t, (int)UD_FLOAT, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_bitfield_t, UD_FLOAT, STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_enum_int32, "union discriminated by enum") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum_t, UD_INT32, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum8_t, (int)UD_INT32, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum16_t, (int)UD_INT32, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum32_t, (int)UD_INT32, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum64_t, (int)UD_INT32, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_bitfield_t, UD_INT32, STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_enum_ptr, "union discriminated by pointer on enum") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum_ptr_t, (enum_discriminator_t[]){ UD_FLOAT }, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_enum_ptr_t, (enum_discriminator_t[]){ UD_INT32 }, STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_int_0, "union discriminated by int") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int8_t, 0, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int16_t, 0, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int32_t, 0, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int64_t, 0, STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_int_1, "union discriminated by int") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int8_t, 1, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int16_t, 1, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int32_t, 1, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_int64_t, 1, STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_uint_0, "union discriminated by unsigned int") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint8_t, 0, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint16_t, 0, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint32_t, 0, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint64_t, 0, STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_uint_1, "union discriminated by unsigned int") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint8_t, 1, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint16_t, 1, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint32_t, 1, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_uint64_t, 1, STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_str, "union discriminated by string") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_string_t, "x", STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_string_t, "y", STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_ca_t, "x", STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_ca_t, "y", STRUCT_XY_X_CMP);
} END_TEST

MR_START_TEST (union_str_ptr, "union discriminated by pointer on string") {
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_string_ptr_t, (string_t[]) { "x" }, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_string_ptr_t, (string_t[]) { "y" }, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_ca_ptr_t, (ca2_t[]){ "x" }, STRUCT_XY_X_CMP);
  ALL_METHODS (ASSERT_SAVE_LOAD_UNION, struct_union_ca_ptr_t, (ca2_t[]){ "y" }, STRUCT_XY_X_CMP);
} END_TEST

MAIN ();
