struct in_stu {
    float4 pos: POS;
    float4 color: COLOR;
};

struct out_stu {
    float4 color: COLOR;
    float4 pos: SV_Position;
};

out_stu vertex(in_stu input) {
    out_stu output;
    output.pos = input.pos;
    output.color = input.color;
    return output;
}