// Copyright 2018 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the "License"); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

// Author: Enrico Zelioli, ezelioli@iis.ee.ethz.ch
// Description: Delay and Buffer AXI-like handshaking

module stream_fifo_delay_dyn #(
  parameter type payload_t           = logic,
  parameter int unsigned MaxDelay    = 1024,
  parameter int unsigned Depth       = 4,      // Power of two
  // DO NOT EDIT, derived parameters
  parameter int unsigned CounterWidth = $clog2(MaxDelay) + 1
)(
  input  logic                    clk_i,
  input  logic                    rst_ni,
  input  logic [CounterWidth-1 : 0]     delay_i,
  input  payload_t                payload_i,
  output logic                    ready_o,
  input  logic                    valid_i,
  output payload_t                payload_o,
  input  logic                    ready_i,
  output logic                    valid_o
);

  if (Depth & (Depth - 1) == 0)
    $fatal(1, "Depth must be a power of two");

/*
  localparam int unsigned AddrWidth = (Depth > 1) ? $clog2(Depth) : 1;

  typedef logic    [AddrWidth-1:0] addr_t;
  typedef logic [CounterWidth-1:0] count_t;

  count_t count_val;

  logic fifo_empty, fifo_full;
  logic fifo_push, fifo_pop, fifo_next; 

  logic [AddrWidth:0] fill_level_d, fill_level_q;
  logic [AddrWidth:0] ready_level_d, ready_level_q;

  payload_t [Depth-1:0] fifo_d, fifo_q;

  count_t [Depth-1:0] target_d, target_q;

  addr_t read_pointer_d, read_pointer_q;
  addr_t write_pointer_d, write_pointer_q;
  addr_t ready_pointer_d, ready_pointer_q;

  assign fifo_push = valid_i & ready_o;
  assign fifo_pop  = valid_o & ready_i;
  assign fifo_next = (ready_level_q < fill_level_q) && (target_q[ready_pointer_q] == count_val);

  assign fifo_empty = fill_level_q == 0;
  assign fifo_full  = fill_level_q == Depth;

  assign ready_o   = ~fifo_full;
  assign valid_o   = ready_level_q > 0;
  assign payload_o = fifo_q[read_pointer_q];

  always_comb begin
    fifo_d          = fifo_q;
    target_d        = target_q;
    read_pointer_d  = read_pointer_q;
    write_pointer_d = write_pointer_q;
    ready_pointer_d = ready_pointer_q;
    fill_level_d    = fill_level_q;
    ready_level_d   = ready_level_q;
    if (fifo_push) begin
      fifo_d[write_pointer_q]   = payload_i;
      target_d[write_pointer_q] = count_val + delay_i + 1;
      write_pointer_d           = write_pointer_q + 1;
      fill_level_d              = fill_level_q + 1;
    end
    if (fifo_pop) begin
      read_pointer_d = read_pointer_q + 1;
      fill_level_d   = fill_level_q - 1;
      ready_level_d  = ready_level_q - 1;
    end
    if (fifo_next) begin
      ready_pointer_d = ready_pointer_q + 1;
      ready_level_d   = ready_level_q + 1;
    end
    if (fifo_push & fifo_pop) begin // keep fill level stable if push and pop in same cycle
      fill_level_d = fill_level_q;
    end
    if (fifo_next & fifo_pop) begin // keep ready level stable if next and pop in same cycle
      ready_level_d = ready_level_q;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if(~rst_ni) begin
      fill_level_q    <= '0;
      ready_level_q   <= '0;
      read_pointer_q  <= '0;
      write_pointer_q <= '0;
      ready_pointer_q <= '0;
    end else begin
      fill_level_q    <= fill_level_d;
      ready_level_q   <= ready_level_d;
      read_pointer_q  <= read_pointer_d;
      write_pointer_q <= write_pointer_d;
      ready_pointer_q <= ready_pointer_d;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if(~rst_ni) begin
      fifo_q   <= '0;
      target_q <= '0;
    end else begin
      fifo_q   <= fifo_d;
      target_q <= target_d;
    end
  end

*/

  // head_deadline : latest element's deadline
  // tail_deadline : next element's deadline
  logic [CounterWidth-1 : 0] count_val;
  logic [CounterWidth-1 : 0] head_deadline, tail_deadline;

  logic fifos_full, fifos_empty, fifos_push, fifos_pop;

  payload_t payload_fifo_o;

  assign tail_deadline = (count_val + delay_i);
  assign fifos_push = (delay_i != 0) & (~fifos_full) & valid_i;
  assign fifos_pop = (count_val == head_deadline) && !(fifos_empty);

  assign valid_o = (delay_i != 0) ? fifos_pop : valid_i;
  assign ready_o = !fifos_full;

  assign payload_o = (delay_i != 0) ? payload_fifo_o : payload_i;

  counter #(
    .WIDTH      ( CounterWidth )
  ) i_counter (
    .clk_i,
    .rst_ni,
    .clear_i    ( 1'b0         ),
    .en_i       ( 1'b1         ),
    .load_i     ( 1'b0         ),
    .down_i     ( 1'b0         ),
    .d_i        ( '0           ),
    .q_o        ( count_val    ),
    .overflow_o (              )
  );

  fifo_v3 #(
    .FALL_THROUGH(0),
    .DATA_WIDTH($bits(payload_t)),
    .DEPTH(Depth)
  ) data_fifo (
    .clk_i,
    .rst_ni,
    .flush_i(0),
    .testmode_i(0),
    .full_o(fifos_full),
    .empty_o(fifos_empty),
    .usage_o(),
    .data_i(payload_i),
    .push_i(fifos_push),
    .data_o(payload_fifo_o),
    .pop_i(fifos_pop)
  );

  fifo_v3 #(
    .FALL_THROUGH(0),
    .DATA_WIDTH(CounterWidth),
    .DEPTH(Depth)
  ) deadline_fifo (
    .clk_i,
    .rst_ni,
    .flush_i(0),
    .testmode_i(0),
    .full_o(),
    .empty_o(),
    .usage_o(),
    .data_i(tail_deadline),
    .push_i(fifos_push),
    .data_o(head_deadline),
    .pop_i(fifos_pop)
  );

endmodule
