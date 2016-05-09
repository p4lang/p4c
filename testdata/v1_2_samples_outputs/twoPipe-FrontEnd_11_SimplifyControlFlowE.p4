extern packet_in {
}

extern packet_out {
}

struct inControl {
}

struct outControl {
}

parser Parser<IH>(packet_in b, out IH parsedHeaders);
control IMAP<T, IH, OH>(in IH inputHeaders, in inControl inCtrl, out OH outputHeaders, out T toEgress, out outControl outCtrl);
control OMAP<T, IH, OH>(in IH inputHeaders, in inControl inCtrl, in T fromIngress, out OH outputHeaders, out outControl outCtrl);
control Deparser<OH>(in OH outputHeaders, packet_out b);
package Ingress<T, IH, OH>(Parser<IH> p, IMAP<T, IH, OH> map, Deparser<OH> d);
package Egress<T, IH, OH>(Parser<IH> p, OMAP<T, IH, OH> map, Deparser<OH> d);
package Switch<T>(Ingress<T, _, _> ingress, Egress<T, _, _> egress);
