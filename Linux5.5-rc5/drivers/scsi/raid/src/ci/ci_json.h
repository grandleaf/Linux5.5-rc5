/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_json.h												JSON
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_list.h"

#define CI_JDT_BIT					4		/* use 4 bits to indicate type */

enum {
	CI_JDT_OBJ,			/* object */
	CI_JDT_ARY,			/* array */
	
	CI_JDT_NULL,		/* json data type null */
	CI_JDT_PTR,			/* void * */
	
	CI_JDT_STR,			/* string */
	CI_JDT_BIN,			/* binary string */
	CI_JDT_BMP,			/* bitmap */

	CI_JDT_NUM_START,	/* number start */
	CI_JDT_U8  = CI_JDT_NUM_START,
	CI_JDT_U16,
	CI_JDT_U32,
	CI_JDT_U64,

	CI_JDT_SNUM_START,	/* signed number start */
	CI_JDT_INT = CI_JDT_SNUM_START,		/* integer */
	CI_JDT_S8,
	CI_JDT_S16, 
	CI_JDT_S32,			/* a dup of CI_JDT_INT */
	CI_JDT_S64,
	CI_JDT_NUM_END = CI_JDT_S64,

	CI_NR_JDT			/* <= 1 << CI_JDT_BIT */
};
ci_static_assert(CI_NR_JDT <= (1 << CI_JDT_BIT));



/* json object, a container, includes json data */
typedef struct {
	ci_list_t						 head;		/* chain children json data */
} ci_json_obj_t;


/* json array */
typedef struct {
	ci_list_t						 head;		/* chain children json data */
} ci_json_ary_t;


/* json value for json data */
typedef union {
	ci_json_obj_t					 obj;
	ci_json_ary_t					 ary;

	void							*ptr;			/* for void *, bin/str/bmp, the later needs extra storage */
	u64								 ui64;			/* for int, u8~u64, s8~s64 */
} ci_json_val_t;


/* json data */
typedef struct __ci_json_data_t {
	char							*name;
	int								 flag;
#define CI_JDF_DUMP_ENUM				0x0001					/* invoke enum dumper */
#define CI_JDF_DUMP_FLAG				0x0002					/* invoke flag dumper */
#define CI_JDF_DUMP_CUST				0x0004					/* invoke customized dumper */
	
	ci_json_val_t					 val;
	
	u32								 type : CI_JDT_BIT;			/* CI_JDT_* */
	u32								 size : 32 - CI_JDT_BIT;	/* variable for BIN/STR/BMP's size */

	union {
		ci_int_to_name_t			*int_to_name;				/* use this when CI_JDF_DUMP_ENUM/FLAG are set */
		int (*dumper)(struct __ci_json_data_t *);				/* use this when CI_JDF_DUMP_CUST is set */
	};

	struct __ci_json_data_t			*parent;
	struct __ci_json_t				*json;
	ci_list_t						 link;						/* belonged to obj/ary */
} ci_json_data_t;


/* json */
typedef struct __ci_json_t {
	int								 flag;
#define CI_JSON_SORT					0x0001			/* sort by name */
#define CI_JSON_DUMP_DATA_TYPE			0x0002			/* dump the data type */
#define CI_JSON_DUMP_BIG_BIN			0x0004			/* dump big binary buffer (> CI_PR_JSON_BIN_LEN) */
#define CI_JSON_CLI						0x0008			/* command from cli */

	int								 max_name_len;

	void							*cookie;			/* for user */
	ci_json_data_t					*root;				/* last: mandatory root json object with name = "root" */
	ci_balloc_t						*ba;				/* pointer to the allocator */
} ci_json_t;



/* 	most frequently used shortcuts */
#define ci_json_create(name)							__ci_json_create(name, &ci_node_info->ba_json)
#define ci_json_destroy(json)							__ci_json_destroy(json)
#define ci_json_del(json, name)							ci_json_obj_del((json)->root, name)

#define ci_json_get(json, ...)							ci_json_obj_get((json)->root, __VA_ARGS__)
#define ci_json_obj_get(...)							ci_va_args_mmaker(__ci_json_obj_get, __VA_ARGS__)
#define ci_json_ary_get(...)							ci_va_args_mmaker(__ci_json_ary_get, __VA_ARGS__)
#define ci_json_ary_get_last(ary, ...)					__ci_json_ary_get_last(ary, __VA_ARGS__)
#define ci_json_ary_get_first(ary, ...)					ci_json_ary_get(ary, 0, __VA_ARGS__)

