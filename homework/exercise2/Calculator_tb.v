`include "Calculator.v"

`timescale 1ns/1ns

module Calculator_tb;
    reg clk;
    reg rst;
    reg data_valid;
    reg[1:0] op;
    reg signed[3:0] data;
    wire result_valid;
    wire signed[15:0] result;
    reg[15:0] passed_num;

    Calculator calculator(.clk(clk), .rst(rst), .data_valid(data_valid), .op(op), .data(data), .result_valid(result_valid), .result(result));

    initial begin
        passed_num = 0;

        clk = 0;
        rst = 1;
        #10
        rst = 0;
        data_valid = 1;
        op = 2'b00;
        data = 5;
        #10
        data = 6;
        #10
        data = 3;
        #10
        data = 1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d + %d + %d + %d = %d", 5, 6, 3, 1, result);
        if (result == 5 + 6 + 3 + 1) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b00;
        data = 0;
        #10
        data = 3;
        #10
        data = 3;
        #10
        data = 7;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d + %d + %d + %d = %d", 0, 3, 3, 7, result);
        if (result == 0 + 3 + 3 + 7) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b00;
        data = -5;
        #10
        data = -4;
        #10
        data = 3;
        #10
        data = -1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d + %d + %d + %d = %d", -5, -4, 3, -1, result);
        if (result == -5 + -4 + 3 + -1) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b00;
        data = -8;
        #10
        data = 7;
        #10
        data = 7;
        #10
        data = -8;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d + %d + %d + %d = %d", -8, 7, 7, -8, result);
        if (result == -8 + 7 + 7 + -8) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b01;
        data = 5;
        #10
        data = 6;
        #10
        data = 3;
        #10
        data = 1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d * %d * %d * %d = %d", 5, 6, 3, 1, result);
        if (result == 5 * 6 * 3 * 1) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b01;
        data = 0;
        #10
        data = 3;
        #10
        data = 3;
        #10
        data = 7;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d * %d * %d * %d = %d", 0, 3, 3, 7, result);
        if (result == 0 * 3 * 3 * 7) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b01;
        data = 5;
        #10
        data = 4;
        #10
        data = 3;
        #10
        data = 1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d * %d * %d * %d = %d", 5, 4, 3, 1, result);
        if (result == 5 * 4 * 3 * 1) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b01;
        data = 7;
        #10
        data = 1;
        #10
        data = 3;
        #10
        data = 3;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("%d * %d * %d * %d = %d", 7, 1, 3, 3, result);
        if (result == 7 * 1 * 3 * 3) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b10;
        data = 5;
        #10
        data = 6;
        #10
        data = 3;
        #10
        data = 1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("max(%d, %d, %d, %d) = %d", 5, 6, 3, 1, result);
        if (result == 6) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b10;
        data = 0;
        #10
        data = 3;
        #10
        data = 3;
        #10
        data = 7;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("max(%d, %d, %d, %d) = %d", 0, 3, 3, 7, result);
        if (result == 7) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b10;
        data = -5;
        #10
        data = -4;
        #10
        data = 3;
        #10
        data = -1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("max(%d, %d, %d, %d) = %d", -5, -4, 3, -1, result);
        if (result == 3) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b10;
        data = -8;
        #10
        data = 7;
        #10
        data = 7;
        #10
        data = -8;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("max(%d, %d, %d, %d) = %d", -8, 7, 7, -8, result);
        if (result == 7) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b11;
        data = 5;
        #10
        data = 6;
        #10
        data = 3;
        #10
        data = 1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("min(%d, %d, %d, %d) = %d", 5, 6, 3, 1, result);
        if (result == 1) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b11;
        data = 0;
        #10
        data = 3;
        #10
        data = 3;
        #10
        data = 7;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("min(%d, %d, %d, %d) = %d", 0, 3, 3, 7, result);
        if (result == 0) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b11;
        data = -5;
        #10
        data = -4;
        #10
        data = 3;
        #10
        data = -1;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("min(%d, %d, %d, %d) = %d", -5, -4, 3, -1, result);
        if (result == -5) begin
            passed_num = passed_num + 1;
        end
        
        #10
        data_valid = 1;
        op = 2'b11;
        data = -8;
        #10
        data = 7;
        #10
        data = 7;
        #10
        data = -8;
        #10
        data_valid = 0;
        op = 0;
        data = 0;
        while (!result_valid) begin
            #10;
        end
        $display("min(%d, %d, %d, %d) = %d", -8, 7, 7, -8, result);
        if (result == -8) begin
            passed_num = passed_num + 1;
        end

        if (passed_num == 16) begin
            $display("All pass");
        end else begin
            $display("There are totally %d errors in this design", 16 - passed_num);
        end
        $display("simulation time: %dns", $time);
        #10 $stop;
    end
    
    always #5 clk = ~clk;
endmodule