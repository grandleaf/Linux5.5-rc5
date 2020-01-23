/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_json.c												JSON
 *                                                          hua.ye@Hua Ye.com
 */
 
#include "ci.h"


ci_int_to_name_t ci_jdt_to_name[] = {
	ci_int_name(CI_JDT_OBJ),
	ci_int_name(CI_JDT_ARY),
	ci_int_name(CI_JDT_NULL),
	ci_int_name(CI_JDT_INT),
	ci_int_name(CI_JDT_PTR),
	ci_int_name(CI_JDT_STR),
	ci_int_name(CI_JDT_BIN),
	ci_int_name(CI_JDT_BMP),
	ci_int_name(CI_JDT_U8),
	ci_int_name(CI_JDT_U16),
	ci_int_name(CI_JDT_U32),
	ci_int_name(CI_JDT_U64),
	ci_int_name(CI_JDT_S8),
	ci_int_name(CI_JDT_S16),
	ci_int_name(CI_JDT_S32),
	ci_int_name(CI_JDT_S64),
	CI_EOT	
};

int __ci_json_data_eval_alloc_size(char *name, int type, int size)
{
	int alloc_size = ci_sizeof(ci_json_data_t);
	
	if (name)
		alloc_size += ci_strsize(name);

	if ((type == CI_JDT_STR) || (type == CI_JDT_BIN) || (type == CI_JDT_BMP))
		alloc_size += size;
	
	return alloc_size;
}

void __ci_json_data_init(ci_json_data_t *data, char *name, int type, int size, int alloc_size, u64 val)
{
	data->flag = 0;
	data->type = type;
	data->size = size;
	data->name = NULL;
	ci_obj_zero(&data->val);

	if (name) {
		int name_size = ci_strsize(name);
		data->name = (char *)data + alloc_size - name_size;
		ci_strlcpy(data->name, name, name_size);
	}	

	switch (type) {
		case CI_JDT_NULL:
			break;
		case CI_JDT_OBJ:
			ci_list_init(&data->val.obj.head);
			break;
		case CI_JDT_ARY:
			ci_list_init(&data->val.ary.head);
			break;
			
		case CI_JDT_STR:
			data->val.ptr = (char *)(data + 1);
			val && ci_strlcpy(data->val.ptr, (char *)(uintptr_t)val, size);
			break;
		case CI_JDT_BIN:
		case CI_JDT_BMP:
			data->val.ptr = (u8 *)(data + 1);
			val && ci_memcpy(data->val.ptr, (void *)(uintptr_t)val, size);
			break;
		case CI_JDT_PTR:
			data->val.ptr = (void *)(uintptr_t)val;
			break;
			
		case CI_JDT_INT:
		case CI_JDT_U8:
		case CI_JDT_U16:
		case CI_JDT_U32:
		case CI_JDT_U64:
		case CI_JDT_S8:
		case CI_JDT_S16:
		case CI_JDT_S32:
		case CI_JDT_S64:
			data->val.ui64 = val;
			break;

		default:
			ci_bug();
			break;
	}
}

int ci_json_obj_nr_child(ci_json_data_t *pdata)
{
	ci_assert(pdata && (pdata->type == CI_JDT_OBJ));
	return ci_list_count(&pdata->val.obj.head);
}

int ci_json_ary_nr_child(ci_json_data_t *pdata)
{
	ci_assert(pdata && (pdata->type == CI_JDT_ARY));
	return ci_list_count(&pdata->val.ary.head);
}

ci_json_data_t *ci_json_obj_get_data(ci_json_data_t *pdata, char *name)
{
	ci_assert(pdata && (pdata->type == CI_JDT_OBJ));
	ci_assert(name && ci_strlen(name));

	ci_json_obj_each(pdata, cdata, {
		ci_assert(cdata->name && ci_strlen(cdata->name));
		if (ci_strequal(cdata->name, name))
			return cdata;
	});

	return NULL;
}