#define ci_json_set(json, ...)							ci_json_obj_set((json)->root, __VA_ARGS__)
#define ci_json_set_bmp(json, name, bmp, bits)			ci_json_obj_set_bmp((json)->root, name, bmp, bits)	/* size in bits */
#define ci_json_set_enum(json, name, val, i2n)			ci_json_obj_set_enum((json)->root, name, val, i2n)
#define ci_json_set_flag(json, name, val, i2n)			ci_json_obj_set_flag((json)->root, name, val, i2n)
#define ci_json_set_obj(json, name)						ci_json_obj_set_obj((json)->root, name)
#define ci_json_set_ary(json, name)						ci_json_obj_set_ary((json)->root, name)


/* most frequently used ci_json_obj_* */
#define ci_json_obj_set(...)							ci_va_args_mmaker(__ci_json_obj_set, __VA_ARGS__)
#define ci_json_obj_set_bmp(obj, name, bmp, bits)		__ci_json_obj_set_bmp(obj, name, bmp, bits)
#define ci_json_obj_set_enum(obj, name, val, i2n)		__ci_json_obj_set_enum(obj, name, val, i2n)	
#define ci_json_obj_set_flag(obj, name, val, i2n)		__ci_json_obj_set_flag(obj, name, val, i2n)
#define ci_json_obj_set_obj(obj, name)					__ci_json_obj_set(obj, name, CI_JDT_OBJ, ci_sizeof(ci_json_obj_t), 0)
#define ci_json_obj_set_ary(obj, name)					__ci_json_obj_set(obj, name, CI_JDT_ARY, ci_sizeof(ci_json_ary_t), 0)

/* most frequently used ci_json_ary_* */
#define ci_json_ary_add(...)							ci_json_ary_insert(__VA_ARGS__, -1)
#define ci_json_ary_add_bmp(ary, bmp, bits)				ci_json_ary_insert_bmp(ary, bmp, bits, -1)
#define ci_json_ary_add_enum(ary, val, i2n)				ci_json_ary_insert_enum(ary, val, i2n, -1)
#define ci_json_ary_add_flag(ary, val, i2n)				ci_json_ary_insert_flag(ary, val, i2n, -1)
#define ci_json_ary_add_obj(ary)						ci_json_ary_insert_obj(ary, -1)
#define ci_json_ary_add_ary(ary)						ci_json_ary_insert_ary(ary, -1)

#define ci_json_ary_add_head(...)						ci_json_ary_insert(__VA_ARGS__, 0)
#define ci_json_ary_add_head_bmp(ary, bmp, bits)		ci_json_ary_insert_bmp(ary, bmp, bits, 0)
#define ci_json_ary_add_head_enum(ary, val, i2n)		ci_json_ary_insert_enum(ary, val, i2n, 0)
#define ci_json_ary_add_head_flag(ary, val, i2n)		ci_json_ary_insert_flag(ary, val, i2n, 0)
#define ci_json_ary_add_head_obj(ary)					ci_json_ary_insert_obj(ary, 0)
#define ci_json_ary_add_head_ary(ary)					ci_json_ary_insert_ary(ary, 0)

#define ci_json_ary_insert(...)							ci_va_args_mmaker(__ci_json_ary_insert, __VA_ARGS__)
#define ci_json_ary_insert_bmp(ary, bmp, bits, pos)		__ci_json_ary_insert_bmp(ary, bmp, bits, pos)
#define ci_json_ary_insert_enum(ary, val, i2n, pos)		__ci_json_ary_insert_enum(ary, val, i2n, pos)
#define ci_json_ary_insert_flag(ary, val, i2n, pos)		__ci_json_ary_insert_flag(ary, val, i2n, pos)
#define ci_json_ary_insert_obj(ary, pos)				__ci_json_ary_insert(ary, CI_JDT_OBJ, ci_sizeof(ci_json_obj_t), 0, pos)
#define ci_json_ary_insert_ary(ary, pos)				__ci_json_ary_insert(ary, CI_JDT_ARY, ci_sizeof(ci_json_ary_t), 0, pos)

/* 	dump json balloc */
#define ci_json_balloc_dump()							ci_balloc_dump(&ci_node_info->ba_json)

