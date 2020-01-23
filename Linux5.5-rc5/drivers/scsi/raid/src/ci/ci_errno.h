/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_errno.h					CI Error Number Definition
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

enum {
	CI_E_NO_ERROR,		/* success */
	CI_E_NOT_FOUND,		/* entry/object/file not found */
	CI_E_AGAIN,			/* retry */
	CI_E_INVALID,		/* something wrong */
	CI_E_FORMAT,		/* wrong format */
	CI_E_EXIST,			/* already exist */
	CI_E_END_OF_FILE,	/* end of file */
	CI_E_TRUNCATED,		/* data truncated */
	CI_E_DUPLICATE,		/* duplicate */
	CI_E_MISSING,		/* something expected is missing */
	CI_E_EXTRA,			/* something extra unexpected */
	CI_E_UNRECOGNIZED,	/* get unrecognized data */
	CI_E_NOT_SET,		/* data should be set */
	CI_E_UNINITIALIZED,	/* unitialized data found */
	CI_E_TYPE,			/* type error/mismatch */
	CI_E_ASYNC,			/* asynchronous, need callback */
	CI_E_RANGE,			/* out of range */
	CI_E_NR				/* total */
};

