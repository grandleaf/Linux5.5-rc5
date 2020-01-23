#include "ci.h"

/* enable one of these */
//#define TEST0
//#define TEST1
#define TEST2


#define CI_JSON_TEST_MAP_BITS			65
ci_bmp_def(ci_json_test_map, CI_JSON_TEST_MAP_BITS);


#ifdef TEST0	/* well, it takes a while to compile this funtion */
void test_json()
{
	ci_json_t *json = ci_json_create("raid_team");
	ci_json_data_t *remley = ci_json_set_obj(json, "Manager");
	
	ci_json_obj_set(remley, "title", 			"Director, Software Development");
	ci_json_obj_set(remley, "first_name", 		"Paul");
	ci_json_obj_set(remley, "last_name", 		"Remley");
	ci_json_obj_set(remley, "work_phone", 		"+1 408-276-0690");
	ci_json_obj_set(remley, "mobile_phone", 	"+1 925-858-6053");
	ci_json_obj_set(remley, "work_email",		"paul.remley@Hua Ye.com");
	ci_json_obj_set(remley, "office", 			2316);
	ci_json_data_t *remley_directs = ci_json_obj_set_ary(remley, "directs");
	
	ci_json_data_t *stark = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(stark, "first_name", 		"Paul");
	ci_json_obj_set(stark, "last_name", 		"Stark");
	ci_json_obj_set(stark, "title", 			"Senior Manager Sustaining Development");
	ci_json_obj_set(stark, "work_phone", 		"+1 408-276-0550");
	ci_json_obj_set(stark, "mobile_phone", 		"+1 408-799-9476");
	ci_json_obj_set(stark, "work_email",		"paul.x.stark@Hua Ye.com");
	ci_json_obj_set(stark, "office", 			2312);
	ci_json_data_t *stark_directs = ci_json_obj_set_ary(stark, "directs");

	ci_json_data_t *rama = ci_json_ary_add_obj(stark_directs);
	ci_json_obj_set(rama, "first_name", 		"Vishvanathan");
	ci_json_obj_set(rama, "middle_name", 		"Alur");
	ci_json_obj_set(rama, "last_name", 			"Ramamurthy");
	ci_json_obj_set(rama, "title", 				"Manager, Software Development");
	ci_json_obj_set(rama, "work_phone", 		"+91 40 6724 4438");
	ci_json_obj_set(rama, "mobile_phone", 		"+91 77022 22704");
	ci_json_obj_set(rama, "work_email",			"vishvanathan.alur.ramamurthy@Hua Ye.com");
	ci_json_obj_set(rama, "office", 			"10A229");
	ci_json_data_t *rama_directs = ci_json_obj_set_ary(rama, "directs");

	ci_json_data_t *bisoyi = ci_json_ary_add_obj(rama_directs);
	ci_json_obj_set(bisoyi, "first_name", 		"Anup");
	ci_json_obj_set(bisoyi, "last_name", 		"Bisoyi");
	ci_json_obj_set(bisoyi, "title", 			"Software Developer");
	ci_json_obj_set(bisoyi, "mobile_phone", 	"+91 86865 91508");
	ci_json_obj_set(bisoyi, "work_email",		"vishvanathan.alur.ramamurthy@Hua Ye.com");
	ci_json_obj_set(bisoyi, "office", 			"10A219");	
	
	ci_json_data_t *gudala = ci_json_ary_add_obj(rama_directs);
	ci_json_obj_set(gudala, "first_name", 		"Manoj");
	ci_json_obj_set(gudala, "middle_name", 		"Kumar");
	ci_json_obj_set(gudala, "last_name", 		"Gudala");
	ci_json_obj_set(gudala, "title", 			"Software Developer");
	ci_json_obj_set(gudala, "work_phone", 		"+91 40 6724 4444");
	ci_json_obj_set(gudala, "work_email",		"umadevi.byrappa@Hua Ye.com");
	ci_json_obj_set(gudala, "mobile_phone", 	"+91 87903 74747");
	ci_json_obj_set(gudala, "office", 			"10A231");	

	ci_json_data_t *inguva = ci_json_ary_add_obj(rama_directs);
	ci_json_obj_set(inguva, "first_name", 		"Vamsi");
	ci_json_obj_set(inguva, "middle_name", 		"Krishna");
	ci_json_obj_set(inguva, "last_name", 		"Inguva");
	ci_json_obj_set(inguva, "title", 			"Software Developer");
	ci_json_obj_set(inguva, "work_phone", 		"+91 40 6724 6858");
	ci_json_obj_set(inguva, "mobile_phone", 	"+91 99482 74119");
	ci_json_obj_set(inguva, "work_email",		"vamsi.inguva@Hua Ye.com");
	ci_json_obj_set(inguva, "office", 			"10A230");		

	ci_json_data_t *reddy = ci_json_ary_add_obj(rama_directs);
	ci_json_obj_set(reddy, "middle_name", 		"Krishna");
	ci_json_obj_set(reddy, "last_name", 		"Reddy");
	ci_json_obj_set(reddy, "title", 			"Software Developer");
	ci_json_obj_set(reddy, "work_phone", 		"+91 40 6724 4439");
	ci_json_obj_set(reddy, "work_email",		"rajasekhar.devarapalli@Hua Ye.com");
	ci_json_obj_set(reddy, "office", 			"10A218");		

	ci_json_data_t *sahoo = ci_json_ary_add_obj(rama_directs);
	ci_json_obj_set(sahoo, "first_name", 		"Subhakanta");
	ci_json_obj_set(sahoo, "last_name", 		"Sahoo");
	ci_json_obj_set(sahoo, "title", 			"Software Developer");
	ci_json_obj_set(sahoo, "work_phone", 		"+91 40 6724 4440");
	ci_json_obj_set(sahoo, "mobile_phone", 		"+91 90002 40741");
	ci_json_obj_set(sahoo, "work_email",		"sahoo.subhakanta@Hua Ye.com");
	ci_json_obj_set(sahoo, "office", 			"10A217");		

	ci_json_data_t *byrappa = ci_json_ary_add_obj(stark_directs);
	ci_json_obj_set(byrappa, "first_name", 		"Umadevi");
	ci_json_obj_set(byrappa, "last_name", 		"Byrappa");
	ci_json_obj_set(byrappa, "title", 			"Principal Software Developer");
	ci_json_obj_set(byrappa, "work_phone", 		"+1 408-276-7963");
	ci_json_obj_set(byrappa, "work_email",		"umadevi.byrappa@Hua Ye.com");
	ci_json_obj_set(byrappa, "office", 			"2216");

	ci_json_data_t *hunter = ci_json_ary_add_obj(stark_directs);
	ci_json_obj_set(hunter, "first_name", 		"Zach");
	ci_json_obj_set(hunter, "last_name", 		"Hunter");
	ci_json_obj_set(hunter, "title", 			"Principal Software Developer");
	ci_json_obj_set(hunter, "work_phone", 		"+1 408-276-0639");
	ci_json_obj_set(hunter, "mobile_phone", 	"+1 408-529-6526");
	ci_json_obj_set(hunter, "work_email",		"zach.hunter@Hua Ye.com");
	ci_json_obj_set(hunter, "office", 			"2306");

	ci_json_data_t *pasa = ci_json_ary_add_obj(stark_directs);
	ci_json_obj_set(pasa, "first_name", 		"Srinivas");
	ci_json_obj_set(pasa, "last_name", 			"Pasagadugula");
	ci_json_obj_set(pasa, "title", 				"Senior Software Developer");
	ci_json_obj_set(pasa, "work_phone", 		"+1 408-276-0657");
	ci_json_obj_set(pasa, "mobile_phone", 		"+1 336-489-0257");
	ci_json_obj_set(pasa, "work_email",			"zach.hunter@Hua Ye.com");
	ci_json_obj_set(pasa, "office", 			"2101");	

	ci_json_data_t *pathi = ci_json_ary_add_obj(stark_directs);
	ci_json_obj_set(pathi, "first_name", 		"Poorna");
	ci_json_obj_set(pathi, "middle_name", 		"x");
	ci_json_obj_set(pathi, "last_name", 		"Pathipaka ");
	ci_json_obj_set(pathi, "title", 			"Principal Software Developer");
	ci_json_obj_set(pathi, "work_phone", 		"+91 40 6724 4443");
	ci_json_obj_set(pathi, "mobile_phone", 		"+1 408-416-7555");
	ci_json_obj_set(pathi, "work_email",		"poorna.x.pathipaka@Hua Ye.com");
	ci_json_obj_set(pathi, "office", 			"2215");		

	ci_json_data_t *zhao = ci_json_ary_add_obj(stark_directs);
	ci_json_obj_set(zhao, "first_name", 		"Bryan");
	ci_json_obj_set(zhao, "last_name", 			"Zhao");
	ci_json_obj_set(zhao, "title", 				"Software Developer");
	ci_json_obj_set(zhao, "work_phone", 		"+1 408-276-5868");
	ci_json_obj_set(zhao, "mobile_phone", 		"+1 510-386-4368");
	ci_json_obj_set(zhao, "work_email",			"boyan.zhao@Hua Ye.com");
	ci_json_obj_set(zhao, "office", 			"B330A");	

	ci_json_data_t *dewilde = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(dewilde, "first_name", 		"Brian");
	ci_json_obj_set(dewilde, "last_name", 		"DeWilde");
	ci_json_obj_set(dewilde, "title", 			"Principal Software Developer");
	ci_json_obj_set(dewilde, "work_phone", 		"+1 408-276-0624");
	ci_json_obj_set(dewilde, "mobile_phone", 	"+1 408-276-0624");
	ci_json_obj_set(dewilde, "work_email",		"brian.dewilde@Hua Ye.com");
	ci_json_obj_set(dewilde, "office", 			"2217");	

	ci_json_data_t *indur = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(indur, "first_name", 		"Ravi");
	ci_json_obj_set(indur, "last_name", 		"Indurkar");
	ci_json_obj_set(indur, "title", 			"Principal Software Developer");
	ci_json_obj_set(indur, "work_phone", 		"+1 408-276-0592");
	ci_json_obj_set(indur, "mobile_phone", 		"+1 408-460-0213");
	ci_json_obj_set(indur, "work_email",		"ravi.indurkar@Hua Ye.com");
	ci_json_obj_set(indur, "office", 			"2320");	

	ci_json_data_t *lang = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(lang, "first_name", 		"Jesse");
	ci_json_obj_set(lang, "last_name", 			"Langham");
	ci_json_obj_set(lang, "title", 				"RAID Architect");
	ci_json_obj_set(lang, "mobile_phone", 		"+1 510-427-1834");
	ci_json_obj_set(lang, "work_email",			"jesse.langham@Hua Ye.com");
	ci_json_obj_set(lang, "office", 			"2314");

	ci_json_data_t *ly = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(ly, "first_name", 		"Nguyen");
	ci_json_obj_set(ly, "last_name", 		"Ly");
	ci_json_obj_set(ly, "title", 			"Principal Software Developer");
	ci_json_obj_set(ly, "work_phone", 		"+1 408-276-1307");
	ci_json_obj_set(ly, "work_email",		"nguyen.ly@Hua Ye.com");
	ci_json_obj_set(ly, "office", 			"2265");	

	ci_json_data_t *masles = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(masles, "first_name", 		"John");
	ci_json_obj_set(masles, "last_name", 		"Masles");
	ci_json_obj_set(masles, "title", 			"Architect");
	ci_json_obj_set(masles, "work_email",		"john.masles@Hua Ye.com");
	ci_json_obj_set(masles, "office", 			"2208");

	ci_json_data_t *mei = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(mei, "first_name", 		"Kumar");
	ci_json_obj_set(mei, "last_name", 		"Meiyappan ");
	ci_json_obj_set(mei, "title", 			"Senior Principal Software Developer");
	ci_json_obj_set(mei, "work_email",		"kumar.meiyappan@Hua Ye.com");
	ci_json_obj_set(mei, "office", 			"2302");	

	ci_json_data_t *sawdy = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(sawdy, "first_name", 		"Don");
	ci_json_obj_set(sawdy, "last_name", 		"Sawdy");
	ci_json_obj_set(sawdy, "title", 			"Principal Software Developer");
	ci_json_obj_set(sawdy, "work_phone", 		"+1 408-276-0686");
	ci_json_obj_set(sawdy, "mobile_phone", 		"+1 408-205-6018");
	ci_json_obj_set(sawdy, "work_email",		"don.sawdy@Hua Ye.com");
	ci_json_obj_set(sawdy, "office", 			"2212");

	ci_json_data_t *scott = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(scott, "first_name", 		"Ron");
	ci_json_obj_set(scott, "last_name", 		"Scott");
	ci_json_obj_set(scott, "title", 			"Principal Software Developer");
	ci_json_obj_set(scott, "mobile_phone", 		"+1 918-855-7655");
	ci_json_obj_set(scott, "work_email",		"ron.scott@Hua Ye.com");
	ci_json_obj_set(scott, "office", 			"2223");	

	ci_json_data_t *sheth = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(sheth, "first_name", 		"Parag");
	ci_json_obj_set(sheth, "middle_name", 		"a");
	ci_json_obj_set(sheth, "last_name", 		"Sheth");
	ci_json_obj_set(sheth, "title", 			"Senior Software Developer");
	ci_json_obj_set(sheth, "work_phone", 		"+1 408-276-1132");
	ci_json_obj_set(sheth, "mobile_phone", 		"+1 916-897-6961");
	ci_json_obj_set(sheth, "work_email",		"parag.a.sheth@Hua Ye.com");
	ci_json_obj_set(sheth, "office", 			"2257");

	ci_json_data_t *swiger = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(swiger, "first_name", 		"Dan");
	ci_json_obj_set(swiger, "last_name", 		"Swiger");
	ci_json_obj_set(swiger, "title", 			"Senior Principal Software Developer");
	ci_json_obj_set(swiger, "work_email",		"dan.swiger@Hua Ye.com");
	ci_json_obj_set(swiger, "office", 			"2210");	

	ci_json_data_t *ye = ci_json_ary_add_obj(remley_directs);
	ci_json_obj_set(ye, "first_name", 		"Hua");
	ci_json_obj_set(ye, "last_name", 		"Ye");
	ci_json_obj_set(ye, "title", 			"Principal Software Developer");
	ci_json_obj_set(ye, "work_email",		"hua.ye@Hua Ye.com");
	ci_json_obj_set(ye, "office", 			"2318");
	
	ci_json_dump(json);
	ci_json_destroy(json);
	ci_json_balloc_dump();	
}
#endif


