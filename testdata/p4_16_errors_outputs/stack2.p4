header h {
}

control p() {
    apply {
        h[5] stack1;
        h[3] stack2;
        stack1 = stack2;
        stack1.lastIndex = 3;
        stack2.size = 4;
    }
}

