This is p4testgen, a packet-test generator for P4 programs.

Usage:
=====================
Additional command line parameters:
  =====================
  The ```--top4 ``` option in combination with ```--dump``` can be used to dump the individual compiler pass. For example, ```p4testgen --target bmv2 --std p4-16 --arch v1model --dump dmp --top4   ".*" prog.p4``` will dump all the intermediate passes that are used in the dump folder. Whereas ```p4testgen --target bmv2 --std p4-16 --arch v1model --dump dmp --top4 "FrontEnd.*Side" prog.p4``` will only dump the side-effect ordering pass.
