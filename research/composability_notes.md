# Composability checkpoint

The compiled C long-range profile ran through 1,048,576 symbols. A-F marker-vs-control Hamming distances remained roughly 107-147 bits rather than decaying to zero; this proves perturbation survives, not that marker identity remains decodable.

H composability probe results: exact concatenation 10,000/10,000, associativity 10,000/10,000, tree reduction 10,000/10,000, and inverse-suffix recovery 10,000/10,000.

The C baseline set now also includes CRC64 and polynomial rolling update primitives.
