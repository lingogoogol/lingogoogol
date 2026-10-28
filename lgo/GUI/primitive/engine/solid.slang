struct vertex_in_t
{
    float3 pos;
    float3 color;
    uint vertex_id : SV_VertexID;
};

struct vertex_out_t
{
    float4 pos : SV_Position;
    float3 color;
};

struct cursor_t
{
    float2 center;
    float2 radius;
    float2 framebuffer;
    float2 padding;
};
[[vk::push_constant]] ConstantBuffer<cursor_t> cursor;

[shader("vertex")]
vertex_out_t vertex_main(vertex_in_t input)
{
    vertex_out_t output;
    output.pos = float4(input.pos, 1.0);
    output.color = input.color;
    if (cursor.radius.x > 0.0)
    {
        float2 corner = float2((input.vertex_id & 1) ? 1.0 : -1.0,
                              (input.vertex_id & 2) ? 1.0 : -1.0);
        float2 pixel = cursor.center + corner * cursor.radius;
        output.pos = float4(pixel / cursor.framebuffer * 2.0 - 1.0, 0.0, 1.0);
    }
    return output;
}

[shader("fragment")]
float4 fragment_main(vertex_out_t input) : SV_Target
{
    if (cursor.radius.x > 0.0)
    {
        float distance = length((input.pos.xy - cursor.center) / cursor.radius);
        if (distance >= 1.0) discard;
        float alpha = 0.12 * (1.0 - smoothstep(0.0, 1.0, distance));
        return float4(0.5, 0.5, 0.5, alpha);
    }
    return float4(input.color, 1.0);
}
