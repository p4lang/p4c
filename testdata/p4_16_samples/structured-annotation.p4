#define TEXT_CONST "hello"
#define NUM_CONST 6

@number[2]
@empty[]
@kv[k1=1,k2=2]
@MixedExprList[1,TEXT_CONST,true,1==2,5+NUM_CONST]
@Labels[short="Short Label", hover="My Longer Table Label to appear in hover-help"]
@MixedKV[label="text", my_bool=true, int_val=6]
control c() {
    apply {}
}