/* 	iterators */
#define ci_json_obj_each(parent_data, child_data, ...)			\
	do {	\
		ci_json_data_t *child_data;		\
		ci_assert((parent_data)->type == CI_JDT_OBJ);		\
		\
		ci_list_each(&(parent_data)->val.obj.head, child_data, link) {	\
			__VA_ARGS__;	\
		};	\
	} while (0)
#define ci_json_obj_each_safe(parent_data, child_data, ...)			\
	do {	\
		ci_json_data_t *child_data;		\
		ci_assert((parent_data)->type == CI_JDT_OBJ);		\
		\
		ci_list_each_safe(&(parent_data)->val.obj.head, child_data, link) {	\
			__VA_ARGS__;	\
		};	\
	} while (0)	
#define ci_json_ary_each(parent_data, child_data, ...)			\
	do {	\
		ci_json_data_t *child_data;		\
		ci_assert((parent_data)->type == CI_JDT_ARY);		\
		\
		ci_list_each(&(parent_data)->val.obj.head, child_data, link) {	\
			__VA_ARGS__;	\
		};	\
	} while (0)
#define ci_json_ary_each_safe(parent_data, child_data, ...)			\
	do {	\
		ci_json_data_t *child_data;		\
		ci_assert((parent_data)->type == CI_JDT_ARY);		\
		\
		ci_list_each_safe(&(parent_data)->val.obj.head, child_data, link) {	\
			__VA_ARGS__;	\
		};	\
	} while (0)	
#define ci_json_ary_each_with_index(parent_data, child_data, index, ...)			\
	do {	\
		ci_json_data_t *child_data;		\
		ci_assert((parent_data)->type == CI_JDT_ARY);		\
		\
		ci_list_each_with_index(&(parent_data)->val.obj.head, child_data, link, index) {	\
			__VA_ARGS__;	\
		};	\
	} while (0)	

/*
 *	implementation of json obj 
 */
#define __ci_json_obj_set(__obj, __name, __type, __size, __val)	\
	({	\
		ci_json_data_t *__obj_set_data__;		\
		int __name_len__ = ci_strlen(__name);	\
		\
		ci_assert((__obj) && (__name) && (__name_len__ > 0) && ((__size) >= 0));	\
		ci_assert((__size) <= (1 << (32 - CI_JDT_BIT + 1)) - 1);		\
		ci_assert((__obj)->type == CI_JDT_OBJ);		\
		\
		ci_json_obj_del(__obj, __name);	\
		__obj_set_data__ = __ci_json_data_create((__obj)->json->ba, __name, __type, __size, __val);		\
		\
		if (!((__obj)->json->flag & CI_JSON_SORT))		\
			ci_list_add_tail(&(__obj)->val.obj.head, &__obj_set_data__->link);	\
		else	\
			ci_list_add_sort(&(__obj)->val.obj.head, __obj_set_data__, link, __ci_json_data_name_cmp);	\
		\
		__obj_set_data__->parent = (__obj);		\
		__obj_set_data__->json = (__obj)->json;		\
		\
		if (__obj_set_data__->json->max_name_len < __name_len__)	\
			__obj_set_data__->json->max_name_len = __name_len__;	\
		\
		__obj_set_data__;		\
	})

#define ci_json_obj_set_null(obj, name)				__ci_json_obj_set(obj, name, CI_JDT_NULL, 	0, 					0)
#define ci_json_obj_set_int(obj, name, ivar)		__ci_json_obj_set(obj, name, CI_JDT_INT,  	ci_sizeof(int), 	ivar)
#define ci_json_obj_set_ptr(obj, name, ptr)			__ci_json_obj_set(obj, name, CI_JDT_PTR,  	ci_sizeof(ptr), 	ptr)
#define ci_json_obj_set_str(obj, name, str)			__ci_json_obj_set(obj, name, CI_JDT_STR, 	ci_strsize(str), 	str)
#define ci_json_obj_set_u8(obj, name, ui8)			__ci_json_obj_set(obj, name, CI_JDT_U8,  	ci_sizeof(u8), 		ui8)
#define ci_json_obj_set_u16(obj, name, ui16)		__ci_json_obj_set(obj, name, CI_JDT_U16,  	ci_sizeof(u16), 	ui16)
#define ci_json_obj_set_u32(obj, name, ui32)		__ci_json_obj_set(obj, name, CI_JDT_U32,  	ci_sizeof(u32), 	ui32)
#define ci_json_obj_set_u64(obj, name, ui64)		__ci_json_obj_set(obj, name, CI_JDT_U64,  	ci_sizeof(u64), 	ui64)
#define ci_json_obj_set_s8(obj, name, si8)			__ci_json_obj_set(obj, name, CI_JDT_S8,  	ci_sizeof(s8), 		si8)
#define ci_json_obj_set_s16(obj, name, si16)		__ci_json_obj_set(obj, name, CI_JDT_S16,  	ci_sizeof(s16), 	si16)
#define ci_json_obj_set_s32(obj, name, si32)		__ci_json_obj_set(obj, name, CI_JDT_S32,  	ci_sizeof(s32), 	si32)
#define ci_json_obj_set_s64(obj, name, si64)		__ci_json_obj_set(obj, name, CI_JDT_S64,  	ci_sizeof(s64), 	si64)

