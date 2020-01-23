#include "ci.h"

#define TEST_SCHED_ENT_NR		128
#define TEST_SCHED_NODE_ID		0
#define TEST_SCHED_WORKER_ID	0
#define TEST_SCHED_COUNTDOWN	(2000000ull / 1)

typedef struct {
	int						 node_id;
	int						 worker_id;
	int			 			 req_countdown;
	ci_perf_data_t			*perf_data;
} test_sched_data_t;

typedef struct {
	int						 id;
	u64			 			 countdown;
	ci_sched_ent_t			 sched_ent;
	test_sched_data_t		*data;
} test_sched_req_t;

static ci_sched_grp_t *sched_grp;
static test_sched_req_t *test_sched_req;
static test_sched_data_t *test_sched_data;
static ci_perf_data_t *test_sched_perf_data;

static void ci_sched_grp_init2(ci_sched_grp_t *grp)
{
	ci_obj_zero(grp);

	ci_list_init(&grp->ehead);
	ci_list_init(&grp->rhead);
	ci_slk_init(&grp->lock);
}

static void sched_test_ent_exec(ci_sched_ctx_t *ctx)
{
	test_sched_req_t *r = ci_container_of(ctx->sched_ent, test_sched_req_t, sched_ent);

	if (--r->countdown)
		ci_sched_run_add(ctx->sched_grp, ctx->sched_ent);		// ci_sched_ext_add ci_sched_run_add
	else {
		test_sched_data_t *data = r->data;
		if (!--data->req_countdown) {
			ci_printf("< %d, %02d > ", data->node_id, data->worker_id);
			ci_perf_eval_end(data->perf_data, 1);
		}
	}
}

static void sched_stress(int node_id, int worker_id)
{
	sched_grp = ci_node_halloc(node_id, ci_sizeof(ci_sched_grp_t), 0, "test_sched_grp");
	test_sched_req = ci_node_halloc(node_id, ci_sizeof(test_sched_req_t) * TEST_SCHED_ENT_NR, 0, "test_sched_req");
	test_sched_data = ci_node_halloc(node_id, ci_sizeof(test_sched_data_t), 0, "test_sched_data"); 
	test_sched_perf_data = ci_node_halloc(node_id, ci_sizeof(ci_perf_data_t), 0, "ci_perf_data"); 
	
	ci_obj_zero(test_sched_data);
	test_sched_data->req_countdown = TEST_SCHED_ENT_NR;
	test_sched_data->perf_data = test_sched_perf_data;
	test_sched_data->node_id = node_id;
	test_sched_data->worker_id = worker_id;

	ci_obj_zero(test_sched_perf_data);
	test_sched_perf_data->nr_io = TEST_SCHED_ENT_NR * TEST_SCHED_COUNTDOWN;
	
	ci_sched_grp_init2(sched_grp);
	sched_grp->prio = 15;
	sched_grp->tab = &ci_worker_by_id(node_id, worker_id)->sched_tab;

	ci_printf("< %d, %02d > ", node_id, worker_id);
	ci_perf_eval_start(test_sched_perf_data, 1);

	ci_loop(i, TEST_SCHED_ENT_NR) {
		test_sched_req_t *r = test_sched_req + i;

		ci_obj_zero(r);
		r->id = i;	
		r->countdown = TEST_SCHED_COUNTDOWN;
		r->data = test_sched_data;
		r->sched_ent.exec = sched_test_ent_exec;
		ci_sched_ext_add(sched_grp, &r->sched_ent);
	}
}

static void stress_all()
{
	ci_node_each(node,
		ci_worker_each(node, worker, {
//			if ((node->node_id == 1) || (worker->worker_id >= 18)) continue;
//			if ((node->node_id == 0) || (worker->worker_id >= 18)) continue;
			sched_stress(node->node_id, worker->worker_id);
		});
	);
}

void test_sched()
{
	sched_stress(TEST_SCHED_NODE_ID, TEST_SCHED_WORKER_ID);
//	stress_all();
}


