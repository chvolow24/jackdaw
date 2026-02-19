#ifndef JDAW_VU_METER_H
#define JDAW_VU_METER_H
#include "envelope_follower.h"
#include "layout.h"
#include "textbox.h"


typedef struct vu_meter {
    EnvelopeFollower *ef_L;
    EnvelopeFollower *ef_R;
    float amp_max;
    Layout *layout;
    Textbox *db_labels[6];
} VUMeter;


VUMeter *vu_meter_create(Layout *container, bool horizontal, EnvelopeFollower *ef_L, EnvelopeFollower *ef_R);
void vu_meter_destroy(VUMeter *vu);
void vu_meter_draw(VUMeter *vu);

#endif
