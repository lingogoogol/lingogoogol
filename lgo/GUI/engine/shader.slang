static float2 pos_arr[3] = float2[]
(
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

static float3 color_arr[3] = float3[]
(
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

struct vertex_out_t
{
    float3 color;
    float4 pos : SV_Position;
};

[shader("vertex")]
vertex_out_t vertex_main(uint vertex_id : SV_VertexID)
{
    vertex_out_t output;
    output.pos = float4(pos_arr[vertex_id], 0.0, 1.0);
    output.color = color_arr[vertex_id];
    return output;
}

[shader("fragment")]
float4 fragment_main(vertex_out_t input) : SV_Target
{
    float3 color = input.color;
    return float4(color, 1.0);
}
