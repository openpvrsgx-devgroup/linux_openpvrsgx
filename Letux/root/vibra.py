#!/usr/bin/env python
#
# Author: NeilBrown <neilb@suse.de>
#

import fcntl, struct, time, array

#
# There are two steps to creating a rumble effect
# 1/ describe the effect and give it to the driver using an
#    ioctl.
#   There a 3 paramaters:
#     strength:  from 0 to 0xffff - this code takes a value from 0 to
#                                   1 and scales it
#     duration: milliseconds
#     delay until start: milliseconds.
#
# 2/ write a request to play a specific effect.
#
# It is possible to have multiple effects active.  If they have
# different delays they will start at different times.
# This demo shows combining 3 non-overlapping effects to make
# a simple vibration pattern
#
# An effect is created with f.new_vibe(strength, duration, delay)
# That effect can then be started with 'play' and stopped with 'stop'.

# EVIOCRMFF = _IOW('E', 0x81, int)
# dir: 2  WRITE = 1 == 0x40000
# size 14  4
# type 8  'E' == 0x45
# nr: 8   0x81
#
EVIOCRMFF = 0x40044581
# EVIOCSFF _IOC(_IOC_WRITE, 'E', 0x80, sizeof(struct ff_effect))
EVIOCSFF = 0x402c4580
class Vibra:
    def __init__(self, file = "/dev/input/rumble"):
        self.f = open(file, "r+")

    def close(self):
        self.f.close()

    def new_vibe(self, strength, length, delay):
        # strength is from 0 to 1
        # length and delay are in millisecs
        # this is 'struct ff_effect' from "linux/input.h"
        effect = struct.pack('HhHHHHHxxHH',
                             0x50, -1, 0, # FF_RUMBLE, id, direction
                             0, 0,        # trigger (button interval)
                             length, delay,
                             int(strength * 0xFFFF), 0)
        a = array.array('h', effect)
        fcntl.ioctl(self.f, EVIOCSFF, a, True)
        return a[1]
        id = a[1]
        return (ev_play, ev_stop)

    def multi_vibe(self, length, repeats = 1, delay = None, strength = 1):
        start = 0
        if delay == None:
            delay = length
        v = []
        for i in range(0, repeats):
            v.append(self.new_vibe(strength, length, start))
            start += length + delay
        return v

    def play(self, id):
        # this is 'struct input_event': sec, nsec, type, code, value
        if type(id) == tuple or type(id) == list:
            ev_play = ''
            for i in id:
                ev_play = ev_play + struct.pack('LLHHi', 0, 0, 0x15, i, 1)
        else:
            ev_play = struct.pack('LLHHi', 0, 0, 0x15, id, 1)
        self.f.write(ev_play)
        self.f.flush()

    def stop(self, id):
        # this is 'struct input_event': sec, nsec, type, code, value
        if type(id) == tuple or type(id) == list:
            ev_stop = ''
            for i in id:
                ev_stop = ev_stop + struct.pack('LLHHi', 0, 0, 0x15, i, 0)
        else:
            ev_stop = struct.pack('LLHHi', 0, 0, 0x15, id, 0)
        self.f.write(ev_stop)
        self.f.flush()

    def forget(self, id):
        if type(id) == tuple or type(id) == list:
            for i in id:
                fcntl.ioctl(self.f, EVIOCRMFF, i)
        else:
            fcntl.ioctl(self.f, EVIOCRMFF, id)

if __name__ == '__main__':
    f = Vibra("/dev/input/rumble")

    # rumble for 300ms, pause for 100ms, rumble for 300ms, pause for 200ms
    # then half-speed rumble for 600ms
    p1 = f.new_vibe(1, 300, 0)
    p2 = f.new_vibe(1, 300,400)
    p3 = f.new_vibe(0.5, 600, 900)

    f.play((p1, p2, p3))

    time.sleep(2)
    f.forget((p1, p2, p3))

    f.play(f.multi_vibe(200, 14, delay=100))

    time.sleep(5)