#define __ci_json_obj_set_bmp(obj, name, bmp, bits)	\
	({	\
		int __size_in_byte__ = ci_bits_to_bmp(bits) * ci_sizeof(ci_bmp_t);		\
		__ci_json_obj_set(obj, name, CI_JDT_BMP, __size_in_byte__, bmp);		\
	})
#define __ci_json_obj_set_enum(obj, name, val, i2n)		\
	({	\
		ci_json_data_t *__json_data_enum__ = ci_json_obj_set_int(obj, name, (int)(val));		\
		__json_data_enum__->flag |= CI_JDF_DUMP_ENUM;	\
		__json_data_enum__->int_to_name = i2n;	\
		__json_data_enum__;	\
	})
#define __ci_json_obj_set_flag(obj, name, val, i2n)		\
	({	\
		ci_json_data_t *__json_data_flag__ = ci_json_obj_set_int(obj, name, (int)(val));		\
		__json_data_flag__->flag |= CI_JDF_DUMP_FLAG;	\
		__json_data_flag__->int_to_name = i2n;	\
		__json_data_flag__;	\
	})

#define __ci_json_obj_set_helper(jd, obj, name, val, type)		\
	else if (ci_type_compatible(ci_typeof(val), type))	\
		jd = ci_json_obj_set_ ## type(obj, name, val)

#define __ci_json_obj_set2(obj, name)					ci_json_obj_set_null(obj, name)
#define __ci_json_obj_set3(obj, name, val)	\
	({	\
		ci_json_data_t *__json_obj_set_data__;		\
		\
		if (ci_type_is_str(val) || ci_type_is_str_ptr(ci_typeof(val))) {	\
			char *__str__ = (char *)(uintptr_t)(val);	\
			__json_obj_set_data__ = ci_json_obj_set_str(obj, name, __str__);	\
		}	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, int);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, u8);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, u16);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, u32);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, u64);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, s8);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, s16);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, s32);	\
		__ci_json_obj_set_helper(__json_obj_set_data__, obj, name, val, s64);	\
		else	\
			__json_obj_set_data__ = ci_json_obj_set_ptr(obj, name, val);	\
		\
		__json_obj_set_data__;	\
	})
#define __ci_json_obj_set4(obj, name, bin, size)		__ci_json_obj_set(obj, name, CI_JDT_BIN, size, bin)		/* set bin */



/*
 *	implementation of json obj 
 */
#define __ci_json_ary_insert(__ary, __type, __size, __val, __pos)	\
	({	\
		ci_json_data_t *__obj_insert_data__;		\
		\
		ci_assert((__ary) && ((__size) >= 0));	\
		ci_assert((__size) <= (1 << (32 - CI_JDT_BIT + 1)) - 1);		\
		ci_assert((__ary)->type == CI_JDT_ARY);		\
		\
		__obj_insert_data__ = __ci_json_data_create((__ary)->json->ba, NULL, __type, __size, __val);		\
		\
		if ((__pos) == -1)	\
			ci_list_add_tail(&(__ary)->val.obj.head, &__obj_insert_data__->link);	\
		else if ((__pos) == 0)	\
			ci_list_add_head(&(__ary)->val.obj.head, &__obj_insert_data__->link);	\
		else {	\
			ci_list_t *__insert_after__= ci_list_at_index(&(__ary)->val.ary.head, (__pos) - 1);		\
			ci_assert((__pos) <= ci_json_ary_nr_child(__ary));	\
			ci_assert(__insert_after__);	\
			ci_list_add_after(__insert_after__, &__obj_insert_data__->link);		\
		}	\
		__obj_insert_data__->parent = (__ary);		\
		__obj_insert_data__->json = (__ary)->json;		\
		\
		__obj_insert_data__;		\
	})

