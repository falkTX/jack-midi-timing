/*
    Copyright (C) 2017 falkTX
    This work is free. You can redistribute it and/or modify it under the
    terms of the Do What The Fuck You Want To Public License, Version 2,
    as published by Sam Hocevar. See the COPYING file for more details.
*/

#include <jack/jack.h>
#include <jack/midiport.h>
#include <stdio.h>
#include <unistd.h>

#define RATE_DIVIDER 8

typedef struct {
    jack_client_t* client;
    jack_port_t* port;
//     uint32_t average_1sec;
//     uint32_t average_1sec_calc_table[RATE_DIVIDER];
//     uint32_t average_1sec_calc_index;
//     uint32_t average_1sec_calc_frame;
    double   average_total;
    uint32_t last_event_diff;
    uint32_t last_event_full_frame;
} jack_data_t;

static int process_callback(jack_nframes_t frames, void* ptr)
{
    jack_data_t* data = (jack_data_t*)ptr;

    void* port_buf = jack_port_get_buffer(data->port, frames);

//     data->average_1sec_calc_frame += frames;

    const uint32_t ev_count   = jack_midi_get_event_count(port_buf);
    const uint32_t full_frame = jack_last_frame_time(data->client);

    if (ev_count > 0)
    {
        jack_midi_event_t ev;
        uint32_t ev_full_frame;

        for (uint32_t i=0; i<ev_count; ++i)
        {
            if (jack_midi_event_get(&ev, port_buf, i) != 0)
                break;

            ev_full_frame = full_frame + ev.time;

            if (data->last_event_full_frame == 0)
            {
                // first event, skip calcs
            }
            else if (data->last_event_full_frame > ev_full_frame)
            {
                printf("Got an event in the past, wtf!?\n");
            }
            else
            {
                data->last_event_diff = ev_full_frame - data->last_event_full_frame;
                data->average_total = (double)(data->average_total + data->last_event_diff) / 2.0;
            }

            data->last_event_full_frame = ev_full_frame;
        }
    }

    return 0;
}

int main(void)
{
    jack_data_t* data = (jack_data_t*)malloc(sizeof(jack_data_t));

    if (data == NULL)
        return 1;

    int ret;

    data->client = jack_client_open("midi-receiver", JackNoStartServer, NULL);

    if (data->client == NULL)
    {
        ret = 1;
        goto free;
    }

    data->port = jack_port_register(data->client, "in", JACK_DEFAULT_MIDI_TYPE,
                                    JackPortIsInput|JackPortIsPhysical|JackPortIsTerminal, 0x0);

    if (data->port == NULL)
    {
        ret = 1;
        goto client_close;
    }

    const uint32_t sample_rate = jack_get_sample_rate(data->client);

    // default variables
    const uint32_t expected = sample_rate / RATE_DIVIDER;

//     data->average_1sec  = expected;
    data->average_total = expected;

//     data->average_1sec_calc_index = 0;
//     data->average_1sec_calc_frame = 0;
//     for (uint32_t i=0; i<RATE_DIVIDER; ++i)
//         data->average_1sec_calc_table[i] = expected;

    data->last_event_diff = 0;
    data->last_event_full_frame = 0;

    jack_set_process_callback(data->client, process_callback, data);

    if (jack_activate(data->client) != 0)
    {
        ret = 1;
        goto client_close;
    }

    ret = 0;
    while (1)
    {
        usleep(250*1000);

        if (data->last_event_full_frame != 0)
            printf("Expected: %i. Got last event %i and total average %f\n",
                    expected, data->last_event_diff, data->average_total);
    }

    jack_deactivate(data->client);
    jack_port_unregister(data->client, data->port);

client_close:
    jack_client_close(data->client);

free:
    free(data);

    return ret;
}
