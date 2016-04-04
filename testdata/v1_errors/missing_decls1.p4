parser start {
    extract(data);
    return next;
}
control ingress {
    if (data.b2 == 0) {
    	apply(test1);
	apply(test2);
	apply(test3);
    }
    apply(test4);
}

