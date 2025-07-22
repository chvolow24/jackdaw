#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "midi_objs.h"

PmEvent note_create_event_no_ts(Note *note, uint8_t channel, bool is_note_off)
{
    PmEvent e;
    uint8_t status = is_note_off ? 0x80 + channel : 0x90 + channel;
    e.message = Pm_Message(status, note->note, note->velocity);
    return e;
}

MIDICC midi_cc(uint8_t channel, uint8_t type, uint8_t value,  int32_t clip_or_chunk_tl_start)
{
    MIDICC cc;
    cc.type = type;
    cc.channel = channel;
    cc.value = value;
    if (cc.type >= 64 && cc.type <= 69) {
	cc.is_switch = true;
	cc.switch_state = cc.value;
    }
    return cc;
}

MIDICC midi_cc_from_event(PmEvent *e, int32_t pos_rel)
{
    MIDICC cc;
    cc.pos_rel = pos_rel;
    uint8_t status = Pm_MessageStatus(e->message);
    cc.channel = status & 0x0F;
    cc.type = Pm_MessageData1(e->message);
    cc.value = Pm_MessageData2(e->message);
    if (cc.type >= 64 && cc.type <= 69) {
	cc.is_switch = true;
	cc.switch_state = cc.value;
    }
    return cc;
}

PmEvent midi_cc_create_event_no_ts(MIDICC *cc)
{
    PmEvent e;
    e.message = Pm_Message(
	0xB0 + cc->channel,
	cc->type,
	cc->value
    );
    return e;
}

float midi_pitch_bend_float_from_event(PmEvent *e)
{
    uint16_t value = Pm_MessageData1(e->message) + ((Pm_MessageData2(e->message) << 7));
    return (float)value / 16384.0f;
}

MIDIPitchBend midi_pitch_bend_from_event(PmEvent *e, int32_t pos_rel)
{
    MIDIPitchBend pb;
    pb.pos_rel = pos_rel;
    uint8_t status = Pm_MessageStatus(e->message);
    pb.channel = status & 0x0F;
    pb.data1 = Pm_MessageData1(e->message);
    pb.data2 = Pm_MessageData2(e->message);
    pb.value = Pm_MessageData1(e->message) + ((Pm_MessageData2(e->message) << 7));
    pb.floatval = (float)pb.value / 16384.0f;
    return pb;
}

PmEvent midi_pitch_bend_create_event_no_ts(MIDIPitchBend *pb)
{
    PmEvent e;
    e.message = Pm_Message(
	0xE0 + pb->channel,
	pb->data1,
	pb->data2
    );
    return e;
}

double mtof_calc(double m)
{
    return 440.0 * pow(2.0, ((m - 69.0) / 12.0));
}

const double MTOF[] = {
    8.176,
    8.662,
    9.177,
    9.723,
    10.301,
    10.913,
    11.562,
    12.250,
    12.978,
    13.750,
    14.568,
    15.434,
    16.352,
    17.324,
    18.354,
    19.445,
    20.602,
    21.827,
    23.125,
    24.500,
    25.957,
    27.500,
    29.135,
    30.868,
    32.703,
    34.648,
    36.708,
    38.891,
    41.203,
    43.654,
    46.249,
    48.999,
    51.913,
    55.000,
    58.270,
    61.735,
    65.406,
    69.296,
    73.416,
    77.782,
    82.407,
    87.307,
    92.499,
    97.999,
    103.826,
    110.000,
    116.541,
    123.471,
    130.813,
    138.591,
    146.832,
    155.563,
    164.814,
    174.614,
    184.997,
    195.998,
    207.652,
    220.000,
    233.082,
    246.942,
    261.626,
    277.183,
    293.665,
    311.127,
    329.628,
    349.228,
    369.994,
    391.995,
    415.305,
    440.000,
    466.164,
    493.883,
    523.251,
    554.365,
    587.330,
    622.254,
    659.255,
    698.456,
    739.989,
    783.991,
    830.609,
    880.000,
    932.328,
    987.767,
    1046.502,
    1108.731,
    1174.659,
    1244.508,
    1318.510,
    1396.913,
    1479.978,
    1567.982,
    1661.219,
    1760.000,
    1864.655,
    1975.533,
    2093.005,
    2217.461,
    2349.318,
    2489.016,
    2637.020,
    2793.826,
    2959.955,
    3135.963,
    3322.438,
    3520.000,
    3729.310,
    3951.066,
    4186.009,
    4434.922,
    4698.636,
    4978.032,
    5274.041,
    5587.652,
    5919.911,
    6271.927,
    6644.875,
    7040.000,
    7458.620,
    7902.133,
    8372.018,
    8869.844,
    9397.273,
    9956.063,
    10548.080,
    11175.300,
    11839.820,
    12543.850
};

