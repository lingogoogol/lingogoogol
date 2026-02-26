`include "Multiplier.v"
`include "Adder.v"

`define S0 8'h00
`define S1 8'h01
`define S2 8'h02

module Calculator(input wire clk, input wire rst, input wire data_valid, input wire[1:0] op, input wire[3:0] data, output reg result_valid, output reg[15:0] result);
    reg signed[15:0] mul_reg[0:3];
    wire signed[15:0] mul_wire[0:3];
    reg signed[3:0] data_copy[0:3];
    reg[1:0] op_copy;
    reg signed[3:0] max[0:4];
    reg signed[3:0] min[0:4];
    reg signed[15:0] add_reg[0:3];
    wire signed[15:0] add_wire[0:3];
    reg[7:0] counter;
    integer i;
    reg data_valid_last;
    
    genvar gen_i;
    generate
        for (gen_i = 0; gen_i < 4; gen_i = gen_i + 1) begin
            Multiplier multiplier(.clk(clk), .in1(mul_reg[gen_i]), .in2(data_copy[gen_i]), .out(mul_wire[gen_i]));
			Adder adder(.in1(add_reg[gen_i]), .in2({{12{data_copy[gen_i][3]}},data_copy[gen_i]}), .cin(1'b0), .out(add_wire[gen_i]), .cout());
        end
    endgenerate

    always @(*) begin
        for (i = 1; i < 4; i = i + 1) begin
            mul_reg[i] = mul_wire[i - 1];
            add_reg[i] = add_wire[i - 1];
        end
        for (i = 0; i < 4; i = i + 1) begin
            if (data_copy[i] > max[i]) begin
                max[i + 1] = data_copy[i];
            end else begin
                max[i + 1] = max[i];
            end
            if (data_copy[i] < min[i]) begin
                min[i + 1] = data_copy[i];
            end else begin
                min[i + 1] = min[i];
            end
        end
    end

    always @(posedge clk, posedge rst) begin
        if (rst) begin
            add_reg[0] <= 16'h0000;
            mul_reg[0] <= 16'h0001;
            max[0] <= 4'h8;
            min[0] <= 4'h7;
            op_copy <= 2'b00;
            for (i = 0; i < 4; i = i + 1) begin
                data_copy[i] <= 4'h0;
            end
            data_valid_last <= 0;
            result_valid <= 1'b0;
        end else begin
            case (op_copy)
                2'b00: begin
                    result <= add_wire[3];
                end
                2'b01: begin
                    result <= mul_wire[3];
                end
                2'b10: begin
                    result <= max[4];
                end
                2'b11: begin
                    result <= min[4];
                end
            endcase
            if (data_valid && !data_valid_last) begin
                counter <= 0;
                op_copy <= op;
                data_copy[0] <= data;
                result_valid <= 1'b0;
            end else begin
                counter <= counter + 1;
            end
            case (counter)
                `S0: begin
                    data_copy[1] <= data;
                end
                `S1: begin
                    data_copy[2] <= data;
                end
                `S2: begin
                    data_copy[3] <= data;
                end
                default: begin
                end
            endcase
            if (counter >= 50 && !data_valid) begin
                result_valid <= 1'b1;
            end
            data_valid_last <= data_valid;
        end
    end
endmodule
