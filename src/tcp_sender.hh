#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , timer_( initial_RTO_ms )
    , outstanding_segments_()
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  // the (absolute) sequence number for the next byte to be sent
  uint64_t next_seqno_ { 0 };

  uint16_t window_size_ { 1 };

  // wheather the syn has been sent
  bool syn_sent_ { false };

  // wheather the fin has been sent
  bool fin_sent_ { false };

  class TCPTimer
  {
  private:
    // the RTO (retransmission timeout)
    uint64_t rto_;

    // the timer is running or not
    bool is_running_ { false };

    // time that has passed since last timeout
    uint64_t time_passed_ { 0 };

  public:
    TCPTimer( uint64_t rto ) : rto_( rto ) {}
    ~TCPTimer() {}

    // start the timer
    void start_timer()
    {
      time_passed_ = 0;
      is_running_ = true;
    }

    // stop the timer
    void stop_timer() { is_running_ = false; }

    // double the RTO
    void double_rto() { rto_ *= 2; }

    // the timer is running or not
    bool is_running() { return is_running_; }

    // set the rto
    void set_rto( uint64_t rto ) { rto_ = rto; }

    // time has passed
    // if timeout, return true
    bool tick( uint64_t ms_since_last_tick );
  };

  TCPTimer timer_;

  std::queue<TCPSenderMessage> outstanding_segments_;

  uint64_t sequence_numbers_in_flight_ { 0 };

  // the max ACK received before
  Wrap32 max_ack_ { 0 };

  uint64_t consecutive_retransmissions_ { 0 };

  uint16_t receiver_free_space_ { 1 };

  void send_syn_segment( const TransmitFunction& transmit );

  void send_segment( const TransmitFunction& transmit );

  void resend_segment( const TransmitFunction& transmit );
};
