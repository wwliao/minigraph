#include <string.h>
#include "mgpriv.h"
#include "sys.h"

void mg_idxopt_init(mg_idxopt_t *io)
{
	memset(io, 0, sizeof(mg_idxopt_t));
	io->k = 15;
	io->w = 10;
	io->bucket_bits = 14;
}

void mg_mapopt_init(mg_mapopt_t *mo)
{
	memset(mo, 0, sizeof(mg_mapopt_t));
	mo->seed = 11;
	mo->max_occ1 = 100;
	mo->max_occ_frac1 = 2e-4f;
	mo->median_occ = 1;
	mo->max_gap = 5000;
	mo->max_gap_ref = -1;
	mo->max_frag_len = 800;
	mo->max_lc_skip = 25, mo->max_lc_iter = 10000;
	mo->bw = 2000;
	mo->mini_batch_size = 500000000;
	mo->div = 0.1f;
	mo->chn_pen_gap = 1.0f, mo->chn_pen_skip = 0.05f;
	mo->min_lc_cnt = 2, mo->min_lc_score = 30;
	mo->min_gc_cnt = 3, mo->min_gc_score = 50;
	mo->mask_level = 0.5f;
	mo->sub_diff = 6;
	mo->best_n = 5;
	mo->pri_ratio = 0.8f;
	mo->pe_ori = 0; // FF
	mo->min_cov_mapq = 20;
	mo->min_cov_blen = 1000;
}

void mg_ggopt_init(mg_ggopt_t *go)
{
	memset(go, 0, sizeof(mg_ggopt_t));
	go->algo = MG_G_NONE;
	go->flag |= MG_G_NO_QOVLP;
	go->min_map_len = 50000;
	go->min_depth_len = 10000;
	go->min_mapq = 5;
	go->min_var_len = 250;
	go->match_pen = 10;
	// for ggs
	go->ggs_shrink_pen = 9;
	go->ggs_fc_kmer = 9, go->ggs_fc_max_occ = 100;
	go->ggs_min_end_cnt = 10;
	go->ggs_min_end_frac = 0.1f;
	go->ggs_max_kiden = 0.7f;
}

int mg_opt_set(const char *preset, mg_idxopt_t *io, mg_mapopt_t *mo, mg_ggopt_t *go)
{
	if (preset == 0) {
		mg_idxopt_init(io);
		mg_mapopt_init(mo);
		mg_ggopt_init(go);
	} else if (strcmp(preset, "lr") == 0) {
		io->k = 15, io->w = 10;
		mo->bw = 2000, mo->max_gap = 5000;
	} else if (strcmp(preset, "asm") == 0 || strcmp(preset, "ggs") == 0) {
		io->k = 19, io->w = 10;
		mo->max_gap = mo->bw = 10000;
		mo->min_lc_cnt = 3, mo->min_lc_score = 40;
		mo->min_gc_cnt = 5, mo->min_gc_score = 1000;
		mo->min_cov_mapq = 5;
		mo->min_cov_blen = 50000;
		mo->div = 0.01f;
		if (strcmp(preset, "ggs") == 0)
			go->algo = MG_G_GGSIMPLE, mo->best_n = 0;
	} else if (strcmp(preset, "se") == 0 || strcmp(preset, "sr") == 0) {
		io->k = 21, io->w = 10;
		mo->flag |= MG_M_SR | MG_M_HEAP_SORT | MG_M_2_IO_THREADS;
		mo->max_occ1 = 1000;
		mo->max_gap = 100, mo->bw = 100;
		mo->pri_ratio = 0.5f;
		mo->min_lc_cnt = 2, mo->min_lc_score = 25;
		mo->min_gc_cnt = 3, mo->min_gc_score = 40;
		mo->mini_batch_size = 50000000;
		mo->min_cov_blen = 50;
		mo->chn_pen_gap = 0.2f;
		if (strcmp(preset, "sr") == 0) {
			mo->flag |= MG_M_FRAG_MODE | MG_M_FRAG_MERGE;
			mo->pe_ori = 0<<1|1; // FR
		}
	} else return -1;
	return 0;
}

int mg_opt_check(const mg_idxopt_t *io, const mg_mapopt_t *mo, const mg_ggopt_t *go)
{
	if ((mo->flag & MG_M_FRAG_MODE) && !(mo->flag & MG_M_FRAG_MERGE)) {
		if (mg_verbose >= 1)
			fprintf(stderr, "[ERROR]\033[1;31m the fragment-without-merge mode is not implemented\033[0m\n");
		return -1;
	}
	return 0;
}

void mg_opt_update(const mg_idx_t *gi, mg_mapopt_t *mo, mg_ggopt_t *go)
{
	int32_t max_occ1, median;
	max_occ1 = mg_idx_cal_quantile(gi, mo->max_occ_frac1, &median);
	if (max_occ1 > mo->max_occ1)
		mo->max_occ1 = max_occ1;
	mo->median_occ = median;
	if (mg_verbose >= 3)
		fprintf(stderr, "[M::%s::%.3f*%.2f] minimizer median occurrence: %d; cutoff: %d\n", __func__, realtime() - mg_realtime0, cputime() / (realtime() - mg_realtime0), median, mo->max_occ1);
}
