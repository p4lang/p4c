extern packet_in {
}

parser filter(packet_in packet, out bool drop);
package Filter(filter f);
