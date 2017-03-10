/*
    Copyright (C) 2017 falkTX
    This work is free. You can redistribute it and/or modify it under the
    terms of the Do What The Fuck You Want To Public License, Version 2,
    as published by Sam Hocevar. See the COPYING file for more details.
*/

#include <jack/jack.h>
#include <jack/midiport.h>
#include <stdio.h>

static const uint32_t k_rate_divider = 8;

typedef struct {
    jack_client_t* client;
    jack_port_t* port;
    uint32_t cur_pos;
    uint32_t divider;
} jack_data_t;

static int process_callback(jack_nframes_t frames, void* ptr)
{
    jack_data_t* data = (jack_data_t*)ptr;

    void* port_buf = jack_port_get_buffer(data->port, frames);
    jack_midi_clear_buffer(port_buf);

    data->cur_pos += frames;

    if (data->cur_pos > data->divider)
    {
        uint32_t offset = frames - (data->cur_pos - data->divider);
        //printf("event at %i %i %i\n", data->cur_pos, data->divider, offset);

        if (offset >= frames)
        {
            printf("higher offset than frames, %i %i %i %i\n", offset, frames, data->cur_pos, data->divider);
            offset = frames-1;
        }

        const jack_midi_data_t mdata[3] = { 0x90, 60, 100 };
        jack_midi_event_write(port_buf, offset, mdata, 3);

        data->cur_pos = frames - offset;
    }

    return 0;
}

int main(void)
{
    jack_data_t* data = (jack_data_t*)malloc(sizeof(jack_data_t));

    if (data == NULL)
        return 1;

    int ret;

    data->client = jack_client_open("midi-sender", JackNoStartServer, NULL);

    if (data->client == NULL)
    {
        ret = 1;
        goto free;
    }

    data->port = jack_port_register(data->client, "out", JACK_DEFAULT_MIDI_TYPE,
                                    JackPortIsOutput|JackPortIsTerminal, 0x0);

    if (data->port == NULL)
    {
        ret = 1;
        goto client_close;
    }

    const uint32_t buffer_size = jack_get_buffer_size(data->client);
    const uint32_t sample_rate = jack_get_sample_rate(data->client);

    // send an event 8 times per second
    data->cur_pos = 0;
    data->divider = sample_rate / k_rate_divider;

    if (buffer_size >= sample_rate || data->divider < buffer_size)
    {
        ret = 1;
        goto port_unregister;
    }

    jack_set_process_callback(data->client, process_callback, data);

    if (jack_activate(data->client) != 0)
    {
        ret = 1;
        goto client_close;
    }

    ret = 0;
    while (1);

    jack_deactivate(data->client);

port_unregister:
    jack_port_unregister(data->client, data->port);

client_close:
    jack_client_close(data->client);

free:
    free(data);

    return ret;
}