#define ci_json_ary_insert_null(ary, pos)				__ci_json_ary_insert(ary, CI_JDT_NULL, 	0, 					0, 		pos)
#define ci_json_ary_insert_int(ary, ivar, pos)			__ci_json_ary_insert(ary, CI_JDT_INT,  	ci_sizeof(int), 	ivar, 	pos)
#define ci_json_ary_insert_ptr(ary, ptr, pos)			__ci_json_ary_insert(ary, CI_JDT_PTR,  	ci_sizeof(ptr), 	ptr, 	pos)
#define ci_json_ary_insert_str(ary, str, pos)			__ci_json_ary_insert(ary, CI_JDT_STR, 	ci_strsize(str), 	str, 	pos)
#define ci_json_ary_insert_u8(ary, ui8, pos)			__ci_json_ary_insert(ary, CI_JDT_U8,  	ci_sizeof(u8), 		ui8, 	pos)
#define ci_json_ary_insert_u16(ary, ui16, pos)			__ci_json_ary_insert(ary, CI_JDT_U16,  	ci_sizeof(u16), 	ui16, 	pos)
#define ci_json_ary_insert_u32(ary, ui32, pos)			__ci_json_ary_insert(ary, CI_JDT_U32,  	ci_sizeof(u32), 	ui32, 	pos)
#define ci_json_ary_insert_u64(ary, ui64, pos)			__ci_json_ary_insert(ary, CI_JDT_U64,  	ci_sizeof(u64), 	ui64, 	pos)
#define ci_json_ary_insert_s8(ary, si8, pos)			__ci_json_ary_insert(ary, CI_JDT_S8,  	ci_sizeof(s8), 		si8, 	pos)
#define ci_json_ary_insert_s16(ary, si16, pos)			__ci_json_ary_insert(ary, CI_JDT_S16,  	ci_sizeof(s16), 	si16, 	pos)
#define ci_json_ary_insert_s32(ary, si32, pos)			__ci_json_ary_insert(ary, CI_JDT_S32,  	ci_sizeof(s32), 	si32, 	pos)
#define ci_json_ary_insert_s64(ary, si64, pos)			__ci_json_ary_insert(ary, CI_JDT_S64,  	ci_sizeof(s64), 	si64, 	pos)

#define __ci_json_ary_insert_bmp(ary, bmp, bits, pos)	\
	({	\
		int __size_in_byte__ = ci_bits_to_bmp(bits) * ci_sizeof(ci_bmp_t);		\
		__ci_json_ary_insert(ary, CI_JDT_BMP, __size_in_byte__, bmp, pos); 		\
	})
#define __ci_json_ary_insert_enum(ary, val, i2n, pos)		\
	({	\
		ci_json_data_t *__json_data_enum__ = ci_json_ary_insert(ary, (int)(val), pos);		\
		__json_data_enum__->flag |= CI_JDF_DUMP_ENUM;	\
		__json_data_enum__->int_to_name = i2n;	\
		__json_data_enum__;	\
	})
#define __ci_json_ary_insert_flag(ary, val, i2n, pos)		\
	({	\
		ci_json_data_t *__json_data_flag__ = ci_json_ary_insert(ary, (int)(val), pos);		\
		__json_data_flag__->flag |= CI_JDF_DUMP_FLAG;	\
		__json_data_flag__->int_to_name = i2n;	\
		__json_data_flag__;	\
	})

#define __ci_json_ary_insert_helper(jd, ary, val, type, pos)		\
	else if (ci_type_compatible(ci_typeof(val), type))	\
		jd = ci_json_ary_insert_ ## type(ary, val, pos)

#define __ci_json_ary_insert2(ary, pos)					ci_json_ary_insert_null(ary, pos)
#define __ci_json_ary_insert3(ary, val, pos)		\
	({	\
		ci_json_data_t *__json_ary_insert_data__;		\
		\
		if (ci_type_is_str(val) || ci_type_is_str_ptr(ci_typeof(val))) {	\
			char *__str__ = (char *)(uintptr_t)(val);	\
			__json_ary_insert_data__ = ci_json_ary_insert_str(ary, __str__, pos);	\
		}	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, int, pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, u8,  pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, u16, pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, u32, pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, u64, pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, s8,  pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, s16, pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, s32, pos);	\
		__ci_json_ary_insert_helper(__json_ary_insert_data__, ary, val, s64, pos);	\
		else	\
			__json_ary_insert_data__ = ci_json_ary_insert_ptr(ary, val, pos);	\
		\
		__json_ary_insert_data__;	\
	})
