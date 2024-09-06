#include "project.h"

static void do_insert(ClipRef *insertion, ClipRef **cr_list, uint16_t *num_clips, uint16_t insertion_point)
{
    for (uint16_t i=*num_clips; i > insertion_point; i--) {
	cr_list[i] = cr_list[i-1];
    }
    cr_list[insertion_point] = insertion;
    (*num_clips)++;
}

static inline void do_print(ClipRef *insertion, ClipRef **cr_list, uint16_t *num_clips, int16_t search_ind)
{
    fprintf(stderr, "--------------------------\n");
    for (int i=0; i<*num_clips; i++) {
	fprintf(stderr, "%d", cr_list[i]->pos_sframes);
	if (i == search_ind) {
	    fprintf(stderr, "\t<----%d\n", insertion->pos_sframes);
	} else {
	    fprintf(stderr, "\n");
	}
    }
}

void clipref_insert_sorted(ClipRef *insertion, ClipRef **cr_list, uint16_t *num_clips, bool by_end)
{
    if (*num_clips == 0) {
	do_insert(insertion, cr_list, num_clips, 0);
	return;
    }
    uint16_t len = *num_clips / 2;
    int16_t search_ind = *num_clips / 2;
    int direction = 0;
    while (len > 0) {
	ClipRef *test_clip = cr_list[search_ind];
	int32_t test_clip_pos = by_end ? test_clip->pos_sframes + clip_ref_len(test_clip) : test_clip->pos_sframes;
	int32_t clip_pos = by_end ? insertion->pos_sframes + clip_ref_len(insertion) : insertion->pos_sframes;
	/* do_print(insertion, cr_list, num_clips, search_ind); */
	if (clip_pos == test_clip_pos) {
	    do_insert(insertion, cr_list, num_clips, search_ind);
	    return;
	} else if (clip_pos > test_clip_pos) {
	    len/=2;
	    direction = 1;
	    search_ind+=len;
	} else {
	    len/=2;
	    direction = -1;
	    search_ind-=len;
	}
    }
    ClipRef *test_clip = cr_list[search_ind];
    int32_t test_clip_pos = by_end ? test_clip->pos_sframes + clip_ref_len(test_clip) : test_clip->pos_sframes;
    int32_t clip_pos = by_end ? insertion->pos_sframes + clip_ref_len(insertion) : insertion->pos_sframes;
    if (direction > 0) {
	while (clip_pos > test_clip_pos) {
	    search_ind++;
	    /* do_print(insertion, cr_list, num_clips, search_ind); */
	    if (search_ind >= *num_clips) {
		search_ind = *num_clips;
		break;
	    }
	    test_clip = cr_list[search_ind];
	    test_clip_pos = by_end ? test_clip->pos_sframes + clip_ref_len(test_clip) : test_clip->pos_sframes;
	    clip_pos = by_end ? insertion->pos_sframes + clip_ref_len(insertion) : insertion->pos_sframes;
	}
    } else {
	while (clip_pos < test_clip_pos) {
	    search_ind--;
	    /* do_print(insertion, cr_list, num_clips, search_ind); */
	    if (search_ind < 0) {
		search_ind = 0;
		break;
	    }
	    test_clip = cr_list[search_ind];
	    test_clip_pos = by_end ? test_clip->pos_sframes + clip_ref_len(test_clip) : test_clip->pos_sframes;
	    clip_pos = by_end ? insertion->pos_sframes + clip_ref_len(insertion) : insertion->pos_sframes;
	}
    }

    if (clip_pos > test_clip_pos && search_ind != *num_clips) {
	/* do_print(insertion, cr_list, num_clips, search_ind + 1); */
	/* fprintf(stderr, "========DONE========\n"); */
	do_insert(insertion, cr_list, num_clips, search_ind + 1);
    } else {
	/* do_print(insertion, cr_list, num_clips, search_ind ); */
	/* fprintf(stderr, "========DONE========\n"); */
	do_insert(insertion, cr_list, num_clips, search_ind);
    }
}

static int clipref_compare_fwd(const void *a, const void *b)
{
    ClipRef *cr_a = *(ClipRef **)a;
    ClipRef *cr_b = *(ClipRef **)b;
    return (int)cr_b->pos_sframes - (int)cr_a->pos_sframes;
}


static int clipref_compare_bckwrd(const void *a, const void *b)
{
    ClipRef *cr_a = *(ClipRef **)a;
    ClipRef *cr_b = *(ClipRef **)b;
    return (int)(cr_a->pos_sframes + clip_ref_len(cr_a)) - (int)(cr_b->pos_sframes + clip_ref_len(cr_b));
}

void clipref_list_fwdsort(ClipRef **list, uint16_t num_clips)
{
    /* qsort(void *base, size_t nel, size_t width, int (* _Nonnull compar)(const void *, const void *)) -> void */
    qsort((void *)list, (size_t)num_clips, sizeof(ClipRef *), clipref_compare_fwd);
}

void clipref_list_backsort(ClipRef **list, uint16_t num_clips)
{
    qsort((void *)list, (size_t)num_clips, sizeof(ClipRef *), clipref_compare_bckwrd);
}

void track_index_clips(Track *t, bool index_fwd, bool index_bckwd)
{
    
}
