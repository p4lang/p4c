The tests use vendor_copy.p4 and customer.p4 programs.

1. vendor_copy.p4 is akin to base.p4 for code reuse.
2. customer.p4 is akin to new P4 code for code reuse.
   Note, customer.p4 uses `override` and `super` keywords.
3. customer.p4 includes vendor_copy.p4. p4c lang parser needs to first
   process vendor_copy.p4 and then process customer.p4 to `override`
   or use `super`. This is why only customer.p4 is provided to `p4test`
   to generate a merged p4-16 program.  An example customer.p4 file exists
   in the `new-ethtype` sub-directory

```bash
./p4test --std p4-18 -I <path> customer.p4
```
