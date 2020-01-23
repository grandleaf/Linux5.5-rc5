#include "ci.h"

static void my_probe(ci_mod_t *mod, ci_json_t *json)
{
	ci_m_printf(mod, "my_probe .......................\n");
	ci_mod_probe_done(mod, json);
}

static void my_init(ci_mod_t *mod, ci_json_t *json)
{
	ci_printf("my_init .......................\n");
	ci_mod_init_done(mod, json);
}

static void my_start(ci_mod_t *mod, ci_json_t *json)
{
	ci_printf("my_start .......................\n");
	ci_mod_start_done(mod, json);
}

static void my_stop(ci_mod_t *mod, ci_json_t *json)
{
	ci_printf("my_stop .......................\n");
	ci_mod_stop_done(mod, json);
}

static int tm_jcmd_read(ci_mod_t *mod, ci_json_t *json)
{
	int rv;

	struct {
		struct {	/* all flags are u8 */
			u8		lba;
			u8		len;
			u8		help;
			u8 		verbose;
			u8		test;
		} flag;

		u64			lba;
		u16			len;
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_required_has_arg(&opt, lba, 	'l', 	"logical block address"),
		ci_jcmd_opt_required_has_arg(&opt, len, 	'n', 	"length"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 	'h', 	"show help"),
		ci_jcmd_opt_optional_nil_arg(&opt, verbose, 'v', 	"verbose mode"),
		ci_jcmd_opt_optional_nil_arg(&opt, test, 	0, 		NULL),
		CI_EOT
	};	

	if (ci_jcmd_no_opt(json)) 		/* no opt/arg, let's do help */
		return ci_jcmd_dft_help(opt_tab);

	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;

	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);

	ci_printf("lba=%lld(%#llX), len=%d(%#X), verbose=%d, help=%d\n", opt.lba, opt.lba, opt.len, opt.len, opt.flag.verbose, opt.flag.help);

	return 0;
}

static ci_jcmd_t test_mod_jcmd[] = {
	{ "testm.read", 	tm_jcmd_read, 	"read a sector", 	"--lba <lba> --len <len> [--help] [--verbose] [--test]" },
	CI_EOT
};

ci_mod_def(mod_test, {
	.name = "testm",
	.desc = "test module for demo purpose",

	.vect = {
		[CI_MODV_PROBE] 	= my_probe,
		[CI_MODV_INIT] 		= my_init,
		[CI_MODV_START] 	= my_start,
		[CI_MODV_STOP]		= my_stop,
	},

	.mem = {
		.size_shr	= 10243,
		.size_node	= 20484,
		.size_worker	= 16482
	},

	.jcmd = test_mod_jcmd		
});

