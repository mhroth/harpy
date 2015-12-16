# Copyright (c) 2015, Martin Roth (mhroth@gmail.com). All Rights Reserved.

import math
import midi
from pythonosc import osc_message_builder
from pythonosc import osc_bundle_builder
import struct


def main():
    pattern = midi.read_midifile("drums.mid")
    pattern.make_ticks_abs()

    tempo = 137.0 # bpm

    # seconds per tick
    sec_per_tick = 60.0/tempo/pattern.resolution

    print pattern

    with open("drums.mid.osc", "wb") as f:
        for track in pattern:
            for event in track:
                # if isinstance(event, midi.NoteOnEvent):
                midi32 = ((event.statusmsg + event.channel) << 16) + \
                    (event.data[0] << 8) + \
                    event.data[1]
                print event.tick*sec_per_tick, event.data[0], event.data[1]
                s = event.tick*sec_per_tick
                timetag = (int(s) << 32) + int((s - math.floor(s)) * 4294967296.0)
                bundle = osc_bundle_builder.OscBundleBuilder(timetag)
                message = osc_message_builder.OscMessageBuilder("/slot0")
                message.add_arg(midi32)
                bundle.add_content(message.build())
                bundle = bundle.build()
                print bundle.dgram
                # write length of bundle, then bundle
                f.write(struct.pack('>I', len(bundle.dgram))) # big-endian unsigned 32-bit integer
                f.write(bundle.dgram)

if __name__ == "__main__":
    main()
