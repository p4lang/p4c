header H {}
control C()() {
  H[0] stack1;
  H[0] stack2;
  apply {
    stack1 = {};
    stack2 = stack1;
    stack1.pop_front(1);
  }
}