ci_json_data_t *ci_json_ary_get_data(ci_json_data_t *pdata, int idx)
{
	ci_json_data_t *data;
	
	ci_assert(pdata && (pdata->type == CI_JDT_ARY));
	ci_range_check(idx, 0, ci_json_ary_nr_child(pdata));
	
	data = ci_list_obj_at_index(&pdata->val.ary.head, ci_json_data_t, link, idx);
	ci_assert(data);
	return data;
}

int ci_json_obj_del(ci_json_data_t *pdata, char *name)
{
	ci_json_data_t *data;

	if (!(data = ci_json_obj_get_data(pdata, name)))
		return -CI_E_NOT_FOUND;
	
	ci_json_del_data(data);
	return 0;
}

int ci_json_ary_del(ci_json_data_t *pdata, int idx)
{
	ci_json_data_t *data;

	if (!(data = ci_json_ary_get_data(pdata, idx)))
		return -CI_E_NOT_FOUND;
	
	ci_json_del_data(data);
	return 0;
}

void ci_json_del_data(ci_json_data_t *data)
{
	ci_balloc_t *ba;
	
	/* delete children recursively first */
	ci_assert(data);
	if (data->type == CI_JDT_OBJ)
		ci_json_obj_each_safe(data, child, ci_json_del_data(child));
	else if (data->type == CI_JDT_ARY)
		ci_json_ary_each_safe(data, child, ci_json_del_data(child));

	/* delete my self */
	if (data->parent != data)		/* not root */
		ci_list_del(&data->link);
	ci_assert(data->parent && data->json);
	ba = data->json->ba;
	ci_dbg_exec(data->parent = NULL, data->json = NULL);

	ci_bfree(ba, data);
}

int ci_list_outline(ci_list_t *ent, void (*get_parent)(ci_list_t *, ci_list_t **, ci_list_t **), int cur_idt, int prn_idt)
{
	ci_list_t *parent_head, *parent_list = ent;
	ci_assert(ent && get_parent && (cur_idt > prn_idt) && (prn_idt >= 0));

	ci_loop(cur_idt - prn_idt) {
		ent = parent_list;
		get_parent(parent_list, &parent_head, &parent_list);
	}
	
	if (cur_idt - 1 == prn_idt)		/* end node */
		return ci_list_next(parent_head, ent) ? CI_PR_OUTLINE_DIRECT : CI_PR_OUTLINE_DIRECT_LAST;

	return ci_list_next(parent_head, ent) ? CI_PR_OUTLINE_INDIRECT : CI_PR_OUTLINE_NONE;
}

static void ci_json_parent_list(ci_list_t *ent, ci_list_t **parent_head, ci_list_t **parent_list)
{
	ci_json_data_t *data = ci_container_of(ent, ci_json_data_t, link);
	ci_json_data_t *parent = data->parent;

	ci_assert(parent && ((parent->type == CI_JDT_OBJ) || (parent->type == CI_JDT_ARY)));
	ci_assert(parent_head && parent_list);
	
	*parent_head = parent->type == CI_JDT_OBJ ? &parent->val.obj.head : &parent->val.ary.head;
	*parent_list = &parent->link;
}

