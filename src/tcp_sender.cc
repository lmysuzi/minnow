#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::send_syn_segment( const TransmitFunction& transmit )
{
  TCPSenderMessage segment = TCPSenderMessage();
  segment.SYN = true;
  segment.seqno = Wrap32::wrap( next_seqno_, isn_ );
  sequence_numbers_in_flight_ += 1;
  receiver_free_space_ -= 1;

  if ( input_.reader().is_finished() ) {
    segment.FIN = true;
    fin_sent_ = true;
    sequence_numbers_in_flight_ += 1;
    receiver_free_space_ -= 1;
  }

  segment.RST = input_.has_error();

  transmit( segment );
  outstanding_segments_.push( segment );
  next_seqno_ += segment.sequence_length();

  timer_.start_timer();
}

void TCPSender::send_segment( const TransmitFunction& transmit )
{
  TCPSenderMessage segment = TCPSenderMessage();
  segment.seqno = Wrap32::wrap( next_seqno_, isn_ );

  size_t payload_size = min( static_cast<size_t>( receiver_free_space_ ), TCPConfig::MAX_PAYLOAD_SIZE );
  payload_size = min( payload_size, input_.reader().bytes_buffered() );
  string payload;
  read( input_.reader(), payload_size, segment.payload );

  sequence_numbers_in_flight_ += payload_size;
  receiver_free_space_ -= payload_size;

  if ( input_.reader().is_finished() && receiver_free_space_ > 0 ) {
    payload_size += 1;
    segment.FIN = true;
    fin_sent_ = true;
    sequence_numbers_in_flight_ += 1;
    receiver_free_space_ -= 1;
  }

  segment.RST = input_.has_error();

  transmit( segment );
  outstanding_segments_.push( segment );
  next_seqno_ += segment.sequence_length();

  if ( !timer_.is_running() )
    timer_.start_timer();
}

void TCPSender::resend_segment( const TransmitFunction& transmit )
{
  if ( outstanding_segments_.empty() )
    return;

  transmit( outstanding_segments_.front() );

  if ( window_size_ > 0 ) {
    timer_.double_rto();
    consecutive_retransmissions_++;
  }
  if ( !timer_.is_running() )
    timer_.start_timer();
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if ( fin_sent_ )
    return;

  if ( !syn_sent_ ) {
    syn_sent_ = true;
    send_syn_segment( transmit );
    return;
  }

  while ( receiver_free_space_ > 0 ) {
    if ( input_.reader().bytes_buffered() )
      send_segment( transmit );
    else if ( input_.reader().is_finished() && !fin_sent_ ) {
      send_segment( transmit );
      break;
    } else
      break;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg = TCPSenderMessage();
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
  msg.RST = input_.has_error();
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST )
    input_.set_error();

  if ( !syn_sent_ )
    return;

  //去除不合法的ack
  if ( msg.ackno > Wrap32::wrap( next_seqno_, isn_ ) )
    return;

  while ( !outstanding_segments_.empty()
          && outstanding_segments_.front().seqno + outstanding_segments_.front().sequence_length() <= msg.ackno ) {
    sequence_numbers_in_flight_ -= outstanding_segments_.front().sequence_length();
    outstanding_segments_.pop();
  }

  if ( msg.ackno > max_ack_ ) {
    if ( msg.window_size )
      timer_.set_rto( initial_RTO_ms_ );
    if ( !outstanding_segments_.empty() )
      timer_.start_timer();
    consecutive_retransmissions_ = 0;
    max_ack_.set_value( msg.ackno.value() );
  }

  if ( outstanding_segments_.empty() ) {
    timer_.stop_timer();
  }

  window_size_ = msg.window_size;
  if ( window_size_ )
    receiver_free_space_
      = window_size_ > sequence_numbers_in_flight_ ? window_size_ - sequence_numbers_in_flight_ : 0;
  else
    receiver_free_space_ = sequence_numbers_in_flight_ > 0 ? 0 : 1;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( !timer_.tick( ms_since_last_tick ) )
    return;

  resend_segment( transmit );
}

bool TCPSender::TCPTimer::tick( uint64_t ms_since_last_tick )
{
  if ( !is_running_ )
    return false;

  time_passed_ += ms_since_last_tick;

  if ( time_passed_ >= rto_ ) {
    time_passed_ = 0;
    return true;
  }

  return false;
}