#ifdef TEST1
void test_json()
{
	ci_here();

	ci_json_t *json = ci_json_create("json_test");
//	json->flag |= CI_JSON_DUMP_DATA_TYPE;		// CI_JSON_DUMP_DATA_TYPE 	CI_JSON_SORT

	u8 u8_ary[512] = { 0x00, 0x11, 0x22, 0x33, 0x44 };
	ci_json_test_map_t map;
	ci_json_test_map_fill(&map);

#define TEST_ENUM_0		1
#define TEST_ENUM_1		2
#define TEST_ENUM_2		4

	static ci_int_to_name_t test_enum_int_to_name[] = {
		ci_int_name(TEST_ENUM_0),
		ci_int_name(TEST_ENUM_1),
		ci_int_name(TEST_ENUM_2),
		CI_EOT	
	};	


#if 1
	/* null type */
	ci_json_set(json, "verbose");
	
	/* integers */
	ci_json_set(json, "u8", 	(u8)0xFF);
	ci_json_set(json, "u16",	(u16)0xFFFF);
	ci_json_set(json, "u32",	(u32)0xFFFFFFFF);
	ci_json_set(json, "u64",	(u64)0x7FFFFFFFFFFFFFFFull);	
	ci_json_set(json, "u64_max",(u64)0xFFFFFFFFFFFFFFFFull);	/* minus print problem */
	ci_json_set(json, "s8", 	(s8)0xFF);
	ci_json_set(json, "s16",	(s16)0xFFFF);
	ci_json_set(json, "s32",	(s32)0xFFFFFFFF);
	ci_json_set(json, "s64",	(s64)0x7FFFFFFFFFFFFFFFull);	

	/* pointer */
	ci_json_set(json, "ptr",	(void *)3);

	/* string */
	char str[] = "a string";
	ci_json_set(json, "str",	str);

	/* integer and pointer */
	int i = 520;
	ci_json_set(json, "int",	i);
	ci_json_set(json, "int&", 	&i);
	
	/* binary stream */
	ci_json_set(json, "u8_ary[]_ptr",	u8_ary);		/* pointer if size is not provided */
	ci_json_set(json, "u8_ary[]", u8_ary, ci_sizeof(u8_ary));	/* binarys stream copy */

	/* bitmap test */
	ci_json_set_bmp(json, "ci_json_test_map", &map, CI_JSON_TEST_MAP_BITS);


	/*
	 *	enum and flags
	 */
	ci_json_set_enum(json, "enum_test0", 	TEST_ENUM_0, 	test_enum_int_to_name);
	ci_json_set_enum(json, "enum_test1", 	TEST_ENUM_1, 	NULL);
	ci_json_set_enum(json, "enum_test1", 	250, 			test_enum_int_to_name);

	ci_json_set_flag(json, "flag_test0",	TEST_ENUM_0 | TEST_ENUM_2, test_enum_int_to_name);
	ci_json_set_flag(json, "flag_test2",	TEST_ENUM_0 | TEST_ENUM_2, NULL);
	ci_json_set_flag(json, "flag_test2",	TEST_ENUM_0 | TEST_ENUM_1 | TEST_ENUM_2 | 0x8000, test_enum_int_to_name);

	/* delete test */
	ci_json_del(json, "flag_test2");

	ci_json_data_t *jd = ci_json_set(json, "gaga", "gaga");
	ci_json_del_data(jd);
	 
#endif

#if 1	
	/* add a new obj */
	ci_json_data_t *obj = ci_json_set_obj(json, "new_employee");
	ci_json_obj_set(obj, "last_name",  	"John");
	ci_json_obj_set(obj, "first_name", 	"Doe");
	ci_json_obj_set(obj, "company", 	"Hua Ye America Inc.");
	ci_json_obj_set(obj, "age", 		34);
	ci_json_obj_set(obj, "weight", 		180);


	
	/* add an array */
	ci_json_data_t *ary = ci_json_set_ary(json, "fruits");
	ci_loop(i, 10)
		ci_json_ary_add(ary, i);

	/* basic integers */
	ci_json_ary_add(ary);			/* add a null */
	ci_json_ary_add(ary, 55);		/* add an int */
	ci_json_ary_add(ary, (u8)8);	
	ci_json_ary_add(ary, (u16)16);	
	ci_json_ary_add(ary, (u32)32);	
	ci_json_ary_add(ary, (u64)64);	
	ci_json_ary_add(ary, (s8)8);	
	ci_json_ary_add(ary, (s16)16);	
	ci_json_ary_add(ary, (s32)32);	
	ci_json_ary_add(ary, (s64)64);	
	ci_json_ary_add(ary, (s64)64);	
	ci_json_ary_add(ary, "John Doe");	
	ci_json_ary_add_head(ary, "Doe John");	
	ci_json_ary_add(ary, ary);		/* pointer */
	ci_json_ary_add(ary, u8_ary);	/* another pointer */
	ci_json_ary_add(ary, u8_ary, ci_sizeof(u8_ary));	/* add binary stream */
	ci_json_ary_insert(ary, "position5", 5);

	/* add bmp */
	ci_json_ary_add_bmp(ary, &map, CI_JSON_TEST_MAP_BITS);

	/* add enum */
	ci_json_ary_add_enum(ary, TEST_ENUM_0, 	test_enum_int_to_name);
	ci_json_ary_add_enum(ary, TEST_ENUM_1, 	NULL);
	ci_json_ary_add_enum(ary, 250, 	test_enum_int_to_name);

	/* add flag */
	ci_json_ary_add_flag(ary, TEST_ENUM_0 | TEST_ENUM_2, test_enum_int_to_name);
	ci_json_ary_add_flag(ary, TEST_ENUM_0 | TEST_ENUM_2, NULL);
	ci_json_ary_add_flag(ary, TEST_ENUM_0 | TEST_ENUM_1 | TEST_ENUM_2 | 0x8000, test_enum_int_to_name);

	/* add obj */
	ci_json_data_t *new_obj = ci_json_ary_add_obj(ary);
	ci_json_obj_set(new_obj, "last_name", "Jagular");
	ci_json_obj_set(new_obj, "first_name", "Tiger");

	/* add another array */
	ci_json_data_t *new_ary = ci_json_ary_add_ary(ary);
	ci_json_ary_add(new_ary, "My");
	ci_json_ary_add(new_ary, 334455ull);

	ci_json_ary_del(new_ary, 0);
//	ci_json_ary_del(new_ary, 25);	// assert triggered
		
#endif

	ci_json_dump(json);

	ci_json_balloc_dump();
	ci_json_destroy(json);
	ci_json_balloc_dump();
}
#endif



