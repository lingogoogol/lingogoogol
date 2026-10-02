struct vertex_in_t
{
    float3 pos;
    float3 color;
};

struct vertex_out_t
{
    float4 pos : SV_Position;
    float3 color;
};

[shader("vertex")]
vertex_out_t vertex_main(vertex_in_t input)
{
    vertex_out_t output;
    output.pos = float4(input.pos, 1.0);
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragment_main(vertex_out_t input) : SV_Target
{
    return float4(input.color, 1.0);
}