#define __ci_json_ary_insert4(ary, bin, size, pos)		__ci_json_ary_insert(ary, CI_JDT_BIN, size, bin, pos)		/* insert bin */


#define __ci_json_data_create(__balloc, __name, __type, __size, __val)		\
	({	\
		int __alloc_size__ = __ci_json_data_eval_alloc_size(__name, __type, __size);	\
		ci_json_data_t *__json_data__ = (ci_json_data_t *)ci_balloc(__balloc, __alloc_size__);	\
		\
		ci_assert(__json_data__);	\
		__ci_json_data_init(__json_data__, __name, __type, __size, __alloc_size__, (u64)(__val));		\
		\
		__json_data__;		\
	})
#define __ci_json_create(name, __balloc)		\
	({	\
		ci_json_t *__json__ = (ci_json_t *)ci_balloc(__balloc, ci_sizeof(ci_json_t));		\
		ci_assert((name) && (ci_strlen(name) > 0));	\
		ci_assert(__json__);	\
		\
		__json__->flag = __json__->max_name_len = 0;		\
		__json__->root = __ci_json_data_create(__balloc, name, CI_JDT_OBJ, ci_sizeof(ci_json_obj_t), 0);	\
		__json__->root->parent = __json__->root;		\
		__json__->root->json = __json__;	\
		__json__->ba = __balloc;	\
		\
		__json__;	\
	})
#define __ci_json_destroy(json)		\
	do {	\
		ci_json_del_data((json)->root);		\
		ci_dbg_nullify((json)->root);		\
		ci_bfree((json)->ba, json);		\
		ci_dbg_nullify(json);	\
	} while (0)
#define __ci_json_data_name_cmp(a, b)			ci_strcmp((a)->name, (b)->name)
#define __ci_json_data_type_is_num(type)		(((type) >= CI_JDT_NUM_START) && ((type) <= CI_JDT_NUM_END))
#define __ci_json_data_type_is_snum(type)		(__ci_json_data_type_is_num(type) && ((type) >= CI_JDT_SNUM_START))
#define __ci_json_data_type_is_unum(type)		(__ci_json_data_type_is_num(type) && ((type) < CI_JDT_SNUM_START))


/*
 *	obj_get and ary_get
 */
#define ci_type_num_compatible(type)		\
	(	\
		ci_type_compatible(type, int) 	|| 	\
	 	ci_type_compatible(type, u8) 	||	\
		ci_type_compatible(type, u16) 	||	\
		ci_type_compatible(type, u32) 	||	\
		ci_type_compatible(type, u64) 	||	\
		ci_type_compatible(type, s8)  	||	\
		ci_type_compatible(type, s16) 	||	\
		ci_type_compatible(type, s32) 	||	\
		ci_type_compatible(type, s64)	\
	 )
#define __ci_json_ary_get3(ary, idx, addr) 				__ci_json_ary_get4(ary, idx, addr, NULL)
#define __ci_json_ary_get4(ary, idx, addr, size_ptr)	\
	({	\
		int __ary_get_rv__ = 0;		\
		ci_json_data_t *__get_data__;	\
		\
		if ((__get_data__ = ci_json_ary_get_data(ary, idx))) 	\
			ci_json_data_get(__get_data__, addr, size_ptr);		\
		else {	\
			if (ci_type_num_compatible(ci_typeof(*(addr))))	\
				*(addr) = (ci_typeof(*(addr)))-1;		\
			else	\
				*(addr) = (ci_typeof(*(addr)))0;		\
			__ary_get_rv__ = -CI_E_NOT_FOUND;		\
		} 	\
		\
		__ary_get_rv__;		\
	})
#define __ci_json_ary_get_last(ary, ...)		\
	({	\
		int __ary_get_last_idx__ = ci_json_ary_nr_child(ary) - 1;		\
		ci_assert(__ary_get_last_idx__ >= 0);		\
		ci_json_ary_get(ary, __ary_get_last_idx__, __VA_ARGS__);		\
	})

		
