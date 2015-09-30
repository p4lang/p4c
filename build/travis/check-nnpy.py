import nnpy
import sys

pub = nnpy.Socket(nnpy.AF_SP, nnpy.PUB)
pub.bind('inproc://foo')

sub = nnpy.Socket(nnpy.AF_SP, nnpy.SUB)
sub.connect('inproc://foo')
sub.setsockopt(nnpy.SUB, nnpy.SUB_SUBSCRIBE, '')

pub.send('hello, world')
recv = sub.recv()

if recv != 'hello, world':
    sys.exit(1)