static int ci_json_data_dump(ci_json_data_t *data, int indent, int idx /* only valid for ary's children */)
{
	int rv = 0;

	ci_loop(print_indent, indent) 
		rv += ci_printf("%s", ci_pr_outline_str(ci_list_outline(&data->link, ci_json_parent_list, indent, print_indent)));

	if (data->json->flag & CI_JSON_DUMP_DATA_TYPE)
		rv += ci_printf("%-12s", ci_int_to_name(ci_jdt_to_name, data->type));

	if (data->name) {
		ci_assert(data->parent->type == CI_JDT_OBJ);
		if ((data->type != CI_JDT_OBJ) && (data->type != CI_JDT_ARY))
			rv += ci_printf(ci_ssf("%%-%ds", data->json->max_name_len), data->name);
		else
			rv += ci_printf("%s", data->name);
		
		if ((data->type != CI_JDT_OBJ) && (data->type != CI_JDT_ARY))
			rv += ci_printf(" : ");
	} else {
		ci_assert(data->parent->type == CI_JDT_ARY);
		rv += ci_printf(ci_ssf("%%%dd : ", ci_nr_digit(ci_json_ary_nr_child(data->parent))), idx);
	}

	switch (data->type) {
		case CI_JDT_OBJ:
			rv += ci_printf(" { %d }\n", ci_json_obj_nr_child(data));
			ci_json_obj_each(data, child, rv += ci_json_data_dump(child, indent + 1, 0));
			break;
		case CI_JDT_ARY:
			rv += ci_printf(" [ %d ]\n", ci_json_ary_nr_child(data));
			ci_json_ary_each_with_index(data, child, idx, rv += ci_json_data_dump(child, indent + 1, idx));
			break;

		case CI_JDT_NULL:
			rv += ci_printf("null\n");
			break;
		case CI_JDT_INT:
			if ((data->flag & CI_JDF_DUMP_ENUM) && data->int_to_name)			/* print enum */
				rv += ci_printf("%s : %d\n", ci_int_to_name(data->int_to_name, (int)data->val.ui64), (int)data->val.ui64);
			else if ((data->flag & CI_JDF_DUMP_FLAG) && data->int_to_name)		/* print flags */
				rv += ci_printf("%s : %#010X\n", ci_flag_str(data->int_to_name, (int)data->val.ui64), (int)data->val.ui64);
			else
				rv += ci_printf("%i - %#X\n", (int)data->val.ui64, (int)data->val.ui64);
			break;
		case CI_JDT_PTR:
			rv += ci_printf("%p\n", data->val.ptr);
			break;
			
		case CI_JDT_STR:
			rv += ci_printf("\"%s\"\n", (char *)data->val.ptr);
			break;
		case CI_JDT_BIN: {
				u8 *p = (u8 *)data->val.ptr;
				int data_size = data->size;

				rv += ci_printf("%d / %#X, [ ", data_size, data_size);
				ci_loop(ci_min(data_size, CI_PR_JSON_BIN_LEN)) {
					rv += ci_printf("%02X ", *p);	/* ci_printf() checking will cause *p++ eval twice */
					p++;
				}
				if (CI_PR_JSON_BIN_LEN < data_size)
					rv += ci_printf("... ");
				rv += ci_printf("]\n");
			}
			break;
		case CI_JDT_BMP: {
			int data_size = data->size;
			ci_bmp_dump(data->val.ptr, ci_min(data_size, CI_PR_JSON_BIN_LEN) * 8);	/* might dump a little bit more */
			if (CI_PR_JSON_BIN_LEN < data_size)
				rv += ci_printf("... ");
			rv += ci_printf("\n");
			break;
		}
			
		case CI_JDT_U8:
			rv += ci_printf("%i - %#02X\n", (u8)data->val.ui64, (u8)data->val.ui64);
			break;
		case CI_JDT_U16:
			rv += ci_printf("%i - %#04X\n", (u16)data->val.ui64, (u16)data->val.ui64);
			break;
		case CI_JDT_U32:
			rv += ci_printf("%lli - %#08X\n", (u64)data->val.ui64, (u32)data->val.ui64);
			break;
		case CI_JDT_U64:
			rv += ci_printf("%lli - %#016llX\n", data->val.ui64, data->val.ui64);
			break;

		case CI_JDT_S8:
			rv += ci_printf("%i - %#02X\n", (s8)data->val.ui64, (u8)data->val.ui64);
			break;
		case CI_JDT_S16:
			rv += ci_printf("%i - %#04X\n", (s16)data->val.ui64, (u16)data->val.ui64);
			break;
		case CI_JDT_S32:
			rv += ci_printf("%lli - %#08X\n", (s64)data->val.ui64, (u32)data->val.ui64);
			break;
		case CI_JDT_S64:
			rv += ci_printf("%lli - %#016llX\n", data->val.ui64, data->val.ui64);
			break;
			
		default:
			rv += ci_printf("print for %s not implemented\n", ci_int_to_name(ci_jdt_to_name, data->type));
			break;
	}

	return rv;
}

int ci_json_dump(ci_json_t *json)
{
	return ci_json_data_dump(json->root, 0, 0);
}

 
