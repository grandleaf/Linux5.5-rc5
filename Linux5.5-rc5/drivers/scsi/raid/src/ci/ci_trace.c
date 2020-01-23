/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_trace.c				Trace Utilities
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

/*
 *	XXX, Temporary
 */
ci_trace_mgr_t				 trace_mgr;


int ci_trace_mgr_init(ci_trace_mgr_t *mgr)
{
	ci_obj_zero(mgr);
	ci_slk_init(&mgr->lock);

	ci_loop(i, CI_NR_TRACE_CHUNK) {	
		ci_trace_chunk_t *tc = (ci_trace_chunk_t *)pal_zalloc(ci_sizeof(ci_trace_chunk_t));	/* ci_todo ... */
		tc->meta = (ci_trace_chunk_meta_t *)pal_zalloc(CI_TRACE_CHUNK_SIZE);
		tc->end	 = (u8 *)tc->meta + CI_TRACE_CHUNK_SIZE;
		if (!mgr->curr) {
			mgr->curr = mgr->first = tc;
			ci_list_init(&mgr->curr->link);
		}
//		tc->meta->id = i;
		tc->mgr = mgr;
		ci_list_add_tail(&mgr->curr->link, &tc->link);
	}
	
	return 0;
}

ci_trace_chunk_t *ci_trace_mgr_get_chunk(ci_trace_mgr_t *mgr, ci_trace_chunk_t *old)
{
	ci_trace_chunk_t *tc;

	ci_slk_protected(&mgr->lock, {	
		old && (old->flag &= ~CI_TRACE_CHUNK_BUSY);
		tc = mgr->curr;
		
		ci_big_loop(CI_NR_TRACE_CHUNK) {
			if (tc->flag & CI_TRACE_CHUNK_BUSY) {
				if (ci_unlikely(tc->pass < mgr->pass))
					tc->meta->ts_walk = pal_timestamp();		/* tell the utility to discard old frames */
				if (ci_unlikely((tc = ci_list_next_obj(&tc->link, tc, link)) == mgr->first))
					mgr->pass++;
				
				continue;
			}

			/* init a new tc */
			tc->meta->ts_walk = 0;	/* utility detect time stamp in reverse order, then discard */
			tc->pass = mgr->pass;
			tc->flag |= CI_TRACE_CHUNK_BUSY;
			tc->curr = (u8 *)tc->meta + CI_TRACE_FRAME_META_SIZE;
			break;
		}
		
		if (ci_unlikely((mgr->curr = ci_list_next_obj(&tc->link, tc, link)) == mgr->first))
			mgr->pass++;
		ci_assert(mgr->curr, "CI_TRACE_BUF_SIZE=%#X is too small", CI_TRACE_BUF_SIZE);
	});

//	ci_printf("########### get tc idx=%d\n", tc->meta->id);
//	ci_printf("########### get tc idx=%p\n", tc->meta);
	return tc;
}