#define __ci_json_obj_get3(obj, name, addr) 			__ci_json_obj_get4(obj, name, addr, NULL)
#define __ci_json_obj_get4(obj, name, addr, size_ptr)	\
	({	\
		int __obj_get_rv__ = 0;		\
		ci_json_data_t *__get_data__;	\
		ci_assert((obj) && name && ci_strlen(name));	\
		\
		if ((__get_data__ = ci_json_obj_get_data(obj, name))) 	\
			ci_json_data_get(__get_data__, addr, size_ptr);		\
		else {	\
			if (ci_type_num_compatible(ci_typeof(*(addr))))	\
				*(addr) = (ci_typeof(*(addr)))-1;		\
			else	\
				*(addr) = (ci_typeof(*(addr)))0;		\
			__obj_get_rv__ = -CI_E_NOT_FOUND;		\
		} 	\
		\
		__obj_get_rv__;		\
	})
#define ci_json_data_get(data, addr, size_ptr)		\
	({	\
		ci_json_data_t *__data__ = data;		\
		int *__size_ptr__ = size_ptr;		\
		ci_assert(addr);	\
		\
		if (ci_type_compatible(ci_typeof(*(addr)), ci_json_data_t *))	\
			(*addr) = (ci_typeof(*(addr)))(uintptr_t)__data__;		\
		else if (ci_type_num_compatible(ci_typeof(*(addr)))) {	/* num */	\
			ci_dbg_exec(	\
				if (!__ci_json_data_type_is_num(__data__->type))	\
					ci_warn_printf("integer/non-integer mismatch, target is %s%d, source is %s\n", 	\
									ci_type_is_unsigned(*(addr)) ? "u" : "s",	\
									ci_sizeof(*(addr)) * 8,		\
									ci_int_to_name(ci_jdt_to_name, __data__->type)), ci_here();		\
				else if (ci_type_is_unsigned(*(addr)) && __ci_json_data_type_is_snum(__data__->type))	\
					ci_warn_printf("signed/unsigned mismatch, target is unsigned, source is signed\n"), ci_here();	\
				else if (ci_type_is_signed(*(addr)) && __ci_json_data_type_is_unum(__data__->type))	\
					ci_warn_printf("signed/unsigned mismatch, target is signed, source is unsigned\n"), ci_here();	\
				\
				if (ci_sizeof(*(addr)) < __data__->size)		\
					ci_warn_printf("data truncated, target size=%d, source size=%d\n", ci_sizeof(*(addr)), __data__->size), ci_here();		\
			);	\
			*(addr) = (ci_typeof(*(addr)))__data__->val.ui64;		\
		} else {	\
			ci_dbg_exec(		\
				if (((ci_type_compatible(ci_typeof(*(addr)), char *)) || (ci_type_compatible(ci_typeof(*(addr)), const char *))) &&		\
					(__data__->type != CI_JDT_STR))	/* string */		\
					ci_warn_printf("str/non-str mismatch, target is str, source is %s\n", \
									ci_int_to_name(ci_jdt_to_name, __data__->type)), ci_here();	\
				else if (((ci_type_compatible(ci_typeof(*(addr)), u8 *)) || (ci_type_compatible(ci_typeof(*(addr)), const u8 *))) && 	\
					(__data__->type != CI_JDT_BIN))	/* binary */		\
					ci_warn_printf("bin/non-bin mismatch, target is bin/(u8 *), source is %s\n", \
									ci_int_to_name(ci_jdt_to_name, __data__->type)), ci_here();	\
			);	\
			\
			*(addr) = (ci_typeof(*(addr)))(uintptr_t)__data__->val.ptr;		/* ptr or bmp */	\
			__size_ptr__ && (*__size_ptr__ = __data__->size);		\
		}	\
	})



extern ci_int_to_name_t ci_jdt_to_name[];


ci_json_data_t *ci_json_obj_get_data(ci_json_data_t *pdata, char *name);
ci_json_data_t *ci_json_ary_get_data(ci_json_data_t *pdata, int idx);
int ci_json_obj_del(ci_json_data_t *pdata, char *name);
int ci_json_ary_del(ci_json_data_t *pdata, int idx);

void ci_json_del_data(ci_json_data_t *data);

int ci_json_dump(ci_json_t *json);
int ci_json_obj_nr_child(ci_json_data_t *pdata);
int ci_json_ary_nr_child(ci_json_data_t *pdata);


/* internal use functions */
int  __ci_json_data_eval_alloc_size(char *name, int type, int size);
void __ci_json_data_init(ci_json_data_t *data, char *name, int type, int size, int alloc_size, u64 val);