#ifdef TEST2
void test_json()
{
	ci_json_t *json = ci_json_create("json_test");
	int size = 0;

	/* set int and get */
	ci_json_set(json, "int", 0xFF);
	int get_int;
	ci_json_get(json, "int", &get_int);
	ci_printf("get_int=%d\n", get_int);

	/* set string and get */
	ci_json_set(json, "str", "string");
	const char *get_str;
	ci_json_get(json, "str", &get_str, &size);
	ci_printf("get_str=%s\n", get_str);
	ci_printf("size=%d\n", size);

	/* set bin and get */
	u8 u8_ary[512] = { 0x00, 0x11, 0x22, 0x33, 0x44 };
	ci_json_set(json, "u8_ary[]", u8_ary, ci_sizeof(u8_ary));	/* binarys stream copy */
	const u8 *get_u8_ary;
	ci_json_obj_get(json->root, "u8_ary[]", &get_u8_ary, &size);
	ci_printf("get_u8_ary=%02X %02X %02X %02X %02X\n", get_u8_ary[0], get_u8_ary[1], get_u8_ary[2], get_u8_ary[3], get_u8_ary[4]);
	ci_printf("size=%d\n", size);

	/* set bmp and get */
	ci_json_test_map_t map;
	ci_json_test_map_fill(&map);
	ci_json_set_bmp(json, "ci_json_test_map", &map, CI_JSON_TEST_MAP_BITS);
	ci_json_test_map_t *map_ptr;
	ci_json_get(json, "ci_json_test_map", &map_ptr, &size);
	ci_json_test_map_dumpln(map_ptr);
	ci_printf("size=%d\n", size);

	/* add an array */
	ci_json_data_t *ary = ci_json_set_ary(json, "fruits");
	ci_loop(i, 10)
		ci_json_ary_add(ary, i);
	ci_json_ary_add(ary, "what_a_string");

	ci_json_ary_get(ary, 3, &get_int);
	ci_printf("from array get_int=%d\n", get_int);

	char *get_str2;
	ci_json_ary_get(ary, 10, &get_str2, &size);
	ci_printf("get_str2=%s\n", get_str2);
	ci_printf("size=%d\n", size);
		
	ci_json_dump(json);
	ci_json_destroy(json);
	ci_json_balloc_dump();	
}
#endif



