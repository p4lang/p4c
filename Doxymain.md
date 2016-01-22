# Doxygen ignores the first line

# IMPLEMENTING YOUR SWITCH TARGET WITH BMv2

This guide and the accompanying doxygen documentation are targetted at
developpers that wish to design their own switch model using the building blocks
exposed by bmv2. We have not documented every single method in doxygen; instead
only the public methods which can help you design and implement your own model
have been documented and are visible here.

If you are not yet familiar with bmv2 (if you have never used bmv2 before), you
should be starting with the [bmv2 README]
(https://github.com/p4lang/behavioral-model).

The bmv2 code already comes with 3 example targets: [simple_router]
(https://github.com/p4lang/behavioral-model/tree/master/targets/simple_router),
[l2_switch]
(https://github.com/p4lang/behavioral-model/tree/master/targets/l2_switch) and
[simple_switch]
(https://github.com/p4lang/behavioral-model/tree/master/targets/simple_switch).
simple_router is the smallest and simplest one, and I suggest starting with
it. l2_switch introduces some additional complexity by including a packet
replication engine (to support multicast). simple_switch is the standard P4
target and although it includes a lot of functionality, the code is still
relatively small and straightforward.

As the bmv2 code becomes more stable, we intend to improve this documentation
and include a step-by-step guide on how to design and implement your target
using bmv2. I believe that a step-by-step guide recreating the implementation of
the l2_switch target would be most useful.


## FAQ

TODO
