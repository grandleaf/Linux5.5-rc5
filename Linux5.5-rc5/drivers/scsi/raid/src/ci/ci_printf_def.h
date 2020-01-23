/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_printf_def.h				CI Print Functions
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

/* box */
enum {
	CI_PR_BOX_ID_LEFT,
	CI_PR_BOX_ID_HLINE,
	CI_PR_BOX_ID_VLINE,
	CI_PR_BOX_ID_RIGHT,
	CI_PR_BOX_ID_NR
};

enum {
	CI_PR_BOX_TITLE_TOP,
	CI_PR_BOX_TITLE_TXT,
	CI_PR_BOX_TITLE_BTM,
	CI_PR_BOX_BODY_TOP,
	CI_PR_BOX_BODY_TXT,
	CI_PR_BOX_BODY_BTM,
	CI_PR_BOX_TYPE_NR
};

#define CI_PR_BOX_ALIGN_LEFT				0x00000000
#define CI_PR_BOX_ALIGN_RIGHT				0x10000000
#define CI_PR_BOX_ALIGN_CENTER				0x20000000
#define CI_PR_BOX_ALIGN_MASK				0xF0000000
#define CI_PR_BOX_SIZE_MASK					(~CI_PR_BOX_ALIGN_MASK)


/* outline */
#define CI_PR_OUTLINE_PREFIX				"  "	
#define CI_PR_OUTLINE_SUFFIX				" "	
enum {
	CI_PR_OUTLINE_NONE,						/* draw space */
	CI_PR_OUTLINE_INDIRECT,					/* draw | */
	CI_PR_OUTLINE_DIRECT,					/* draw |- */
	CI_PR_OUTLINE_DIRECT_LAST,				/* draw `- */
	CI_PR_OUTLINE_NR
};


#ifdef PAL_PR_UNICODE
#define CI_PR_STR_HLINE						"\u2500"		/* - */

#define CI_PR_OUTLINE_STR_NONE				"  "
#define CI_PR_OUTLINE_STR_INDIRECT			"\u2502" CI_PR_OUTLINE_STR_NONE		/* "|" */
#define CI_PR_OUTLINE_STR_DIRECT			"\u251C\u2500"	/* "|-" */
#define CI_PR_OUTLINE_STR_DIRECT_LAST		"\u2514\u2500"	/* "`-" */

#define CI_PR_BOX_STR_TITLE_TOP_LEFT		"\u2554"		/* + */
#define CI_PR_BOX_STR_TITLE_TOP_HLINE		"\u2550"		/* = */
#define CI_PR_BOX_STR_TITLE_TOP_VLINE		"\u2564"		/* T */
#define CI_PR_BOX_STR_TITLE_TOP_RIGHT		"\u2557"		/* + */

#define CI_PR_BOX_STR_TITLE_TXT_LEFT 		"\u2551"		/* || */
#define CI_PR_BOX_STR_TITLE_TXT_HLINE		" "
#define CI_PR_BOX_STR_TITLE_TXT_VLINE		"\u2502"		/* | */
#define CI_PR_BOX_STR_TITLE_TXT_RIGHT		"\u2551"		/* || */

#define CI_PR_BOX_STR_TITLE_BTM_LEFT		"\u2560"		/* ||= */
#define CI_PR_BOX_STR_TITLE_BTM_HLINE		"\u2550"		/* = */
#define CI_PR_BOX_STR_TITLE_BTM_VLINE		"\u256a"		/* + */
#define CI_PR_BOX_STR_TITLE_BTM_RIGHT		"\u2563"		/* -|| */

#define CI_PR_BOX_STR_BODY_TOP_LEFT			"\u255f"		/* ||- */
#define CI_PR_BOX_STR_BODY_TOP_HLINE		"\u2500"		/* - */
#define CI_PR_BOX_STR_BODY_TOP_VLINE		"\u253c"		/* + */
#define CI_PR_BOX_STR_BODY_TOP_RIGHT		"\u2562"		/* -|| */

#define CI_PR_BOX_STR_BODY_TXT_LEFT 		"\u2551"		/* || */
#define CI_PR_BOX_STR_BODY_TXT_HLINE		" "
#define CI_PR_BOX_STR_BODY_TXT_VLINE		"\u2502"		/* | */
#define CI_PR_BOX_STR_BODY_TXT_RIGHT		"\u2551"		/* || */

#define CI_PR_BOX_STR_BODY_BTM_LEFT			"\u255a"		/* + */
#define CI_PR_BOX_STR_BODY_BTM_HLINE		"\u2550"		/* = */
#define CI_PR_BOX_STR_BODY_BTM_VLINE		"\u2567"		/* upside down T */
#define CI_PR_BOX_STR_BODY_BTM_RIGHT		"\u255d"		/* + */
#else
#define CI_PR_STR_HLINE						"-"

#define CI_PR_OUTLINE_STR_NONE				"  "
#define CI_PR_OUTLINE_STR_INDIRECT			"| " CI_PR_OUTLINE_STR_NONE		/* "|" */
#define CI_PR_OUTLINE_STR_DIRECT			"|-"			/* "|-" */
#define CI_PR_OUTLINE_STR_DIRECT_LAST		"`-"			/* "`-" */

#define CI_PR_BOX_STR_TITLE_TOP_LEFT		"+"				/* + */
#define CI_PR_BOX_STR_TITLE_TOP_HLINE		"="				/* = */
#define CI_PR_BOX_STR_TITLE_TOP_VLINE		"="				/* T */
#define CI_PR_BOX_STR_TITLE_TOP_RIGHT		"+"				/* + */

#define CI_PR_BOX_STR_TITLE_TXT_LEFT 		"|"				/* || */
#define CI_PR_BOX_STR_TITLE_TXT_HLINE		" "
#define CI_PR_BOX_STR_TITLE_TXT_VLINE		"|"				/* | */
#define CI_PR_BOX_STR_TITLE_TXT_RIGHT		"|"				/* || */

#define CI_PR_BOX_STR_TITLE_BTM_LEFT		"+"				/* ||= */
#define CI_PR_BOX_STR_TITLE_BTM_HLINE		"="				/* = */
#define CI_PR_BOX_STR_TITLE_BTM_VLINE		"="				/* + */
#define CI_PR_BOX_STR_TITLE_BTM_RIGHT		"+"				/* -|| */

#define CI_PR_BOX_STR_BODY_TOP_LEFT			"|"				/* ||- */
#define CI_PR_BOX_STR_BODY_TOP_HLINE		"-"				/* - */
#define CI_PR_BOX_STR_BODY_TOP_VLINE		"+"				/* + */
#define CI_PR_BOX_STR_BODY_TOP_RIGHT		"|"				/* -|| */

#define CI_PR_BOX_STR_BODY_TXT_LEFT 		"|"				/* || */
#define CI_PR_BOX_STR_BODY_TXT_HLINE		" "
#define CI_PR_BOX_STR_BODY_TXT_VLINE		"|"				/* | */
#define CI_PR_BOX_STR_BODY_TXT_RIGHT		"|"				/* || */

#define CI_PR_BOX_STR_BODY_BTM_LEFT			"+"				/* + */
#define CI_PR_BOX_STR_BODY_BTM_HLINE		"="				/* = */
#define CI_PR_BOX_STR_BODY_BTM_VLINE		"="				/* upside down T */
#define CI_PR_BOX_STR_BODY_BTM_RIGHT		"+"				/* + */
#endif


