struct in_stu {
    float4 color: COLOR;
};

struct out_stu {
    float4 color: SV_Target;
};

out_stu pixel(in_stu input) {
    out_stu output;
    output.color = input.color;
    return output;
}